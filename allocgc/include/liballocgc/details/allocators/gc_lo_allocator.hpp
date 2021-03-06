#ifndef ALLOCGC_GC_LO_ALLOCATOR_HPP
#define ALLOCGC_GC_LO_ALLOCATOR_HPP

#include <mutex>

#include <boost/range/iterator_range.hpp>

#include <liballocgc/gc_alloc.hpp>
#include <liballocgc/details/gc_interface.hpp>
#include <liballocgc/details/gc_cell.hpp>

#include <liballocgc/details/allocators/gc_box.hpp>
#include <liballocgc/details/allocators/allocator_tag.hpp>
#include <liballocgc/details/allocators/sys_allocator.hpp>
#include <liballocgc/details/allocators/list_allocator.hpp>
#include <liballocgc/details/allocators/gc_core_allocator.hpp>
#include <liballocgc/details/allocators/redirection_allocator.hpp>
#include <liballocgc/details/allocators/gc_object_descriptor.hpp>

#include <liballocgc/details/utils/locked_range.hpp>
#include <liballocgc/details/utils/dummy_mutex.hpp>
#include <liballocgc/details/utils/utility.hpp>

#include <liballocgc/details/compacting/forwarding.hpp>

namespace allocgc { namespace details { namespace allocators {

class gc_lo_allocator : private utils::noncopyable, private utils::nonmovable
{
    typedef list_allocator<
            redirection_allocator<gc_core_allocator>,
            utils::dummy_mutex
        > list_alloc_t;

    typedef gc_object_descriptor descriptor_t;
    typedef std::mutex mutex_t;

    class descriptor_iterator : public boost::iterator_adaptor<
              descriptor_iterator
            , typename list_alloc_t::iterator
            , descriptor_t
            , boost::bidirectional_traversal_tag
        >
    {
    public:
        descriptor_iterator(const descriptor_iterator &) noexcept = default;
        descriptor_iterator& operator=(const descriptor_iterator&) noexcept = default;
    private:
        friend class gc_lo_allocator;
        friend class boost::iterator_core_access;

        descriptor_iterator(const typename list_alloc_t::iterator& it)
            : descriptor_iterator::iterator_adaptor_(it)
        {}

        descriptor_t& dereference() const
        {
            return *get_descr(*this->base());
        }
    };

    class memory_iterator : public boost::iterator_adaptor<
              memory_iterator
            , typename list_alloc_t::iterator
            , gc_cell
            , boost::bidirectional_traversal_tag
            , gc_cell
        >
    {
    public:
        memory_iterator(const memory_iterator&) noexcept = default;
        memory_iterator& operator=(const memory_iterator&) noexcept = default;
    private:
        friend class gc_lo_allocator;
        friend class boost::iterator_core_access;

        memory_iterator(const typename list_alloc_t::iterator& it)
            : memory_iterator::iterator_adaptor_(it)
            , m_cell(gc_cell::from_cell_start(get_memblk(*it), get_descr(*it)))
        {}

        gc_cell dereference() const
        {
            return m_cell;
        }

        void increment() noexcept
        {
            ++this->base_reference();
            update_cell(this->base());
        }

        void decrement() noexcept
        {
            --this->base_reference();
            update_cell(this->base());
        }

        void update_cell(const typename list_alloc_t::iterator& it) noexcept
        {
            m_cell = gc_cell::from_cell_start(get_memblk(*it), get_descr(*it));
        }

        gc_cell m_cell;
    };

    typedef boost::iterator_range<memory_iterator> memory_range_type;
public:
    typedef stateful_alloc_tag alloc_tag;

    explicit gc_lo_allocator(gc_core_allocator* core_alloc);
    ~gc_lo_allocator();

    gc_alloc::response allocate(const gc_alloc::request& rqst);

    gc_collect_stat collect(compacting::forwarding& frwd);
    void fix(const compacting::forwarding& frwd);
    void finalize();

    gc_memstat stats();
private:
    static constexpr size_t get_blk_size(size_t alloc_size)
    {
        return align_size(sizeof(descriptor_t) + gc_box::box_size(alloc_size));
    }

    static constexpr size_t get_cell_size(size_t alloc_size)
    {
        return align_size(sizeof(descriptor_t) + gc_box::box_size(alloc_size)) - sizeof(descriptor_t);
    }

    static constexpr size_t align_size(size_t size)
    {
        return list_alloc_t::align_size(size, PAGE_SIZE);
    }

    static constexpr byte* align_by_page(byte* ptr)
    {
        return reinterpret_cast<byte*>(reinterpret_cast<std::uintptr_t>(ptr) & ((~0ull) << PAGE_SIZE_LOG2));
    }

    static constexpr descriptor_t* get_descr(byte* blk)
    {
        return reinterpret_cast<descriptor_t*>(blk);
    }

    static constexpr byte* get_blk_by_descr(descriptor_t* descr)
    {
        return reinterpret_cast<byte*>(descr);
    }

    static constexpr byte* get_memblk(byte* blk)
    {
        return blk + sizeof(descriptor_t);
    }

    void destroy(const descriptor_iterator& it);

    descriptor_iterator descriptors_begin();
    descriptor_iterator descriptors_end();

    memory_iterator memory_begin();
    memory_iterator memory_end();

    byte* allocate_blk(size_t size);
    void  deallocate_blk(byte* ptr, size_t size);

    list_alloc_t m_alloc;
    mutex_t      m_mutex;
};

}}}

#endif //ALLOCGC_GC_LO_ALLOCATOR_HPP
