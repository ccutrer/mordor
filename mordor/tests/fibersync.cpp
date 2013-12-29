// Copyright (c) 2009 - Mozy, Inc.


#include "mordor/fiber.h"
#include "mordor/fibersynchronization.h"
#include "mordor/test/test.h"
#include "mordor/workerpool.h"

using namespace Mordor;

MORDOR_UNITTEST(FiberMutex, basic)
{
    WorkerPool pool;
    FiberMutex mutex;

    FiberMutex::ScopedLock lock(mutex);
}


static void contentionFiber(int fiberNo, FiberMutex &mutex, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(++sequence, fiberNo);
    FiberMutex::ScopedLock lock(mutex);
    MORDOR_TEST_ASSERT_EQUAL(++sequence, fiberNo + 3 + 1);
}

MORDOR_UNITTEST(FiberMutex, contention)
{
    WorkerPool pool;
    FiberMutex mutex;
    int sequence = 0;
    Fiber::ptr fiber1(new Fiber(NULL)), fiber2(new Fiber(NULL)),
        fiber3(new Fiber(NULL));
    fiber1->reset(std::bind(&contentionFiber, 1, std::ref(mutex),
        std::ref(sequence)));
    fiber2->reset(std::bind(&contentionFiber, 2, std::ref(mutex),
        std::ref(sequence)));
    fiber3->reset(std::bind(&contentionFiber, 3, std::ref(mutex),
        std::ref(sequence)));

    {
        FiberMutex::ScopedLock lock(mutex);
        pool.schedule(fiber1);
        pool.schedule(fiber2);
        pool.schedule(fiber3);
        pool.dispatch();
        MORDOR_TEST_ASSERT_EQUAL(++sequence, 4);
    }
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 8);
}

#ifndef NDEBUG
MORDOR_UNITTEST(FiberMutex, notRecursive)
{
    WorkerPool pool;
    FiberMutex mutex;

    FiberMutex::ScopedLock lock(mutex);
    MORDOR_TEST_ASSERT_ASSERTED(mutex.lock());
}
#endif

static void signalMe(FiberCondition &condition, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 2);
    condition.signal();
}

MORDOR_UNITTEST(FiberCondition, signal)
{
    int sequence = 0;
    WorkerPool pool;
    FiberMutex mutex;
    FiberCondition condition(mutex);

    FiberMutex::ScopedLock lock(mutex);
    pool.schedule(std::bind(&signalMe, std::ref(condition),
        std::ref(sequence)));
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 1);
    condition.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 3);
}

static void waitOnMe(FiberCondition &condition, FiberMutex &mutex,
                     int &sequence, int expected)
{
    MORDOR_TEST_ASSERT_EQUAL(++sequence, expected * 2);
    FiberMutex::ScopedLock lock(mutex);
    MORDOR_TEST_ASSERT_EQUAL(++sequence, expected * 2 + 1);
    condition.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, expected + 8);
}

MORDOR_UNITTEST(FiberCondition, broadcast)
{
    int sequence = 0;
    WorkerPool pool;
    FiberMutex mutex;
    FiberCondition condition(mutex);

    pool.schedule(std::bind(&waitOnMe, std::ref(condition),
        std::ref(mutex), std::ref(sequence), 1));
    pool.schedule(std::bind(&waitOnMe, std::ref(condition),
        std::ref(mutex), std::ref(sequence), 2));
    pool.schedule(std::bind(&waitOnMe, std::ref(condition),
        std::ref(mutex), std::ref(sequence), 3));
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 1);
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 8);
    condition.broadcast();
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 12);
}

static void signalMe2(FiberEvent &event, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 2);
    event.set();
}

MORDOR_UNITTEST(FiberEvent, autoReset)
{
    int sequence = 0;
    WorkerPool pool;
    FiberEvent event;

    pool.schedule(std::bind(&signalMe2, std::ref(event),
        std::ref(sequence)));
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 1);
    event.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 3);
    event.set();
    event.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 4);
}

MORDOR_UNITTEST(FiberEvent, manualReset)
{
    int sequence = 0;
    WorkerPool pool;
    FiberEvent event(false);

    pool.schedule(std::bind(&signalMe2, std::ref(event),
        std::ref(sequence)));
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 1);
    event.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 3);
    // It's manual reset; you can wait as many times as you want until it's
    // reset
    event.wait();
    event.wait();
}

static void waitOnMe2(FiberEvent &event, int &sequence, int expected)
{
    MORDOR_TEST_ASSERT_EQUAL(++sequence, expected + 1);
    event.wait();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, expected + 5);
}

MORDOR_UNITTEST(FiberEvent, manualResetMultiple)
{
    int sequence = 0;
    WorkerPool pool;
    FiberEvent event(false);

    pool.schedule(std::bind(&waitOnMe2, std::ref(event),
        std::ref(sequence), 1));
    pool.schedule(std::bind(&waitOnMe2, std::ref(event),
        std::ref(sequence), 2));
    pool.schedule(std::bind(&waitOnMe2, std::ref(event),
        std::ref(sequence), 3));
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 1);
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 5);
    event.set();
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(++sequence, 9);
    // It's manual reset; you can wait as many times as you want until it's
    // reset
    event.wait();
    event.wait();
}

static void lockIt(FiberMutex &mutex)
{
    FiberMutex::ScopedLock lock(mutex);
}

MORDOR_UNITTEST(FiberMutex, unlockUnique)
{
    WorkerPool pool;
    FiberMutex mutex;

    FiberMutex::ScopedLock lock(mutex);
    MORDOR_TEST_ASSERT(!lock.unlockIfNotUnique());
    pool.schedule(std::bind(&lockIt, std::ref(mutex)));
    Scheduler::yield();
    MORDOR_TEST_ASSERT(lock.unlockIfNotUnique());
    pool.dispatch();
}
