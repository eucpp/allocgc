#include <cstdlib>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <cassert>
#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>

#include <liballocgc/details/utils/barrier.hpp>
#include <liballocgc/details/utils/scope_guard.hpp>
#include <liballocgc/details/utils/utility.hpp>
#include <liballocgc/details/utils/scoped_thread.hpp>

#ifdef BDW_GC
    #define GC_THREADS
    #include <gc/gc.h>
#endif

#ifdef PRECISE_GC_SERIAL
#include "liballocgc/liballocgc.hpp"
    using namespace allocgc;
    using namespace allocgc::serial;
#endif

#ifdef PRECISE_GC_CMS
#include "liballocgc/liballocgc.hpp"
    using namespace allocgc;
    using namespace allocgc::cms;
#endif

#include "../../common/macro.hpp"
#include "../../common/timer.hpp"

static const int threads_cnt        = 8; // std::thread::hardware_concurrency();
static const int nodes_per_thread   = 2048;

static const int node_size    = 64;
static const int lists_count  = 30;
static const int lists_length = threads_cnt * nodes_per_thread;

struct Node
{
    size_t x;
    ptr_t(Node) next;
//    char data[32];
};

struct List
{
    List()
        : head(nullptr)
        , length(0)
    {}

    List(ptr_in(Node) head_, size_t length_)
        : head(head_)
        , length(length_)
    {}

    ptr_t(Node) head;
    size_t length;
};

ptr_t(Node) advance(ptr_t(Node) node, size_t n)
{
    while (n > 0) {
        node = node->next;
        --n;
    }
    return node;
}

#if !(defined(PRECISE_GC_SERIAL) || defined(PRECISE_GC_CMS))
    template <typename Function, typename... Args>
    std::thread create_thread(Function&& f, Args&&... args)
    {
        return std::thread(std::forward<Function>(f), std::forward<Args>(args)...);
    };
#endif


static std::atomic<bool> done_flag{false};
static allocgc::details::utils::barrier tasks_ready_barrier{threads_cnt};
static allocgc::details::utils::barrier tasks_done_barrier{threads_cnt};

static std::vector<allocgc::details::utils::scoped_thread> threads{threads_cnt - 1};

List merge_sort(const List& list);

void thread_routine(const List& input, List& output)
{
    #ifdef BDW_GC
        GC_stack_base sb;
        GC_get_stack_base(&sb);
        GC_register_my_thread(&sb);
        auto guard = allocgc::details::utils::make_scope_guard([] { GC_unregister_my_thread(); });
    #endif

    while (true) {
        tasks_ready_barrier.wait();
        if (done_flag) {
            return;
        }

        output = merge_sort(input);

        tasks_done_barrier.wait();
    }
}

void init(const List* inputs, List* outputs)
{
    srand(time(nullptr));
    for (int i = 0; i < threads_cnt - 1; ++i) {
        threads[i] = create_thread(thread_routine, std::ref(inputs[i]), std::ref(outputs[i]));
    }
}

List merge(const List& fst, const List& snd)
{
    if (fst.length == 0 && snd.length == 0) {
        return List();
    }

    ptr_t(Node) res;
    ptr_t(Node) it1 = fst.head;
    ptr_t(Node) it2 = snd.head;
    ptr_t(Node) prev1 = it1;
    ptr_t(Node) prev2 = it2;
    size_t l1 = fst.length;
    size_t l2 = snd.length;
    size_t length = l1 + l2;

    if (it1->x < it2->x) {
        res = it1;
        it1 = it1->next;
        l1--;
    } else {
        res = it2;
        it2 = it2->next;
        l2--;
    }

    ptr_t(Node) dst = res;
    while (l1 > 0 && l2 > 0) {
        assert(it1 && it2);
        pin_t(Node) pIt1 = pin(it1);
        pin_t(Node) pIt2 = pin(it2);
        pin_t(Node) pDst = pin(dst);
        if (pIt1->x < pIt2->x) {
            pDst->next = it1;
            dst = pDst->next;
            prev1 = it1;
            it1 = pIt1->next;
            --l1;
        } else {
            pDst->next = it2;
            dst = pDst->next;
            prev2 = it2;
            it2 = pIt2->next;
            --l2;
        }
    }

    if (l1 > 0) {
        dst->next = it1;
    }
    if (l2 > 0) {
        dst->next = it2;
    }

    return List(res, length);
}

