#include <libprecisegc/details/serial_gc.hpp>

#include <utility>

#include <libprecisegc/details/gc_unsafe_scope.h>
#include <libprecisegc/details/threads/thread_manager.hpp>
#include <libprecisegc/details/threads/world_state.hpp>

namespace precisegc { namespace details {

serial_gc::serial_gc(gc_compacting compacting, std::unique_ptr<initation_policy>&& init_policy)
    : m_initator(this, std::move(init_policy))
    , m_heap(compacting)
{}

managed_ptr serial_gc::allocate(size_t size)
{
    return m_heap.allocate(size);
}

byte* serial_gc::rbarrier(const atomic_byte_ptr& p)
{
    return p.load(std::memory_order_relaxed);
}

void serial_gc::wbarrier(atomic_byte_ptr& dst, const atomic_byte_ptr& src)
{
    gc_unsafe_scope unsafe_scope;
    dst.store(src.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void serial_gc::initation_point(initation_point_type ipoint)
{
    m_initator.initation_point(ipoint);
}

gc_stat serial_gc::stat() const
{
    gc_stat stat;
//    stat.heap_size                  = m_heap.size();
    stat.heap_size                  = 0;
    stat.incremental                = false;
    stat.support_concurrent_mark    = false;
    stat.support_concurrent_sweep   = false;
    return stat;
}

void serial_gc::gc()
{
    using namespace threads;
    world_state wstate = thread_manager::instance().stop_the_world();
    m_marker.trace_roots(wstate);
    m_marker.trace_pins(wstate);
    m_marker.mark();
    m_heap.sweep();
}

}}

