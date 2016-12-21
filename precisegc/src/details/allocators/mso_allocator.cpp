#include <libprecisegc/details/allocators/mso_allocator.hpp>

#include <algorithm>

#include <libprecisegc/details/collectors/memory_index.hpp>

namespace precisegc { namespace details { namespace allocators {

mso_allocator::mso_allocator()
{
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        size_t sz_cls = 1ull << (i + MIN_CELL_SIZE_BITS_CNT);
        m_buckets[i].first = sz_cls;
    }
}

gc_alloc_response mso_allocator::allocate(const gc_alloc_request& rqst)
{
    size_t size = rqst.alloc_size() + sizeof(collectors::traceable_object_meta);
    assert(size <= LARGE_CELL_SIZE);
    auto it = std::lower_bound(m_buckets.begin(), m_buckets.end(), size,
                               [] (const bucket_t& a, size_t sz) { return a.first < sz; });
    return it->second.allocate(rqst, it->first);
}

gc_heap_stat mso_allocator::collect(compacting::forwarding& frwd, thread_pool_t& thread_pool)
{
    std::vector<std::function<void()>> tasks;
    std::array<gc_heap_stat, BUCKET_COUNT> part_stats;
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        if (m_buckets[i].second.empty()) {
            continue;
        }
        tasks.emplace_back([this, i, &frwd, &part_stats] {
            part_stats[i] = m_buckets[i].second.collect(frwd);
        });
    }
    thread_pool.run(tasks.begin(), tasks.end());

    gc_heap_stat stat;
    for (auto& part_stat: part_stats) {
        stat += part_stat;
    }

    return stat;
}

void mso_allocator::fix(const compacting::forwarding& frwd, thread_pool_t& thread_pool)
{
    std::vector<std::function<void()>> tasks;
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        if (m_buckets[i].second.empty()) {
            continue;
        }
        tasks.emplace_back([this, i, &frwd] {
            m_buckets[i].second.fix(frwd);
        });
    }
    thread_pool.run(tasks.begin(), tasks.end());
}

}}}
