#include "gc_heap.hpp"

#include <cassert>
#include <vector>
#include <functional>

#include <liballocgc/details/compacting/fix_ptrs.hpp>
#include <liballocgc/details/compacting/two_finger_compactor.hpp>
#include <liballocgc/details/threads/gc_thread_manager.hpp>
#include <liballocgc/details/utils/static_thread_pool.hpp>
#include <liballocgc/details/utils/math.hpp>
#include <liballocgc/details/logging.hpp>

namespace allocgc { namespace details {

gc_heap::gc_heap(const gc_factory::options& opt)
{
    m_conservative_mode = opt.conservative;
    m_conservative_mode = opt.compacting;
}

gc_alloc::response gc_heap::allocate(const gc_alloc::request& rqst)
{
    if (rqst.alloc_size() <= LARGE_CELL_SIZE) {
        return allocate_on_tlab(rqst);
    } else {
        return m_loa.allocate(rqst);
    }
}

gc_heap_stat gc_heap::collect(const threads::world_snapshot& snapshot, size_t threads_available,
                              collectors::static_root_set* static_roots)
{
    forwarding frwd;
    utils::static_thread_pool thread_pool(threads_available);

    gc_heap_stat stat;
    for (auto& kv: m_tlab_map) {
        stat += kv.second.collect(frwd, thread_pool);
    }
    stat += m_loa.collect(frwd);

    if (stat.mem_copied > 0) {
        for (auto& kv: m_tlab_map) {
            kv.second.fix(frwd, thread_pool);
        }
        m_loa.fix(frwd);

        auto fix_roots_cb = [&frwd] (gc_handle* root) {
            frwd.forward(root);
        };

        static_roots->trace(fix_roots_cb);
        if (!m_conservative_mode) {
            snapshot.trace_roots(fix_roots_cb);
        }
    }

    for (auto& kv: m_tlab_map) {
        kv.second.finalize();
    }
    m_loa.finalize();

    return stat;
}

gc_alloc::response gc_heap::allocate_on_tlab(const gc_alloc::request& rqst)
{
    static thread_local mso_alloc_t& tlab = get_tlab();
    return tlab.allocate(rqst);
}

gc_heap::mso_alloc_t& gc_heap::get_tlab()
{
    std::lock_guard<std::mutex> lock(m_tlab_map_mutex);
    return m_tlab_map[std::this_thread::get_id()];
}

}}