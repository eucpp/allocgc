#pragma once

#include <cstdlib>

#include "object.h"

namespace _GC_ {

    class base_meta {
    public:
        size_t *shell; /**< pointer on the box(meta info struct for storing offsets) of object */
        size_t count;  /**< array count */
    };

    bool is_heap_pointer(void *);

    void* gcmalloc(size_t, void *, size_t);

    void set_meta_after_gcmalloc(void* ptr, void* clMeta);
    Object * get_object_header(void * ptr);
    base_meta * get_meta_inf (void * ptr);

    int mark_object(void *ptr, bool mark_pin);
    int get_object_mark(void *ptr, bool mark_pin);

    bool mark_after_overflow(void);
}