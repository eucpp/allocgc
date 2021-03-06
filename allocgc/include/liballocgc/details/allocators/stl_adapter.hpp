#ifndef ALLOCGC_STL_ADAPTER_H
#define ALLOCGC_STL_ADAPTER_H

#include <type_traits>
#include <memory>
#include <limits>
#include <utility>

#include <liballocgc/details/allocators/allocator_tag.hpp>
#include <liballocgc/gc_common.hpp>
#include <liballocgc/details/utils/utility.hpp>

namespace allocgc { namespace details { namespace allocators {

template<typename T, typename Alloc>
class stl_adapter : private Alloc
{
    static_assert(std::is_same<typename Alloc::pointer_type, byte*>::value,
                  "stl_adapter should be used with raw memory allocator");

    typedef typename Alloc::alloc_tag alloc_tag;

    template <typename U, typename OtherAlloc>
    friend class stl_adapter;
public:
    typedef T value_type;
    typedef value_type& reference;
    typedef value_type const& const_reference;
    typedef value_type* pointer;
    typedef typename std::pointer_traits<pointer>::template rebind<const value_type> const_pointer;
    typedef typename std::pointer_traits<pointer>::template rebind<void> void_pointer;
    typedef typename std::pointer_traits<pointer>::template rebind<const void> const_void_pointer;
    typedef typename std::pointer_traits<pointer>::difference_type difference_type;
    typedef typename std::make_unsigned<difference_type>::type size_type;

    // stateful allocators should be propagated
    typedef typename std::is_same<alloc_tag, stateful_alloc_tag>::type propagate_on_container_copy_assignment;
    typedef typename std::is_same<alloc_tag, stateful_alloc_tag>::type propagate_on_container_move_assignment;
    typedef typename std::is_same<alloc_tag, stateful_alloc_tag>::type propagate_on_container_swap;

    template <typename U>
    struct rebind { typedef stl_adapter<U, Alloc> other; };

    stl_adapter() = default;
    stl_adapter(const stl_adapter&) = default;
    stl_adapter(stl_adapter&&) = default;

    template <typename U>
    stl_adapter(const stl_adapter<U, Alloc>& other)
        : Alloc(other)
    {}

    template <typename U>
    stl_adapter(stl_adapter<U, Alloc>&& other)
        : Alloc(std::move(other))
    {}

    stl_adapter& operator=(const stl_adapter&) = default;
    stl_adapter& operator=(stl_adapter&&) = default;

    T* allocate(size_t n)
    {
        return reinterpret_cast<T*>(Alloc::allocate(n * sizeof(T)));
    }

    void deallocate(T* ptr, size_t n)
    {
        Alloc::deallocate(reinterpret_cast<byte*>(ptr), n * sizeof(T));
    }

    static size_t max_size()
    {
        return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
    };

    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    };
};

}}}

#endif //ALLOCGC_STL_ADAPTER_H
