#include <libprecisegc/details/gc_hooks.hpp>
#include <libprecisegc/details/gc_handle.hpp>

#include <libprecisegc/details/threads/static_root_set.hpp>
#include <libprecisegc/details/threads/this_managed_thread.hpp>
#include <libprecisegc/details/garbage_collector.hpp>

namespace precisegc { namespace details {

static garbage_collector gc_instance{};

void gc_initialize(std::unique_ptr<gc_strategy> strategy, std::unique_ptr<initiation_policy> init_policy)
{
    gc_instance.init(std::move(strategy), std::move(init_policy));
}

void gc_register_root(gc_handle* root)
{
    using namespace threads;
    if (this_managed_thread::is_stack_ptr(root)) {
        this_managed_thread::register_root(root);
    } else {
        static_root_set::register_root(root);
    }
}

void gc_deregister_root(gc_handle* root)
{
    using namespace threads;
    if (this_managed_thread::is_stack_ptr(root)) {
        this_managed_thread::deregister_root(root);
    } else {
        static_root_set::deregister_root(root);
    }
}

bool gc_is_root(const gc_handle* ptr)
{
    using namespace threads;
    return this_managed_thread::is_stack_ptr(ptr)
                ? this_managed_thread::is_root(ptr)
                : static_root_set::is_root(ptr);
}

bool gc_is_heap_ptr(const gc_handle* ptr)
{
    using namespace threads;
    return this_managed_thread::is_heap_ptr(ptr);
}

gc_alloc_descriptor gc_allocate(size_t obj_size, size_t obj_cnt, const type_meta* tmeta)
{
    return gc_instance.allocate(obj_size, obj_cnt, tmeta);
}

void gc_commit(const gc_alloc_descriptor& ptr)
{
    gc_instance.commit(ptr);
}

void gc_initiation_point(initiation_point_type ipoint, const initiation_point_data& ipd)
{
    gc_instance.initiation_point(ipoint, ipd);
}

gc_info gc_get_info()
{
    return gc_instance.info();
}

gc_stat gc_get_stats()
{
    return gc_instance.stats();
}

gc_state gc_get_state()
{
    return gc_instance.state();
}

void gc_enable_print_stats()
{
    gc_instance.set_printer_enabled(true);
}

void gc_disable_print_stats()
{
    gc_instance.set_printer_enabled(false);
}

void gc_register_page(const byte* page, size_t size)
{
    gc_instance.register_page(page, size);
}

void gc_deregister_page(const byte* page, size_t size)
{
    gc_instance.deregister_page(page, size);
}

byte* gc_handle::rbarrier() const
{
    return gc_instance.rbarrier(*this);
}

void gc_handle::wbarrier(const gc_handle& other)
{
    gc_instance.wbarrier(*this, other);
}

void gc_handle::interior_wbarrier(ptrdiff_t offset)
{
    gc_instance.interior_wbarrier(*this, offset);
}

gc_handle::pin_guard gc_handle::pin() const
{
    return pin_guard(*this);
}

gc_handle::stack_pin_guard gc_handle::push_pin() const
{
    return stack_pin_guard(*this);
}

void gc_handle::reset()
{
    gc_instance.wbarrier(*this, null);
}

bool gc_handle::equal(const gc_handle& other) const
{
    return gc_instance.compare(*this, other);
}

bool gc_handle::is_null() const
{
    return rbarrier() == nullptr;
}

gc_handle::pin_guard::pin_guard(const gc_handle& handle)
    : m_ptr(gc_instance.pin(handle))
{}

gc_handle::pin_guard::pin_guard(pin_guard&& other)
    : m_ptr(other.m_ptr)
{
    other.m_ptr = nullptr;
}

gc_handle::pin_guard::~pin_guard()
{
    gc_instance.unpin(m_ptr);
}

gc_handle::pin_guard& gc_handle::pin_guard::operator=(pin_guard&& other)
{
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
    return *this;
}

byte* gc_handle::pin_guard::get() const noexcept
{
    return m_ptr;
}

gc_handle::stack_pin_guard::stack_pin_guard(const gc_handle& handle)
    : m_ptr(gc_instance.push_pin(handle))
{}

gc_handle::stack_pin_guard::stack_pin_guard(stack_pin_guard&& other)
    : m_ptr(other.m_ptr)
{
    other.m_ptr = nullptr;
}

gc_handle::stack_pin_guard::~stack_pin_guard()
{
    gc_instance.pop_pin(m_ptr);
}

byte* gc_handle::stack_pin_guard::get() const noexcept
{
    return m_ptr;
}

gc_handle gc_handle::null{nullptr};

}}