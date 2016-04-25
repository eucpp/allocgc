#include "gc_initator.h"

#include <atomic>

#include "gc_heap.h"
#include "gc_garbage_collector.h"
#include "logging.h"

namespace precisegc { namespace details {

static size_t mem_lower_bound = 0;
static size_t mem_upper_bound = 0;
static std::atomic<size_t> alloc_ticks(0);

static double b_to_mb(size_t size)
{
    return size / (1024.0 * 1024.0);
}

void init_initator(size_t lower_bound, size_t upper_bound)
{
    mem_lower_bound = lower_bound;
    mem_upper_bound = upper_bound;
}

void initate_gc()
{
    ++alloc_ticks;
    if (alloc_ticks < 3000) {
        return;
    }
    gc_heap& heap = gc_heap::instance();
    gc_garbage_collector& garbage_collector = gc_garbage_collector::instance();
    size_t mem = heap.size();
    if (mem > mem_lower_bound ) {
        logging::debug() << "Heap size exceeded " << b_to_mb(mem_lower_bound);
        garbage_collector.start_marking();
    }
    if (mem > mem_upper_bound) {
        logging::debug() << "Heap size exceeded " << b_to_mb(mem_upper_bound);
        alloc_ticks.store(0);
        garbage_collector.start_compacting();
    }
}

}}