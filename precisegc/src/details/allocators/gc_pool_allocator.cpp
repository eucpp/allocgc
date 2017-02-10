#include <libprecisegc/details/allocators/gc_pool_allocator.hpp>

#include <tuple>
#include <iterator>

#include <libprecisegc/details/allocators/gc_box.hpp>
#include <libprecisegc/details/compacting/two_finger_compactor.hpp>
#include <libprecisegc/details/compacting/fix_ptrs.hpp>
#include <libprecisegc/details/collectors/gc_new_stack_entry.hpp>
#include <libprecisegc/details/collectors/memory_index.hpp>

namespace precisegc { namespace details { namespace allocators {

gc_pool_allocator::gc_pool_allocator()
    : m_freelist(nullptr)
    , m_top(nullptr)
    , m_end(nullptr)
{}

gc_pool_allocator::~gc_pool_allocator()
{
    for (auto it = m_descrs.begin(); it != m_descrs.end(); ) {
        it = destroy_descriptor(it);
    }
}

gc_alloc::response gc_pool_allocator::allocate(const gc_alloc::request& rqst, size_t aligned_size)
{
    if (m_top == m_end) {
        return try_expand_and_allocate(aligned_size, rqst, 0);
    }
    return stack_allocation(aligned_size, rqst);
}

gc_alloc::response gc_pool_allocator::try_expand_and_allocate(size_t size,
                                                           const gc_alloc::request& rqst,
                                                           size_t attempt_num)
{
    byte*  blk;
    size_t blk_size;
    std::tie(blk, blk_size) = allocate_block(size);
    if (blk) {
        create_descriptor(blk, blk_size, size);
        m_top = blk;
        m_end = blk + blk_size;
        return stack_allocation(size, rqst);
    } else if (m_freelist) {
        return freelist_allocation(size, rqst);
    } else {
        if (attempt_num == 0) {
            gc_options opt;
            opt.kind = gc_kind::COLLECT;
            opt.gen  = 0;
            gc_initiation_point(initiation_point_type::HEAP_LIMIT_EXCEEDED, opt);
        } else if (attempt_num == 1) {
            gc_core_allocator::expand_heap();
        } else {
            throw gc_bad_alloc();
        }
        return try_expand_and_allocate(size, rqst, ++attempt_num);
    }
}

gc_alloc::response gc_pool_allocator::stack_allocation(size_t size, const gc_alloc::request& rqst)
{
    assert(m_top <= m_end - size);

    descriptor_t* descr = &m_descrs.back();
    byte* ptr = m_top;
    m_top += size;
    return init_cell(ptr, rqst, descr);
}

gc_alloc::response gc_pool_allocator::freelist_allocation(size_t size, const gc_alloc::request& rqst)
{
    assert(m_freelist);

    byte* ptr  = reinterpret_cast<byte*>(m_freelist);
    m_freelist = reinterpret_cast<byte**>(m_freelist[0]);

    assert(contains(ptr));

    memset(ptr, 0, size);
    descriptor_t* descr = static_cast<descriptor_t*>(gc_index_memory(ptr));
    return init_cell(ptr, rqst, descr);
}

gc_alloc::response gc_pool_allocator::init_cell(byte* cell_start, const gc_alloc::request& rqst, descriptor_t* descr)
{
    assert(descr);
    assert(cell_start);

    collectors::gc_new_stack_entry* stack_entry = reinterpret_cast<collectors::gc_new_stack_entry*>(rqst.buffer());
    stack_entry->descriptor = descr;

    byte* obj_start = descr->init_cell(cell_start, rqst.obj_count(), rqst.type_meta());
    return gc_alloc::response(obj_start, cell_start, descr->cell_size(), rqst.buffer());
}

gc_pool_allocator::iterator_t gc_pool_allocator::create_descriptor(byte* blk, size_t blk_size, size_t cell_size)
{
    m_descrs.emplace_back(blk, blk_size, cell_size);
    auto last = std::prev(m_descrs.end());
    gc_add_to_index(blk, blk_size, &(*last));
    return last;
}

gc_pool_allocator::iterator_t gc_pool_allocator::destroy_descriptor(iterator_t it)
{
    sweep(*it, false);
    gc_remove_from_index(it->memory(), it->size());
    deallocate_block(it->memory(), it->size());
    return m_descrs.erase(it);
}

std::pair<byte*, size_t> gc_pool_allocator::allocate_block(size_t cell_size)
{
    size_t chunk_size = descriptor_t::chunk_size(cell_size);
    return std::make_pair(gc_core_allocator::allocate(chunk_size), chunk_size);
}

void gc_pool_allocator::deallocate_block(byte* ptr, size_t size)
{
    gc_core_allocator::deallocate(ptr, size);
}

bool gc_pool_allocator::contains(byte* ptr) const
{
    for (auto& descr: m_descrs) {
        if (descr.contains(ptr)) {
            return true;
        }
    }
    return false;
}

gc_heap_stat gc_pool_allocator::collect(compacting::forwarding& frwd)
{
    gc_heap_stat stat;
    if (m_descrs.begin() == m_descrs.end()) {
        return stat;
    }
    shrink(stat);
    m_top = nullptr;
    m_end = m_top;
    m_freelist = nullptr;
    if (is_compaction_required(stat)) {
        compact(frwd, stat);
    } else {
        sweep(stat);
    }
    return stat;
}

void gc_pool_allocator::shrink(gc_heap_stat& stat)
{
    for (iterator_t it = m_descrs.begin(), end = m_descrs.end(); it != end; ) {
        stat.mem_before_gc += it->size();
        if (it->unused()) {
            stat.mem_freed += it->size();
            it = destroy_descriptor(it);
        } else {
            stat.mem_all    += it->size();
            stat.mem_live   += it->cell_size() * it->count_lived();
            stat.pinned_cnt += it->count_pinned();
            ++it;
        }
    }
}

void gc_pool_allocator::sweep(gc_heap_stat& stat)
{
    for (auto& descr: m_descrs) {
        stat.mem_freed += sweep(descr, true);
    }
}

size_t gc_pool_allocator::sweep(descriptor_t& descr, bool add_to_freelist)
{
    byte*  it   = descr.memory();
    byte*  end  = descr.memory() + descr.size();
    size_t size = descr.cell_size();

    size_t freed = 0;
    for (size_t i = 0; it < end; it += size, ++i) {
        if (descr.get_lifetime_tag(i) == gc_lifetime_tag::GARBAGE) {
            descr.finalize(i);
            freed += size;
            if (add_to_freelist) {
                insert_into_freelist(it);
            }
        } else if (descr.get_lifetime_tag(i) == gc_lifetime_tag::FREE && add_to_freelist) {
            insert_into_freelist(it);
        }
    }
    return freed;
}

void gc_pool_allocator::insert_into_freelist(byte* ptr)
{
    assert(contains(ptr));

    byte** next = reinterpret_cast<byte**>(ptr);
    next[0]     = reinterpret_cast<byte*>(m_freelist);
    m_freelist  = next;
}

void gc_pool_allocator::compact(compacting::forwarding& frwd, gc_heap_stat& stat)
{
    compacting::two_finger_compactor compactor;
    auto rng = memory_range();
    compactor(rng, frwd, stat);
}

void gc_pool_allocator::fix(const compacting::forwarding& frwd)
{
    auto rng = memory_range();
    compacting::fix_ptrs(rng.begin(), rng.end(), frwd);
}

void gc_pool_allocator::finalize()
{
    for (auto& descr: m_descrs) {
        descr.unmark();
    }
}

bool gc_pool_allocator::empty() const
{
    return m_descrs.empty();
}

bool gc_pool_allocator::is_compaction_required(const gc_heap_stat& stat) const
{
    return stat.residency() < RESIDENCY_COMPACTING_THRESHOLD;
//    return stat.residency() < RESIDENCY_COMPACTING_THRESHOLD
//           || (stat.residency() < RESIDENCY_NON_COMPACTING_THRESHOLD
//               && std::abs(stat.residency() - m_prev_residency) < RESIDENCY_EPS);
}

gc_pool_allocator::memory_range_type gc_pool_allocator::memory_range()
{
    return utils::flatten_range(m_descrs.begin(), m_descrs.end());
}

}}}