#ifndef ALLOCGC_TEST_CHUNK_H
#define ALLOCGC_TEST_CHUNK_H

#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <liballocgc/details/allocators/allocator_tag.hpp>
#include <liballocgc/gc_common.hpp>

class test_chunk
{
    typedef allocgc::byte byte;
public:
    typedef byte* pointer_type;

    static size_t chunk_size(size_t cell_size)
    {
        return cell_size;
    }

    static const size_t CHUNK_MINSIZE = 1;
    static const size_t CHUNK_MAXSIZE = 1;

    class iterator: public boost::iterator_facade<iterator, byte* const, boost::bidirectional_traversal_tag>
    {
    public:
        iterator() noexcept
            : m_ptr(nullptr)
            , m_obj_size(0)
        {}

        iterator(const iterator&) noexcept = default;
        iterator(iterator&&) noexcept = default;

        iterator& operator=(const iterator&) noexcept = default;
        iterator& operator=(iterator&&) noexcept = default;

        friend class test_chunk;
    private:
        friend class boost::iterator_core_access;

        iterator(byte* ptr, size_t obj_size) noexcept
            : m_ptr(ptr)
            , m_obj_size(obj_size)
        {}

        reference dereference() const
        {
            return m_ptr;
        }

        void increment() noexcept
        {
            m_ptr += m_obj_size;
        }

        void decrement() noexcept
        {
            m_ptr -= m_obj_size;
        }

        bool equal(const iterator& other) const noexcept
        {
            return m_ptr == other.m_ptr;
        }

        byte* m_ptr;
        size_t m_obj_size;
    };

    typedef boost::iterator_range<iterator> range_type;

    test_chunk()
        : m_mem(nullptr)
        , m_size(0)
        , m_obj_size(0)
        , m_available(false)
    {}

    test_chunk(byte* mem, size_t size, size_t obj_size)
            : m_mem(mem)
            , m_size(size)
            , m_obj_size(obj_size)
            , m_available(true)
    {
        assert(mem);
        assert(size > 0);
//        assert(size == obj_size);
    }

    test_chunk(const test_chunk&) = delete;
    test_chunk(test_chunk&&) = default;

    test_chunk& operator=(const test_chunk&) = delete;
    test_chunk& operator=(test_chunk&&) = default;

    byte* allocate(size_t obj_size)
    {
        assert(obj_size == m_obj_size);
        if (m_available) {
            m_available = false;
            return m_mem;
        }
        return nullptr;
    }

    void deallocate(byte* ptr, size_t obj_size)
    {
        assert(obj_size == m_obj_size);
        assert(!m_available);
        assert(contains(ptr));
        m_available = true;
    }

    bool contains(byte* ptr) const noexcept
    {
        return ptr == m_mem;
    }

    bool memory_available() const noexcept
    {
        return m_available;
    }

    bool empty() const noexcept
    {
        return m_available;
    }

    void set_empty() noexcept
    {
        m_available = true;
    }

    byte* memory() const
    {
        return m_mem;
    }

    size_t size() const
    {
        return m_size;
    }

    iterator begin()
    {
        return iterator(m_mem, m_obj_size);
    }

    iterator end()
    {
        return iterator(m_mem + m_obj_size, m_obj_size);
    }

    range_type get_range()
    {
        return range_type(begin(), end());
    }
private:
    byte* m_mem;
    size_t m_size;
    size_t m_obj_size;
    bool m_available;
};

#endif //ALLOCGC_TEST_CHUNK_H
