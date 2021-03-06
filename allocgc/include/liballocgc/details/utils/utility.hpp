#ifndef ALLOCGC_UTILITY_H
#define ALLOCGC_UTILITY_H

#include <type_traits>

namespace allocgc { namespace details { namespace utils {

template <typename... >
using void_t = void;

class noncopyable
{
public:
    noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

    noncopyable(noncopyable&&) = default;
    noncopyable& operator=(noncopyable&&) = default;
};

class nonmovable
{
public:
    nonmovable() = default;

    nonmovable(const nonmovable&) = default;
    nonmovable& operator=(const nonmovable&) = default;

    nonmovable(nonmovable&&) = delete;
    nonmovable& operator=(nonmovable&&) = delete;
};

class nonconstructible : private utils::noncopyable, private utils::nonmovable
{
public:
    nonconstructible() = delete;
};

template <typename... Bases>
class ebo : private Bases...
{
public:
    template <typename Base>
    Base& get_base()
    {
        static_assert(std::is_base_of<Base, ebo>::value, "Inappropriate base class");
        return (*this);
    }

    template <typename Base>
    const Base& get_base() const
    {
        static_assert(std::is_base_of<Base, ebo>::value, "Inappropriate base class");
        return (*this);
    }
};

}}}

#endif //ALLOCGC_UTILITY_H
