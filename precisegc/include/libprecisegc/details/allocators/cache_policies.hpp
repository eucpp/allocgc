#ifndef DIPLOMA_CACHE_POLICIES_HPP
#define DIPLOMA_CACHE_POLICIES_HPP

#include <cstddef>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <utility>

#include <libprecisegc/details/constants.hpp>
#include <libprecisegc/details/utils/utility.hpp>

namespace precisegc { namespace details { namespace allocators {

template <typename Iterator>
class always_expand : private utils::noncopyable, private utils::nonmovable
{
public:
    typedef typename Iterator::value_type chunk_type;
    typedef typename chunk_type::pointer_type pointer_type;

    always_expand() = default;

    explicit always_expand(const Iterator& init)
    {}

    pointer_type allocate(size_t size)
    {
        return nullptr;
    }

    bool memory_available(const Iterator& first, const Iterator& last) const
    {
        return false;
    }

    void update(const Iterator& it)
    {}

    void invalidate(const Iterator& it, const Iterator& deflt)
    {}
};

template <typename Iterator>
class single_chunk_cache : private utils::noncopyable, private utils::nonmovable
{
public:
    typedef typename Iterator::value_type chunk_type;
    typedef typename chunk_type::pointer_type pointer_type;

    single_chunk_cache() = default;

    explicit single_chunk_cache(const Iterator& init)
            : m_chunk(init)
    {}

    pointer_type allocate(size_t size)
    {
        return m_chunk->allocate(size);
    }

    bool memory_available(const Iterator& first, const Iterator& last) const
    {
        return m_chunk->memory_available();
    }

    void update(const Iterator& it)
    {
        m_chunk = it;
    }

    void invalidate(const Iterator& it, const Iterator& deflt)
    {
        if (m_chunk == it) {
            m_chunk = deflt;
        }
    }
private:
    Iterator m_chunk;
};

template <typename Iterator>
class single_chunk_with_search_cache
{
public:
    typedef typename Iterator::value_type chunk_type;
    typedef typename chunk_type::pointer_type pointer_type;

    single_chunk_with_search_cache() = default;

    explicit single_chunk_with_search_cache(const Iterator& init)
        : m_chunk(init)
    {}

    pointer_type allocate(size_t size)
    {
        return m_chunk->allocate(size);
    }

    bool memory_available(const Iterator& first, const Iterator& last) const
    {
        typedef typename std::iterator_traits<Iterator>::value_type chunk_type;
        if (!m_chunk->memory_available()) {
            m_chunk = std::find_if(first, last,
                                   [] (const chunk_type& chk) { return chk.memory_available(); });
            return m_chunk != last;
        }
        return true;
    }

    void update(const Iterator& it)
    {
        m_chunk = it;
    }

    void invalidate(const Iterator& it, const Iterator& deflt)
    {
        if (m_chunk == it) {
            m_chunk = deflt;
        }
    }
private:
    mutable Iterator m_chunk;
};

template <typename Iterator>
class single_chunk_with_forward_search_cache
{
public:
    typedef typename Iterator::value_type chunk_type;
    typedef typename chunk_type::pointer_type pointer_type;

    single_chunk_with_forward_search_cache() = default;

    explicit single_chunk_with_forward_search_cache(const Iterator& init)
        : m_chunk(init)
    {}

    pointer_type allocate(size_t size)
    {
        return m_chunk->allocate(size);
    }

    bool memory_available(const Iterator& first, const Iterator& last) const
    {
        typedef typename std::iterator_traits<Iterator>::value_type chunk_type;
        if (!m_chunk->memory_available()) {
            m_chunk = std::find_if(std::next(m_chunk), last,
                                   [] (const chunk_type& chk) { return chk.memory_available(); });
            return m_chunk != last;
        }
        return true;
    }

    void update(const Iterator& it)
    {
        m_chunk = it;
    }

    void invalidate(const Iterator& it, const Iterator& deflt)
    {
        if (m_chunk == it) {
            m_chunk = deflt;
        }
    }
private:
    mutable Iterator m_chunk;
};

}}}

#endif //DIPLOMA_CACHE_POLICIES_HPP
