#ifndef DIPLOMA_GC_POOL_DESCRIPTOR_HPP
#define DIPLOMA_GC_POOL_DESCRIPTOR_HPP

#include <cassert>
#include <bitset>
#include <cstdint>
#include <memory>
#include <mutex>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <libprecisegc/details/gc_cell.hpp>
#include <libprecisegc/details/utils/bitset.hpp>
#include <libprecisegc/details/utils/utility.hpp>
#include <libprecisegc/details/allocators/gc_alloc_messaging.hpp>
#include <libprecisegc/details/gc_memory_descriptor.hpp>
#include <libprecisegc/details/constants.hpp>

namespace precisegc { namespace details { namespace allocators {

class gc_pool_descriptor : public gc_memory_descriptor, private utils::noncopyable, private utils::nonmovable
{
public:
    static const size_t CHUNK_MAXSIZE = MANAGED_CHUNK_OBJECTS_COUNT;
    static const size_t CHUNK_MINSIZE = 4;
private:
    typedef utils::bitset<CHUNK_MAXSIZE> bitset_t;
    typedef utils::sync_bitset<CHUNK_MAXSIZE> sync_bitset_t;

    class memory_iterator: public boost::iterator_facade<
              memory_iterator
            , gc_cell
            , boost::random_access_traversal_tag
            , gc_cell
        >
    {
    public:
        memory_iterator()
            : m_cell_size(0)
        {}

        memory_iterator(const memory_iterator&) = default;
        memory_iterator& operator=(const memory_iterator&) = default;
    private:
        friend class gc_pool_descriptor;
        friend class boost::iterator_core_access;

        memory_iterator(byte* ptr, gc_pool_descriptor* descr, size_t cell_size)
            : m_cell(gc_cell::from_cell_start(ptr, descr))
            , m_cell_size(cell_size)
        {
            assert(ptr);
            assert(descr);
            assert(cell_size > 0);
        }

        gc_cell dereference() const
        {
            return m_cell;
        }

        void increment()
        {
            m_cell.reset(m_cell.get() + m_cell_size);
        }

        void decrement()
        {
            m_cell.reset(m_cell.get() - m_cell_size);
        }

        bool equal(const memory_iterator& other) const
        {
            return m_cell.get() == other.m_cell.get();
        }

        gc_cell m_cell;
        size_t  m_cell_size;
    };
public:
    typedef memory_iterator iterator;
    typedef boost::iterator_range<memory_iterator> memory_range_type;

    static constexpr size_t chunk_size(size_t cell_size)
    {
        return cell_size * CHUNK_MAXSIZE;
    }

    gc_pool_descriptor(byte* chunk, size_t size, size_t cell_size);
    ~gc_pool_descriptor();

    byte*  memory() const;
    size_t size() const;

    gc_memory_descriptor* descriptor();

    byte* init_cell(byte* ptr, size_t obj_count, const gc_type_meta* type_meta);

    bool contains(byte* ptr) const;

    bool unused() const;
    void unmark();

    size_t count_lived() const;
    size_t count_pinned() const;

    double residency() const;

    memory_range_type memory_range();

    iterator begin();
    iterator end();

    bool get_mark(size_t idx) const;
    bool get_pin(size_t idx) const;

    void set_mark(size_t idx, bool mark);
    void set_pin(size_t idx, bool pin);

    bool get_mark(byte* ptr) const override;
    bool get_pin(byte* ptr) const override;

    void set_mark(byte* ptr, bool mark) override;
    void set_pin(byte* ptr, bool pin) override;

    gc_lifetime_tag get_lifetime_tag(size_t idx) const;
    gc_lifetime_tag get_lifetime_tag(byte* ptr) const override;

    size_t cell_size() const;
    size_t cell_size(byte* ptr) const override;
    byte*  cell_start(byte* ptr) const override;

    size_t object_count(byte* ptr) const override;
    const gc_type_meta* get_type_meta(byte* ptr) const override;

    void commit(byte* ptr, bool mark) override;
    void commit(byte* ptr, bool mark, const gc_type_meta* type_meta) override;

    void trace(byte* ptr, const gc_trace_callback& cb) const override;
    void move(byte* to, byte* from, gc_memory_descriptor* from_descr) override;

    void finalize(size_t i);
    void finalize(byte* ptr) override;
private:
    bool is_init(byte* ptr) const;
    void set_init(byte* ptr, bool init);

    bool is_init(size_t idx) const;
    void set_init(size_t idx, bool init);

    size_t calc_cell_ind(byte* ptr) const;

    byte*         m_memory;
    size_t        m_size;
    size_t        m_cell_size;
    bitset_t      m_pin_bits;
    bitset_t      m_init_bits;
    sync_bitset_t m_mark_bits;
};

}}}

#endif // DIPLOMA_GC_POOL_DESCRIPTOR_HPP