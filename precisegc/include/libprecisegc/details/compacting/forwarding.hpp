#ifndef DIPLOMA_FORWARDING_HPP
#define DIPLOMA_FORWARDING_HPP

#include <cstddef>

#include <libprecisegc/details/types.hpp>
#include <libprecisegc/details/gc_word.hpp>
#include <libprecisegc/details/utils/utility.hpp>

namespace precisegc { namespace details { namespace compacting {

class forwarding
{
public:
    forwarding() = default;
    forwarding(const forwarding&) = default;
    forwarding(forwarding&&) = default;

    void create(byte* from, byte* to);

    void forward(gc_word* handle) const;
private:
    static void move_cell(byte* from, byte* to, size_t size);
};

}}}

#endif //DIPLOMA_FORWARDING_HPP
