// Copyright (c) 2009 - Mozy, Inc.

#include "fibersynchronization.h"

#include "assert.h"
#include "fiber.h"
#include "scheduler.h"

namespace Mordor {

FiberMutex::~FiberMutex()
{
#ifndef NDEBUG
    boost::mutex::scoped_lock scopeLock(m_mutex);
    MORDOR_ASSERT(!m_owner);
    MORDOR_ASSERT(m_waiters.empty());
#endif
}

void
FiberMutex::lock()
{
    MORDOR_ASSERT(Scheduler::getThis());
    {
        boost::mutex::scoped_lock scopeLock(m_mutex);
        MORDOR_ASSERT(m_owner != Fiber::getThis());
        MORDOR_ASSERT_PERF(std::find(m_waiters.begin(), m_waiters.end(),
            std::make_pair(Scheduler::getThis(), Fiber::getThis()))
            == m_waiters.end());
        if (!m_owner) {
            m_owner = Fiber::getThis();
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::getThis(),
            Fiber::getThis()));
    }
    Scheduler::yieldTo();
#ifndef NDEBUG
    boost::mutex::scoped_lock scopeLock(m_mutex);
    MORDOR_ASSERT(m_owner == Fiber::getThis());
    MORDOR_ASSERT_PERF(std::find(m_waiters.begin(), m_waiters.end(),
            std::make_pair(Scheduler::getThis(), Fiber::getThis()))
            == m_waiters.end());
#endif
}

void
FiberMutex::unlock()
{
    boost::mutex::scoped_lock lock(m_mutex);
    unlockNoLock();
}

bool
FiberMutex::unlockIfNotUnique()
{
    boost::mutex::scoped_lock lock(m_mutex);
    MORDOR_ASSERT(m_owner == Fiber::getThis());
    if (!m_waiters.empty()) {
        unlockNoLock();
        return true;
    }
    return false;
}

void
FiberMutex::unlockNoLock()
{
    MORDOR_ASSERT(m_owner == Fiber::getThis());
    m_owner.reset();
    if (!m_waiters.empty()) {
        std::pair<Scheduler *, Fiber::ptr> next = m_waiters.front();
        m_waiters.pop_front();
        m_owner = next.second;
        next.first->schedule(next.second);
    }
}

FiberSemaphore::FiberSemaphore(size_t initialConcurrency)
    : m_concurrency(initialConcurrency)
{}

FiberSemaphore::~FiberSemaphore()
{
#ifndef NDEBUG
    boost::mutex::scoped_lock scopeLock(m_mutex);
    MORDOR_ASSERT(m_waiters.empty());
#endif
}

void
FiberSemaphore::wait()
{
    MORDOR_ASSERT(Scheduler::getThis());
    {
        boost::mutex::scoped_lock scopeLock(m_mutex);
        MORDOR_ASSERT_PERF(std::find(m_waiters.begin(), m_waiters.end(),
            std::make_pair(Scheduler::getThis(), Fiber::getThis()))
            == m_waiters.end());
        if (m_concurrency > 0u) {
            --m_concurrency;
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::getThis(),
            Fiber::getThis()));
    }
    Scheduler::yieldTo();
#ifndef NDEBUG_PERF
    boost::mutex::scoped_lock scopeLock(m_mutex);
    MORDOR_ASSERT_PERF(std::find(m_waiters.begin(), m_waiters.end(),
            std::make_pair(Scheduler::getThis(), Fiber::getThis()))
            == m_waiters.end());
#endif
}

void
FiberSemaphore::notify()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (!m_waiters.empty()) {
        std::pair<Scheduler *, Fiber::ptr> next = m_waiters.front();
        m_waiters.pop_front();
        next.first->schedule(next.second);
    } else {
        ++m_concurrency;
    }
}

FiberCondition::~FiberCondition()
{
#ifndef NDEBUG
    boost::mutex::scoped_lock lock(m_mutex);
    MORDOR_ASSERT(m_waiters.empty());
#endif
}

void
FiberCondition::wait()
{
    MORDOR_ASSERT(Scheduler::getThis());
    {
        boost::mutex::scoped_lock lock(m_mutex);
        boost::mutex::scoped_lock lock2(m_fiberMutex.m_mutex);
        MORDOR_ASSERT(m_fiberMutex.m_owner == Fiber::getThis());
        m_waiters.push_back(std::make_pair(Scheduler::getThis(),
            Fiber::getThis()));
        m_fiberMutex.unlockNoLock();
    }
    Scheduler::yieldTo();
#ifndef NDEBUG
    boost::mutex::scoped_lock lock2(m_fiberMutex.m_mutex);
    MORDOR_ASSERT(m_fiberMutex.m_owner == Fiber::getThis());
#endif
}

void
FiberCondition::signal()
{
    std::pair<Scheduler *, Fiber::ptr> next;
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_waiters.empty())
            return;
        next = m_waiters.front();
        m_waiters.pop_front();
    }
    boost::mutex::scoped_lock lock2(m_fiberMutex.m_mutex);
    MORDOR_ASSERT(m_fiberMutex.m_owner != next.second);
    MORDOR_ASSERT_PERF(std::find(m_fiberMutex.m_waiters.begin(),
        m_fiberMutex.m_waiters.end(), next)
        == m_fiberMutex.m_waiters.end());
    if (!m_fiberMutex.m_owner) {
        m_fiberMutex.m_owner = next.second;
        next.first->schedule(next.second);
    } else {
        m_fiberMutex.m_waiters.push_back(next);
    }
}

void
FiberCondition::broadcast()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_waiters.empty())
        return;
    boost::mutex::scoped_lock lock2(m_fiberMutex.m_mutex);

    std::list<std::pair<Scheduler *, Fiber::ptr> >::iterator it;
    for (it = m_waiters.begin();
        it != m_waiters.end();
        ++it) {
        std::pair<Scheduler *, Fiber::ptr> &next = *it;
        MORDOR_ASSERT(m_fiberMutex.m_owner != next.second);
        MORDOR_ASSERT_PERF(std::find(m_fiberMutex.m_waiters.begin(),
            m_fiberMutex.m_waiters.end(), next)
            == m_fiberMutex.m_waiters.end());
        if (!m_fiberMutex.m_owner) {
            m_fiberMutex.m_owner = next.second;
            next.first->schedule(next.second);
        } else {
            m_fiberMutex.m_waiters.push_back(next);
        }
    }
    m_waiters.clear();
}


FiberEvent::~FiberEvent()
{
#ifndef NDEBUG
    boost::mutex::scoped_lock lock(m_mutex);
    MORDOR_ASSERT(m_waiters.empty());
#endif
}

void
FiberEvent::wait()
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (m_signalled) {
            if (m_autoReset)
                m_signalled = false;
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::getThis(),
            Fiber::getThis()));
    }
    Scheduler::yieldTo();
}

void
FiberEvent::set()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (!m_autoReset) {
        m_signalled = true;

        std::list<std::pair<Scheduler *, Fiber::ptr> >::iterator it;
        for (it = m_waiters.begin();
            it != m_waiters.end();
            ++it) {
            it->first->schedule(it->second);
        }
        m_waiters.clear();
        return;
    }
    if (m_waiters.empty()) {
        m_signalled = true;
        return;
    }
    m_waiters.front().first->schedule(m_waiters.front().second);
    m_waiters.pop_front();
}

void
FiberEvent::reset()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_signalled = false;
}

}
