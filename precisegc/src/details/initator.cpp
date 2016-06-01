#include "initator.hpp"

#include <atomic>
#include <stdexcept>
#include <utility>

#include "gc_heap.h"
#include "logging.h"

namespace precisegc { namespace details {

initator::initator(serial_gc_interface* gc, std::unique_ptr<initation_policy>&& policy)
    : m_gc(gc)
    , m_policy(std::move(policy))
{
    assert(m_gc);
    assert(m_policy);
}

void initator::initation_point(initation_point_type ipoint)
{
    if (ipoint == initation_point_type::USER_REQUEST) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_gc->gc();
        m_policy->update(m_gc->stat(), ipoint);
    }
    if (m_policy->check(m_gc->stat(), ipoint)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        gc_stat stat = m_gc->stat();
        if (m_policy->check(stat, ipoint)) {
            m_gc->gc();
            m_policy->update(m_gc->stat(), ipoint);
        }
    }
}

initation_policy* initator::get_policy() const
{
    return m_policy.get();
}

incremental_initator::incremental_initator(incremental_gc_interface* gc,
                                           std::unique_ptr<incremental_initation_policy>&& policy)
    : m_gc(gc)
    , m_policy(std::move(policy))
{
    assert(m_gc);
    assert(m_policy);
}

void incremental_initator::initation_point(initation_point_type ipoint)
{
    if (ipoint == initation_point_type::USER_REQUEST) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_gc->gc();
        m_policy->update(m_gc->stat(), ipoint);
    }
    gc_phase phase = m_policy->check(m_gc->stat(), ipoint);
    if (phase != gc_phase::IDLING) {
        std::lock_guard<std::mutex> lock(m_mutex);
        gc_stat stat = m_gc->stat();
        phase = m_policy->check(stat, ipoint);
        gc_phase curr_phase = m_gc->phase();
        if (phase == gc_phase::MARKING && curr_phase == gc_phase::IDLING) {
            incremental_gc_ops ops;
            ops.phase = phase;
            ops.concurrent_flag = stat.support_concurrent_mark;
            ops.threads_num = 1;
            m_gc->incremental_gc(ops);
        } else if (phase == gc_phase::SWEEPING
                   && (curr_phase == gc_phase::IDLING || curr_phase == gc_phase::MARKING)) {
            incremental_gc_ops ops;
            ops.phase = phase;
            ops.concurrent_flag = stat.support_concurrent_sweep;
            ops.threads_num = 1;
            m_gc->incremental_gc(ops);
        }
        m_policy->update(m_gc->stat(), ipoint);
    }
}

incremental_initation_policy* incremental_initator::get_policy() const
{
    return m_policy.get();
}

}}