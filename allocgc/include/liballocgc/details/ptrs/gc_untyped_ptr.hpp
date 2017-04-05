#ifndef ALLOCGC_GC_PTR_BASE_H
#define ALLOCGC_GC_PTR_BASE_H

#include <cstddef>
#include <cstdint>
#include <atomic>

#include <liballocgc/gc_handle.hpp>
#include <liballocgc/gc_common.hpp>

namespace allocgc { namespace details { namespace ptrs {

class gc_untyped_ptr
{
public:
    gc_untyped_ptr();
    gc_untyped_ptr(byte* ptr);
//    gc_untyped_ptr(byte* ptr, bool is_root);
    gc_untyped_ptr(const gc_untyped_ptr& other);
    ~gc_untyped_ptr();

    gc_untyped_ptr& operator=(std::nullptr_t);
    gc_untyped_ptr& operator=(const gc_untyped_ptr& other);

    gc_handle::pin_guard untyped_pin() const;
    gc_handle::stack_pin_guard push_untyped_pin() const;

    bool is_null() const;
    bool is_root() const;

    bool equal(const gc_untyped_ptr& other) const;

    void advance(ptrdiff_t n);

    byte* get() const;

    void swap(gc_untyped_ptr& other);
    friend void swap(gc_untyped_ptr& a, gc_untyped_ptr& b);
private:
    gc_handle m_handle;
};

}}}

#endif //ALLOCGC_GC_PTR_BASE_H