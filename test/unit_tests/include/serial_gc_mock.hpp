#ifndef DIPLOMA_SERIAL_GC_MOCK_HPP
#define DIPLOMA_SERIAL_GC_MOCK_HPP

#include <gmock/gmock.h>

#include <libprecisegc/details/gc_interface.hpp>

class serial_gc_mock : public precisegc::details::serial_gc_interface
{
    typedef precisegc::details::byte byte;
    typedef precisegc::details::atomic_byte_ptr atomic_byte_ptr;
    typedef precisegc::details::managed_ptr managed_ptr;
    typedef precisegc::details::gc_stat gc_stat;
public:
    MOCK_METHOD1(allocate, managed_ptr(size_t));
    MOCK_METHOD1(rbarrier, byte*(const atomic_byte_ptr&));
    MOCK_METHOD2(wbarrier, void(atomic_byte_ptr&, const atomic_byte_ptr&));
    MOCK_CONST_METHOD0(stat, gc_stat(void));
    MOCK_METHOD0(gc, void(void));
};

#endif //DIPLOMA_SERIAL_GC_MOCK_HPP
