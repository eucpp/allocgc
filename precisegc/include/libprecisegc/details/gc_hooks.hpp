#ifndef DIPLOMA_GC_HOOKS_HPP
#define DIPLOMA_GC_HOOKS_HPP

#include <memory>

#include <libprecisegc/details/gc_interface.hpp>
#include <libprecisegc/details/gc_strategy.hpp>
#include <libprecisegc/details/initiation_policy.hpp>
#include <libprecisegc/gc_stat.hpp>

namespace precisegc { namespace details {

void gc_initialize(std::unique_ptr<gc_strategy> strategy, std::unique_ptr<initiation_policy> init_policy);

void gc_register_root(gc_word* root);
void gc_deregister_root(gc_word* root);

bool gc_is_root(const gc_word* ptr);
bool gc_is_heap_ptr(const gc_word* ptr);

gc_alloc_response gc_allocate(size_t obj_size, size_t obj_cnt, const gc_type_meta* tmeta);

void gc_commit(const gc_alloc_response& ptr);

void gc_initiation_point(initiation_point_type ipoint, const gc_options& opt);

gc_info  gc_get_info();
gc_stat  gc_get_stats();

void gc_enable_print_stats();
void gc_disable_print_stats();

void gc_register_page(const byte* page, size_t size);
void gc_deregister_page(const byte* page, size_t size);

}}

#endif //DIPLOMA_GC_HOOKS_HPP
