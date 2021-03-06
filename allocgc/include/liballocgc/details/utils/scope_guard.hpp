#ifndef ALLOCGC_SCOPE_GUARD_HPP
#define ALLOCGC_SCOPE_GUARD_HPP

#include <type_traits>
#include <utility>

#include <liballocgc/details/utils/utility.hpp>

namespace allocgc { namespace details { namespace utils {

template <typename Callable>
class scope_guard : private utils::noncopyable
{
public:
    template<typename C>
    explicit scope_guard(C&& callable)
        : m_callable(std::forward<C>(callable))
        , m_commited(false)
    {}

    scope_guard(scope_guard&& other)
        : m_callable(std::move(other.m_callable))
        , m_commited(other.m_commited)
    {
        other.m_commited = true;
    }

    ~scope_guard()
    {
        if (!m_commited) {
            m_callable();
        }
    }

    void commit()
    {
        m_commited = true;
    }
private:
    Callable m_callable;
    bool m_commited;
};

template <typename Callable>
auto make_scope_guard(Callable&& callable)
    -> scope_guard<typename std::decay<Callable>::type>
{
    return scope_guard<typename std::decay<Callable>::type>(callable);
}

}}}

#endif //ALLOCGC_SCOPE_GUARD_HPP
