#include "gc_untyped_ptr.h"

#include <cstdint>
#include <cassert>
#include <utility>


#include <libprecisegc/details/logging.h>
#include <libprecisegc/details/threads/managed_thread.hpp>

#include "gc_new_stack.h"
#include "gc_unsafe_scope.h"
#include "write_barrier.h"
#include "logging.h"

namespace precisegc { namespace details {

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
//    gc_unsafe_scope unsafe_scope;
    write_barrier(*this, other);
}

gc_untyped_ptr::gc_untyped_ptr(gc_untyped_ptr&& other) noexcept
    : gc_untyped_ptr()
{
//    gc_unsafe_scope unsafe_scope;
    write_barrier(*this, other);
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
//    gc_unsafe_scope unsafe_scope;
    write_barrier(*this, other);
    return *this;
}

gc_untyped_ptr& gc_untyped_ptr::operator=(gc_untyped_ptr&& other) noexcept
{
//    gc_unsafe_scope unsafe_scope;
    write_barrier(*this, other);
    return *this;
}

void gc_untyped_ptr::swap(gc_untyped_ptr& other) noexcept
{
//    gc_unsafe_scope unsafe_scope;
    gc_untyped_ptr tmp = (*this);
    (*this) = other;
    other = tmp;
}

void* gc_untyped_ptr::get() const noexcept
{
    return m_ptr.load(std::memory_order_acquire);
}

void gc_untyped_ptr::set(void* ptr) noexcept
{
    m_ptr.store(reinterpret_cast<byte*>(ptr), std::memory_order_release);
}

void gc_untyped_ptr::atomic_store(const gc_untyped_ptr& value)
{
//    gc_unsafe_scope unsafe_scope;
    m_ptr.store(reinterpret_cast<byte*>(value.get()), std::memory_order_release);
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

}}