#include "segregated_list.h"

#include <new>

#include "os.h"
#include "object.h"

namespace precisegc { namespace details {

void* segregated_list_element::operator new(size_t )
{
    return memory_allocate(SEGREGATED_STORAGE_ELEMENT_SIZE);
}

void segregated_list_element::operator delete(void* ptr)
{
    memory_deallocate(ptr, SEGREGATED_STORAGE_ELEMENT_SIZE);
}

segregated_list_element::segregated_list_element(size_t obj_size,
                                                 segregated_list_element *next,
                                                 segregated_list_element *prev)
{
    for (size_t i = 0; i < PAGES_PER_SEGREGATED_STORAGE_ELEMENT; ++i) {
        m_pages[i].initialize_page(obj_size);
    }
    m_header = {next, prev, 0};
}

allocate_result segregated_list_element::allocate()
{
    assert(is_memory_available());
    size_t page_id = last_used_page();
    if (!m_pages[page_id].is_memory_available()) {
        for (size_t i = 0; i <= LAST_PAGE_ID; ++i) {
            if (m_pages[i].is_memory_available()) {
                page_id = i;
                set_last_used_page(i);
            }
        }
    }
    void* mem = m_pages[page_id].allocate();
    return std::make_pair(mem, &m_pages[page_id]);
}

void segregated_list_element::clear_mark_bits() noexcept
{
    for (size_t i = 0; i < LAST_PAGE_ID; ++i) {
        m_pages[i].clear_mark_bits();
    }
}

void segregated_list_element::clear_pin_bits() noexcept
{
    for (size_t i = 0; i < LAST_PAGE_ID; ++i) {
        m_pages[i].clear_pin_bits();
    }
}

page_descriptor& segregated_list_element::get_page_descriptor(size_t ind)
{
    assert(ind < PAGES_PER_SEGREGATED_STORAGE_ELEMENT);
    return m_pages[ind];
}

size_t segregated_list_element::last_used_page() const noexcept
{
    return m_header.m_last_used_page;
}

void segregated_list_element::set_last_used_page(size_t id) noexcept
{
    m_header.m_last_used_page = id;
}

bool segregated_list_element::is_memory_available() const noexcept
{
    if (m_pages[last_used_page()].is_memory_available()) {
        return true;
    }
    bool res = false;
    for (size_t i = 0; i <= LAST_PAGE_ID; ++i) {
        res |= m_pages[i].is_memory_available();
    }
    return res;
}

segregated_list_element* segregated_list_element::get_next() const noexcept
{
    return m_header.m_next;
}

void segregated_list_element::set_next(segregated_list_element *next) noexcept
{
    m_header.m_next = next;
}

segregated_list_element* segregated_list_element::get_prev() const noexcept
{
    return m_header.m_prev;
}

void segregated_list_element::set_prev(segregated_list_element *prev) noexcept
{
    m_header.m_prev = prev;
}

segregated_list::segregated_list()
    : segregated_list(0)
{}

segregated_list::segregated_list(size_t alloc_size)
    : m_alloc_size(alloc_size)
    , m_first(nullptr)
    , m_last(nullptr)
{}

segregated_list::~segregated_list()
{
    segregated_list_element* sle = m_first;
    while (sle) {
        segregated_list_element* next = sle->get_next();
        delete sle;
        sle = next;
    }
}

allocate_result segregated_list::allocate()
{
    if (!m_last) {
        m_first = new segregated_list_element(m_alloc_size);
        m_last = m_first;
    }
    if (!m_last->is_memory_available()) {
        segregated_list_element* new_sle = new segregated_list_element(m_alloc_size, nullptr, m_last);
        m_last->set_next(new_sle);
        m_last = new_sle;
    }
    return m_last->allocate();
}

segregated_list::iterator segregated_list::begin() noexcept
{
    if (!m_first) {
        return iterator(nullptr, 0, page_descriptor::iterator());
    }
    size_t i = 0;
    segregated_list_element* sle = m_first;
    auto page_begin = m_first->get_page_descriptor(0).begin();
    auto page_end = m_first->get_page_descriptor(0).end();
    while (page_begin == page_end) {
        if (i == LAST_PAGE_ID) {
            i = 0;
            sle = sle->get_next();
            if (!sle) {
                break;
            }
        } else {
            ++i;
        }
        page_begin = sle->get_page_descriptor(i).begin();
        page_end = sle->get_page_descriptor(i).end();
    }
    return iterator(m_first, 0, page_begin);
}

segregated_list::iterator segregated_list::end() noexcept
{
    if (!m_first) {
        return iterator(nullptr, 0, page_descriptor::iterator());
    }
    size_t page_id = LAST_PAGE_ID;
    return iterator(m_last, page_id, m_last->get_page_descriptor(page_id).end());
}


void segregated_list::clear_mark_bits() noexcept
{
    if (!m_first) {
        return;
    }
    segregated_list_element* sle = m_first;
    while (sle) {
        sle->clear_mark_bits();
        sle = sle->get_next();
    }
}

void segregated_list::clear_pin_bits() noexcept
{
    if (!m_first) {
        return;
    }
    segregated_list_element* sle = m_first;
    while (sle) {
        sle->clear_pin_bits();
        sle = sle->get_next();
    }
}

void segregated_list::set_alloc_size(size_t size) noexcept
{
    assert(m_alloc_size == 0);
    m_alloc_size = size;
}

size_t segregated_list::alloc_size() const noexcept
{
    return m_alloc_size;
}

segregated_list::iterator::iterator(segregated_list_element* sle,
                                    size_t pd_ind,
                                    page_descriptor::iterator pd_itr) noexcept
    : m_sle(sle)
    , m_pd_ind(pd_ind)
    , m_pd_itr(pd_itr)
{}

void* const segregated_list::iterator::operator*() const noexcept
{
    return *m_pd_itr;
}

void segregated_list::iterator::increment() noexcept
{
    assert(m_sle);
    ++m_pd_itr;
    auto page_end = m_sle->get_page_descriptor(m_pd_ind).end();
    while (m_pd_itr == page_end) {
        if (m_pd_ind == LAST_PAGE_ID) {
            segregated_list_element* next_sle = m_sle->get_next();
            if (next_sle) {
                m_pd_ind = 0;
                m_sle = next_sle;
            } else {
                return;
            }
        } else {
            ++m_pd_ind;
        }
        m_pd_itr = m_sle->get_page_descriptor(m_pd_ind).begin();
        page_end = m_sle->get_page_descriptor(m_pd_ind).end();
    }
}

void segregated_list::iterator::decrement() noexcept
{
    assert(m_sle);
    while (m_pd_itr == m_sle->get_page_descriptor(m_pd_ind).begin()) {
        if (m_pd_ind == 0) {
            segregated_list_element* prev_sle = m_sle->get_prev();
            assert(prev_sle);
            m_sle = prev_sle;
            m_pd_ind = LAST_PAGE_ID;
        } else {
            --m_pd_ind;
        }
        m_pd_itr = m_sle->get_page_descriptor(m_pd_ind).end();
    }
    --m_pd_itr;
}

bool segregated_list::iterator::equal(const iterator &other) const noexcept
{
    return m_pd_itr == other.m_pd_itr;
}

void segregated_list::iterator::deallocate() noexcept
{
    m_pd_itr.set_deallocated();
}

bool segregated_list::iterator::is_marked() const noexcept
{
    return m_pd_itr.is_marked();
}

bool segregated_list::iterator::is_pinned() const noexcept
{
    return m_pd_itr.is_pinned();
}

void segregated_list::iterator::set_marked(bool marked) noexcept
{
    m_pd_itr.set_marked(marked);
}

void segregated_list::iterator::set_pinned(bool pinned) noexcept
{
    m_pd_itr.set_pinned(pinned);
}

}}