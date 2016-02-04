#include <gtest/gtest.h>

#include <vector>

#include "libprecisegc/details/class_meta.h"

using namespace std;
using namespace precisegc::details;

struct type_1 {};
struct type_2 {};
struct type_3 {};

TEST(class_meta_test, test_is_initialized)
{
    EXPECT_FALSE(class_meta_provider<type_1>::is_created());
}

TEST(class_meta_test, test_create_meta)
{
    typedef class_meta_provider<type_2> provider;
    vector<size_t> offsets({1, 2, 3});
    provider::create_meta(offsets);
    EXPECT_TRUE(provider::is_created());
    EXPECT_EQ(sizeof(type_2), provider::get_meta().get_type_size());
    EXPECT_EQ(offsets, provider::get_meta().get_offsets());
}

static pthread_barrier_t g_barrier_1;

static void* thread_routine(void* arg)
{
    typedef class_meta_provider<type_3> provider;
    vector<size_t>* offsets = reinterpret_cast<vector<size_t>*>(arg);
    pthread_barrier_wait(&g_barrier_1);
    provider::create_meta(*offsets);
}

TEST(class_meta_test, test_threading)
{
    typedef class_meta_provider<type_3> provider;
    vector<size_t> offsets;
    for (int i = 0; i < 1000; ++i) {
        offsets.push_back(i);
    }
    const int THREAD_COUNT = 100;
    int b_init = pthread_barrier_init(&g_barrier_1, nullptr, THREAD_COUNT);
    ASSERT_EQ(0, b_init);

    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int res = pthread_create(&threads[i], nullptr, thread_routine, (void*) &offsets);
        ASSERT_EQ(0, res);
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        void* ret;
        int res = pthread_join(threads[i], &ret);
        ASSERT_EQ(0, res);
    }

    pthread_barrier_destroy(&g_barrier_1);

    EXPECT_TRUE(provider::is_created());
    EXPECT_EQ(sizeof(type_3), provider::get_meta().get_type_size());
    EXPECT_EQ(offsets, provider::get_meta().get_offsets());
}
