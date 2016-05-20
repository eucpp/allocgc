#ifndef DIPLOMA_MANAGED_POOL_CHUNK_H
#define DIPLOMA_MANAGED_POOL_CHUNK_H

#include <bitset>
#include <cstdint>
#include <memory>
#include <mutex>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include "allocators/plain_pool_chunk.h"
#include "constants.h"
#include "iterator_facade.h"
#include "iterator_access.h"
#include "managed_memory_descriptor.h"
#include "atomic_bitset.h"
#include "managed_ptr.hpp"
#include "util.h"

namespace precisegc { namespace details {

class managed_pool_chunk : public managed_memory_descriptor, private noncopyable, private nonmovable
{
public:
    static const size_t CHUNK_MAXSIZE = atomic_bitset::SIZE;
    static const size_t CHUNK_MINSIZE = 4;
private:
    typedef std::uintptr_t uintptr;
    typedef allocators::plain_pool_chunk plain_pool_chunk;
    typedef std::bitset<CHUNK_MAXSIZE> bitset_t;
public:
    class iterator: public boost::iterator_facade<
              iterator
            , const managed_ptr
            , boost::random_access_traversal_tag>
            , managed_ptr
    {
    public:
        iterator() noexcept;
        iterator(const iterator&) noexcept = default;
        iterator(iterator&&) noexcept = default;

        iterator& operator=(const iterator&) noexcept = default;
        iterator& operator=(iterator&&) noexcept = default;
    private:
        friend class managed_pool_chunk;
        friend class boost::iterator_core_access;

        iterator(byte* ptr, managed_memory_descriptor* descr) noexcept;

        managed_ptr dereference() const;

        void increment() noexcept;
        void decrement() noexcept;

        bool equal(const iterator& other) const noexcept;

        void advance(ptrdiff_t n);
        ptrdiff_t distance_to(const iterator& other) const;

        size_t cell_size() const;

        managed_ptr m_ptr;
    };

    typedef managed_ptr pointer_type;
    typedef boost::iterator_range<iterator> range_type;

    managed_pool_chunk(byte* chunk, size_t size, size_t cell_size);
    ~managed_pool_chunk();

    managed_ptr allocate(size_t cell_size);
    void deallocate(const managed_ptr& ptr, size_t cell_size);

    bool contains(const managed_ptr& ptr) const noexcept;
    bool memory_available() const noexcept;
    bool empty() const noexcept;
    bool empty(size_t cell_size) const noexcept;

    byte* get_mem() const;
    size_t get_mem_size() const;
    size_t get_cell_size() const;

    managed_memory_descriptor* get_descriptor();

    void unmark();

    iterator begin();
    iterator end();
    range_type get_range();

    virtual bool get_mark(byte* ptr) const override;
    virtual bool get_pin(byte* ptr) const override;

    virtual void set_mark(byte* ptr, bool mark) override;
    virtual void set_pin(byte* ptr, bool pin) override;

    virtual bool is_live(byte* ptr) const override;

    virtual void sweep(byte* ptr) override;

    virtual object_meta* get_cell_meta(byte* ptr) const override;
    virtual byte* get_obj_begin(byte* ptr) const override;
    virtual byte* get_cell_begin(byte* ptr) const override;
private:
    static uintptr calc_mask(byte* chunk, size_t chunk_size, size_t cell_size);
    static size_t calc_cell_ind(byte* ptr, size_t obj_size, byte* base_ptr, size_t size);

    void deallocate(byte* ptr, size_t cell_size);

    size_t calc_cell_ind(byte* ptr) const;
    size_t get_log2_cell_size() const;

    plain_pool_chunk m_chunk;
    bitset_t m_alloc_bits;
    size_t m_cell_size;
    size_t m_log2_cell_size;
    uintptr m_mask;
    atomic_bitset m_mark_bits;
    bitset_t m_pin_bits;
};

}}

#endif //DIPLOMA_MANAGED_POOL_CHUNK_H
