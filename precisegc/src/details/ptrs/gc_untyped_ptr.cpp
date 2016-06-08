#include <libprecisegc/details/ptrs/gc_untyped_ptr.hpp>

#include <cstdint>
#include <cassert>
#include <utility>


#include <libprecisegc/details/threads/managed_thread.hpp>
#include <libprecisegc/details/gc_hooks.hpp>
#include <libprecisegc/details/logging.h>

namespace precisegc { namespace details { namespace ptrs {

thread_local gc_new_stack& gc_untyped_ptr::gcnew_stack = gc_new_stack::instance();

gc_untyped_ptr::gc_untyped_ptr() noexcept
    : gc_untyped_ptr(nullptr)
{}

//gc_untyped_ptr::gc_untyped_ptr(nullptr_t) noexcept
//    : gc_untyped_ptr()
//{}

gc_untyped_ptr::gc_untyped_ptr(void* ptr) noexcept
    : m_ptr(reinterpret_cast<byte*>(ptr))
    , m_root_flag(!gcnew_stack.is_active())
{
    if (m_root_flag) {
        register_root();
    } else {
        if (gcnew_stack.is_meta_requsted()) {
            assert((void*) this >= gcnew_stack.get_top_pointer());
            uintptr_t this_uintptr = reinterpret_cast<uintptr_t>(this);
            uintptr_t top_uintptr = reinterpret_cast<uintptr_t>(gcnew_stack.get_top_pointer());
            if (top_uintptr <= this_uintptr && this_uintptr < top_uintptr + gcnew_stack.get_top_size()) {
                gcnew_stack.get_top_offsets().push_back(this_uintptr - top_uintptr);
            }
        }
    }
}

gc_untyped_ptr::gc_untyped_ptr(const gc_untyped_ptr& other) noexcept
    : gc_untyped_ptr()
{
    gc_wbarrier(m_ptr, other.m_ptr);
}

gc_untyped_ptr::gc_untyped_ptr(gc_untyped_ptr&& other) noexcept
    : gc_untyped_ptr()
{
    gc_wbarrier(m_ptr, other.m_ptr);
}

gc_untyped_ptr::~gc_untyped_ptr() noexcept
{
    if (is_root()) {
        delete_root();
    }
}

gc_untyped_ptr& gc_untyped_ptr::operator=(nullptr_t t) noexcept
{
    set(nullptr);
    return *this;
}

gc_untyped_ptr& gc_untyped_ptr::operator=(const gc_untyped_ptr& other) noexcept
{
    gc_wbarrier(m_ptr, other.m_ptr);
    return *this;
}

gc_untyped_ptr& gc_untyped_ptr::operator=(gc_untyped_ptr&& other) noexcept
{
    gc_wbarrier(m_ptr, other.m_ptr);
    return *this;
}

void gc_untyped_ptr::swap(gc_untyped_ptr& other) noexcept
{
    gc_untyped_ptr tmp = (*this);
    (*this) = other;
    other = tmp;
}

void* gc_untyped_ptr::get() const noexcept
{
    return gc_rbarrier(m_ptr);
}

void gc_untyped_ptr::set(void* ptr) noexcept
{
    gc_wbarrier(m_ptr, atomic_byte_ptr((byte*) ptr));
}

gc_untyped_ptr::operator bool() const noexcept
{
    return get() != nullptr;
}

bool gc_untyped_ptr::is_root() const noexcept
{
    return m_root_flag;
}

void gc_untyped_ptr::register_root() noexcept
{
    static thread_local root_set& rt_set = threads::managed_thread::this_thread().get_root_set();
    rt_set.add(this);
}

void gc_untyped_ptr::delete_root() noexcept
{
    static thread_local root_set& rt_set = threads::managed_thread::this_thread().get_root_set();
    rt_set.remove(this);
}

void swap(gc_untyped_ptr& a, gc_untyped_ptr& b) noexcept
{
    a.swap(b);
}

}}}