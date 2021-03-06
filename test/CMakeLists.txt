enable_testing()
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})

set(allocgc_TESTS
        include/time_util.h
        include/memory_descriptor_mock.h
        include/test_chunk.h

        gc_test.cpp
        gc_inside_method_test.cpp
        details/gc_type_meta_factory_test.cpp
        details/gc_heap_test.cpp
        gc_new_test.cpp
        details/gc_untyped_ptr_test.cpp
        gc_environment.cpp
        details/collectors/index_tree_test.cpp
        details/allocators/pool_chunk_test.cpp
        details/allocators/bucket_allocator_test.cpp
        details/allocators/pow2_bucket_policy_test.cpp
        include/page_ptr.h
        details/allocators/managed_pool_chunk_test.cpp
        include/rand_util.h
        gc_ptr_test.cpp
        details/threads/pending_call_test.cpp
        details/threads/pin_set_test.cpp
        details/threads/signal_test.cpp
        details/threads/ass_sync_test.cpp
        details/threads/stw_manager_test.cpp
        details/threads/managed_thread_test.cpp
        details/threads/thread_manager_test.cpp
        include/test_descriptor.hpp

        details/utils/flattened_range_test.cpp
        details/utils/bitset_test.cpp
        details/garbage_collector_test.cpp
        include/serial_gc_mock.hpp
        include/initiation_policy_mock.hpp
        include/incremental_gc_mock.hpp
        details/utils/dynarray_test.cpp
        details/utils/base_offset_test.cpp
        details/utils/static_thread_pool_test.cpp
        details/collectors/marker_test.cpp
        details/utils/barrier_test.cpp
        details/gc_handle_test.cpp
        details/allocators/allocators_test.cpp
        details/threads/pin_stack_test.cpp
        details/threads/stack_bitmap_test.cpp
        details/compacting/compactor_test.cpp
        include/test_forwarding.hpp
        details/compacting/fix_ptrs_test.cpp
        details/allocators/gc_lo_allocator_test.cpp
        details/allocators/gc_pool_allocator_test.cpp
        details/allocators/gc_box_test.cpp
        details/collectors/memory_index_test.cpp include/utils.hpp graph.cpp)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/allocgc/include)

find_package(Threads REQUIRED)

find_library(GMOCK_LIB NAMES libgmock.a
        PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        )

find_library(GMOCK_MAIN_LIB NAMES libgmock_main.a
        PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        )


add_executable(allocgc_test ${allocgc_TESTS})
target_link_libraries(allocgc_test ${GMOCK_LIB})
target_link_libraries(allocgc_test ${GMOCK_MAIN_LIB})
target_link_libraries(allocgc_test ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(allocgc_test liballocgc)

add_test(all_tests allocgc_test)