List merge_sort(const List& list)
{
    if (list.length == 0) {
        return List();
    } else if (list.length == 1) {
        ptr_t(Node) node = new_(Node);
        node->x = list.head->x;
        set_null(node->next);
        return List(node, 1);
    }

    size_t m = list.length / 2;
    ptr_t(Node) mid = advance(list.head, m);

    List lpart(list.head, m);
    List rpart(mid, list.length - m);

    List lsorted = merge_sort(lpart);
    List rsorted = merge_sort(rpart);

    return merge(lsorted, rsorted);
}

List parallel_merge_sort(const List& list, List* input, List* output)
{
    ptr_t(Node) it = list.head;
    for (int i = 0; i < threads_cnt - 1; ++i) {
        input[i] = List(it, nodes_per_thread);
        it = ::advance(it, nodes_per_thread);
    }

    tasks_ready_barrier.wait();

    List lst(it, nodes_per_thread);
    List sorted = merge_sort(lst);

    tasks_done_barrier.wait();

    for (int i = 0; i < threads_cnt - 1; ++i) {
        sorted = merge(output[i], sorted);
    }

    return sorted;
}

List create_list(size_t n, int mod)
{
    if (n == 0) {
        return List();
    }
    unsigned int i = 0;
    size_t length = n;
    ptr_t(Node) head = new_(Node);
    ptr_t(Node) it = head;
    head->x = rand() % mod;
    --n;
    while (n > 0) {
        it->next = new_(Node);
        it = it->next;
        it->x = i++ % mod;
        --n;
    }
    return List(head, length);
}

void clear_list(List& list)
{
    #ifdef NO_GC
        ptr_t(Node) it = list.head;
        while (list.length > 0) {
            ptr_t(Node) next = it->next;
            delete_(it);
            it = next;
            --list.length;
        }
    #endif
    set_null(list.head);
    list.length = 0;
}

void routine(List* input, List* output)
{
    List list = create_list(lists_length, 16);
    List sorted = parallel_merge_sort(list, input, output);

    ptr_t(Node) it = sorted.head;
    assert(sorted.length == lists_length);
    for (size_t i = 0; i < sorted.length - 1; ++i, it = it->next) {
        assert(it->x <= it->next->x);
    }

    clear_list(list);
    clear_list(sorted);
}

int main(int argc, const char* argv[])
{
    bool compacting_flag = false;
    bool incremental_flag = false;
    for (int i = 1; i < argc; ++i) {
        auto arg = std::string(argv[i]);
        if (arg == "--incremental") {
            incremental_flag = true;
        } else if (arg == "--compacting") {
            compacting_flag = true;
        }
    }

    #if defined(PRECISE_GC_SERIAL) || defined(PRECISE_GC_CMS)
        register_main_thread();
        set_threads_available(1);
//        enable_logging(gc_loglevel::DEBUG);
    #elif defined(BDW_GC)
        GC_INIT();
        GC_allow_register_threads();
        if (incremental_flag) {
            GC_enable_incremental();
        }
    #endif

    List input_lists[threads_cnt - 1];
    List output_lists[threads_cnt - 1];

    init(input_lists, output_lists);
    auto guard = allocgc::details::utils::make_scope_guard([&done_flag, &tasks_ready_barrier] {
        done_flag = true;
        tasks_ready_barrier.wait();
    });

    std::cout << "Sorting " << lists_count << " lists with length " << lists_length << std::endl;
    std::cout << "Size of each list " << lists_length * sizeof(Node) << " b" << std::endl;
    std::cout << "Total memory usage " << 2 * lists_length * sizeof(Node) * lists_count << " b" << std::endl;

    timer tm;
    for (int i = 0; i < lists_count; ++i) {
        if ((i+1) % 32 == 0) {
            std::cout << (i+1) * 100 / lists_count << "%" << std::endl;
        }
        routine(input_lists, output_lists);
    }

    std::cout << "Completed in " << tm.elapsed<std::chrono::milliseconds>() << " ms" << std::endl;
    #if defined(BDW_GC)
        std::cout << "Completed " << GC_get_gc_no() << " collections" << std::endl;
        std::cout << "Heap size is " << GC_get_heap_size() << std::endl;
    #elif defined(PRECISE_GC_SERIAL) || defined(PRECISE_GC_CMS)
        gc_stat stat = stats();
        std::cout << "Completed " << stat.gc_count << " collections" << std::endl;
        std::cout << "Time spent in gc " << std::chrono::duration_cast<std::chrono::milliseconds>(stat.gc_time).count() << " ms" << std::endl;
        std::cout << "Average pause time " << std::chrono::duration_cast<std::chrono::microseconds>(stat.gc_time / stat.gc_count).count() << " us" << std::endl;
    #endif

    done_flag = true;
    tasks_ready_barrier.wait();
    guard.commit();
    for (auto& thread: threads) {
        thread.join();
    }
}

