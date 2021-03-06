#ifndef ALLOCGC_APPROX_STACK_MAP_HPP
#define ALLOCGC_APPROX_STACK_MAP_HPP

#include <cmath>
#include <atomic>
#include <bitset>
#include <vector>

#include <boost/integer/static_log2.hpp>

#include <liballocgc/details/constants.hpp>
#include <liballocgc/details/utils/utility.hpp>
#include <liballocgc/details/utils/math.hpp>
#include <liballocgc/details/gc_interface.hpp>

namespace allocgc { namespace details { namespace collectors {

class stack_bitmap : private utils::noncopyable, private utils::nonmovable
{
public:
    stack_bitmap(std::thread::id id, byte* stack_addr, size_t stack_size);

    void register_root(gc_handle* root);
    void deregister_root(gc_handle* root);

    void trace(const gc_trace_callback& cb) const;

    size_t size() const;

    bool contains(const gc_handle* ptr) const;
private:
    static const size_t STACK_FRAME_SIZE = 512;
    static const size_t STACK_FRAME_MASK = STACK_FRAME_SIZE - 1;
    static const size_t STACK_FRAME_SIZE_LOG2 = boost::static_log2<STACK_FRAME_SIZE>::value;
    static const size_t GC_HANDLE_SIZE = sizeof(gc_handle);
    static const size_t GC_HANDLE_SIZE_LOG2 = boost::static_log2<GC_HANDLE_SIZE>::value;

    static_assert(check_pow2(STACK_FRAME_SIZE), "STACK_FRAME_SIZE is not a power of two");
    static_assert(check_pow2(GC_HANDLE_SIZE), "GC_HANDLE_SIZE is not a power of two");

    std::pair<size_t, size_t> root_idxs(const gc_handle* ptr) const;

    typedef std::bitset<STACK_FRAME_SIZE> bitmap_frame_t;

    gc_handle* m_stack_addr;
    size_t m_stack_size;
    std::vector<bitmap_frame_t> m_bitmap;
};

}}}

#endif //ALLOCGC_APPROX_STACK_MAP_HPP
