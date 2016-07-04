#ifndef DIPLOMA_GC_PTR_ACCESS_H
#define DIPLOMA_GC_PTR_ACCESS_H

#include <type_traits>

#include <libprecisegc/gc_ptr.hpp>
#include <libprecisegc/gc_pin.hpp>

namespace precisegc {

namespace details { namespace ptrs {

template <typename T>
class gc_ptr_access
{
    typedef typename std::remove_extent<T>::type U;
public:
    static gc_untyped_ptr& get_untyped(gc_ptr<T>& ptr)
    {
        return (gc_untyped_ptr&) ptr;
    }

    static gc_ptr<T> create(U* raw_ptr)
    {
        return gc_ptr<T>(raw_ptr);
    }

    static T* get(const gc_ptr<T>& ptr)
    {
        return reinterpret_cast<T*>(ptr.get());
    }
};

}}}

#endif //DIPLOMA_GC_PTR_ACCESS_H
