#ifndef DIPLOMA_MSO_ALLOCATOR_HPP
#define DIPLOMA_MSO_ALLOCATOR_HPP

#include <cstring>
#include <array>
#include <utility>

#include <libprecisegc/details/allocators/mpool_allocator.hpp>
#include <libprecisegc/details/allocators/allocator_tag.hpp>
#include <libprecisegc/details/allocators/stl_adapter.hpp>

#include <libprecisegc/details/utils/static_thread_pool.hpp>
#include <libprecisegc/details/utils/utility.hpp>

#include <libprecisegc/details/compacting/forwarding.hpp>

#include <libprecisegc/details/constants.hpp>
#include <libprecisegc/details/logging.hpp>

namespace precisegc { namespace details { namespace allocators {

class mso_allocator : private utils::noncopyable, private utils::nonmovable
{
public:
    typedef gc_alloc_response pointer_type;
    typedef stateful_alloc_tag alloc_tag;
    typedef utils::static_thread_pool thread_pool_t;

    mso_allocator();

    gc_alloc_response allocate(const gc_alloc_request& rqst);

    gc_heap_stat collect(compacting::forwarding& frwd, thread_pool_t& thread_pool);
    void fix(const compacting::forwarding& frwd, thread_pool_t& thread_pool);
private:
    // we have buckets for each 2^k size
    // i.g. [32, 64, 128, 256, ...]
    static const size_t BUCKET_COUNT = LARGE_CELL_SIZE_BITS_CNT - MIN_CELL_SIZE_BITS_CNT + 1;

    typedef std::pair<size_t, mpool_allocator> bucket_t;

    std::array<bucket_t, BUCKET_COUNT> m_buckets;
};

}}}

#endif //DIPLOMA_MSO_ALLOCATOR_HPP
