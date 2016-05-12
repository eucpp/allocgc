#include <nonius/nonius.h++>

#include "libprecisegc/details/sigmask_signal_lock.h"
#include "libprecisegc/details/gc_pause.h"

using namespace precisegc::details;

//NONIUS_BENCHMARK("sigmask_signal_lock", [](nonius::chronometer meter)
//{
//    meter.measure([] {
//        sigmask_gc_signal_lock::lock();
//        return sigmask_gc_signal_lock::unlock();
//    });
//});
//
//NONIUS_BENCHMARK("flag_signal_lock", [](nonius::chronometer meter)
//{
//    meter.measure([] {
//        flag_gc_signal_lock::lock();
//        return flag_gc_signal_lock::unlock();
//    });
//});
