#include "managed_pool_chunk.hpp"

#include <utility>

#include "libprecisegc/details/utils/math.hpp"
#include "logging.h"

namespace precisegc { namespace details {

managed_pool_chunk::managed_pool_chunk(byte* chunk, size_t size, size_t cell_size)
    : m_chunk(chunk, size, cell_size)
    , m_cell_size(cell_size)
    , m_log2_cell_size(log2(cell_size))
    , m_mask(calc_mask(chunk, size, cell_size))
{
    managed_ptr::add_to_index(chunk, size, get_descriptor());
}

managed_pool_chunk::~managed_pool_chunk()
{
    managed_ptr::remove_from_index(get_mem(), get_mem_size());
}

managed_ptr managed_pool_chunk::allocate(size_t size)
{
    assert(size == cell_size());
    byte* raw_ptr = m_chunk.allocate(size);
    return managed_ptr(raw_ptr, get_descriptor());
}

void managed_pool_chunk::deallocate(const managed_ptr& ptr, size_t cell_size)
{
    deallocate(ptr.get(), cell_size);
}

void managed_pool_chunk::deallocate(byte* ptr, size_t size)
{
    assert(size == cell_size());
    m_chunk.deallocate(ptr, size);
}

bool managed_pool_chunk::contains(const managed_ptr& ptr) const noexcept
{
    return m_chunk.contains(ptr.get());
}

bool managed_pool_chunk::memory_available() const noexcept
{
    return m_chunk.memory_available();
}

bool managed_pool_chunk::empty() const noexcept
{
    return m_mark_bits.none() && m_pin_bits.none();
}

byte* managed_pool_chunk::get_mem() const
{
    return m_chunk.get_mem();
}

size_t managed_pool_chunk::get_mem_size() const
{
    return m_chunk.get_mem_size();
}

memory_descriptor* managed_pool_chunk::get_descriptor()
{
    return this;
}

void managed_pool_chunk::unmark()
{
    m_mark_bits.reset_all();
    m_pin_bits.reset_all();
}

managed_pool_chunk::iterator managed_pool_chunk::begin()
{
    assert(get_mem());
    return iterator(get_mem(), get_descriptor());
}

managed_pool_chunk::iterator managed_pool_chunk::end()
{
    assert(get_mem());
    return iterator(get_mem() + get_mem_size(), get_descriptor());
}

managed_pool_chunk::range_type managed_pool_chunk::get_range()
{
    return range_type(begin(), end());
}

size_t managed_pool_chunk::calc_cell_ind(byte* ptr, size_t log2_cell_size, byte* base_ptr, size_t size)
{
    assert(base_ptr <= ptr && ptr < base_ptr + size);
    assert((ptr - base_ptr) % pow2(log2_cell_size) == 0);
    return (ptr - base_ptr) >> log2_cell_size;
}

bool managed_pool_chunk::get_mark(byte* ptr) const
{
    size_t ind = calc_cell_ind(ptr);
    return m_mark_bits.get(ind);
}

bool managed_pool_chunk::get_pin(byte* ptr) const
{
    size_t ind = calc_cell_ind(ptr);
    return m_pin_bits.get(ind);
}

void managed_pool_chunk::set_mark(byte* ptr, bool mark)
{
    size_t ind = calc_cell_ind(ptr);
    m_mark_bits.set(ind, mark);
}

void managed_pool_chunk::set_pin(byte* ptr, bool pin)
{
    size_t ind = calc_cell_ind(ptr);
    m_pin_bits.set(ind, pin);
}

void managed_pool_chunk::set_live(byte* ptr, bool live)
{
    m_chunk.set_live(ptr, m_cell_size, live);
}

bool managed_pool_chunk::is_live(byte* ptr) const
{
    return m_chunk.is_live(ptr, m_cell_size);
}

void managed_pool_chunk::sweep(byte* ptr)
{
    deallocate(managed_ptr(get_cell_begin(ptr), this), m_cell_size);
}

size_t managed_pool_chunk::cell_size() const
{
    return m_cell_size;
}

object_meta* managed_pool_chunk::get_cell_meta(byte* ptr) const
{
    byte* cell_ptr = get_cell_begin(ptr);
    return object_meta::get_meta_ptr((void*) cell_ptr, m_cell_size);
}

byte* managed_pool_chunk::get_cell_begin(byte* ptr) const
{
    uintptr uiptr = reinterpret_cast<uintptr>(ptr);
    uintptr res = (uiptr & m_mask);
    assert(res % m_cell_size == 0);
    return reinterpret_cast<byte*>(res);
}

byte* managed_pool_chunk::get_obj_begin(byte* ptr) const
{
    object_meta* meta = get_cell_meta(ptr);
    byte* cell_ptr = get_cell_begin(ptr);
    size_t obj_size = meta->get_class_meta()->get_type_size();
    size_t obj_ind = (ptr - cell_ptr) / obj_size;
    return cell_ptr + obj_ind * obj_size;
}

managed_pool_chunk::uintptr managed_pool_chunk::calc_mask(byte* chunk,
                                                          size_t chunk_size,
                                                          size_t cell_size)
{
    size_t chunk_size_bits = log2(chunk_size);
    size_t cell_size_bits = log2(cell_size);
    size_t bit_diff = chunk_size_bits - cell_size_bits;
    uintptr ptr = reinterpret_cast<uintptr>(chunk);
    return (ptr | (((1 << bit_diff) - 1) << cell_size_bits));
}

size_t managed_pool_chunk::calc_cell_ind(byte* ptr) const
{
    return managed_pool_chunk::calc_cell_ind(get_cell_begin(ptr),
                                             m_log2_cell_size,
                                             get_mem(),
                                             get_mem_size());
}

size_t managed_pool_chunk::get_log2_cell_size() const
{
    return m_log2_cell_size;
}

managed_pool_chunk::iterator::iterator() noexcept
{}

managed_pool_chunk::iterator::iterator(byte* ptr, memory_descriptor* descr) noexcept
    : m_ptr(ptr, descr)
{}

managed_ptr managed_pool_chunk::iterator::dereference() const
{
    return m_ptr;
}

bool managed_pool_chunk::iterator::equal(const managed_pool_chunk::iterator& other) const noexcept
{
    return m_ptr.get() == other.m_ptr.get();
}

void managed_pool_chunk::iterator::increment() noexcept
{
    m_ptr.advance(cell_size());
}

void managed_pool_chunk::iterator::decrement() noexcept
{
    m_ptr.advance(-cell_size());
}

void managed_pool_chunk::iterator::advance(ptrdiff_t n)
{
    m_ptr.advance(n * cell_size());
}

ptrdiff_t managed_pool_chunk::iterator::distance_to(const iterator& other) const
{
    return other.m_ptr.get() - m_ptr.get();
}

size_t managed_pool_chunk::iterator::cell_size() const
{
    managed_pool_chunk* chunk = static_cast<managed_pool_chunk*>(m_ptr.get_descriptor());
    return chunk->cell_size();
}

}}