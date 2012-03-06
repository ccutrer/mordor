// Copyright (c) 2009 - Mozy, Inc.

#include <boost/bind.hpp>

#include "mordor/iomanager.h"
#include "mordor/sleep.h"
#include "mordor/streams/pipe.h"
#include "mordor/test/test.h"

using namespace Mordor;
using namespace Mordor::Test;

static void
singleTimer(int &sequence, int &expected)
{
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, expected);
}

MORDOR_UNITTEST(IOManager, singleTimer)
{
    int sequence = 0;
    IOManager manager;
    manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.dispatch();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 2);
}

MORDOR_UNITTEST(IOManager, laterTimer)
{
    int sequence = 0;
    IOManager manager;
    manager.registerTimer(100000, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.dispatch();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 2);
}

namespace {
class TickleAccessibleIOManager : public IOManager
{
public:
    void tickle() { IOManager::tickle(); }
};
}

MORDOR_UNITTEST(IOManager, lotsOfTickles)
{
    TickleAccessibleIOManager ioManager;
    // Need at least 64K iterations for Linux, but not more than 16K at once
    // for OS X
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 16000; ++j)
            ioManager.tickle();
        // Let the tickles drain
        sleep(ioManager, 250000ull);
    }
}

// Windows doesn't support asynchronous anonymous pipes yet
#ifndef WINDOWS
static void writeOne(Stream::ptr stream)
{
    MORDOR_TEST_ASSERT_EQUAL(stream->write("t", 1u), 1u);
}

MORDOR_UNITTEST(IOManager, rapidClose)
{
    IOManager ioManager(2);
    for (int i = 0; i < 10000; ++i) {
        std::pair<Stream::ptr, Stream::ptr> pipes = anonymousPipe(&ioManager);
        ioManager.schedule(boost::bind(&writeOne, pipes.second));
        char buffer;
        MORDOR_TEST_ASSERT_EQUAL(pipes.first->read(&buffer, 1u), 1u);
    }
}
#endif

static void
onTimer(unsigned long long &expiredTime, unsigned long long start)
{
    expiredTime = Mordor::TimerManager::now() - start;
    return;
}

static void
busyExecuting(unsigned long long time)
{ // execute at least `time' us and quit
    unsigned long long enter = Mordor::TimerManager::now();
    while (true) {
        if (Mordor::TimerManager::now() > enter + time)
            break;
        Scheduler::yield();
    }
}

MORDOR_UNITTEST(IOManager, busyWorkingTimerDelayed)
{
    IOManager manager(1, true, false);
    manager.schedule(boost::bind(busyExecuting, 300000));
    unsigned long long elapseBeforeExpire = 0;
    Timer::ptr timer = manager.registerTimer(50000,
        boost::bind(onTimer,
                    boost::ref(elapseBeforeExpire),
                    Mordor::TimerManager::now()));
    manager.dispatch();
    // timer will not executed until busyExecuting() done
    MORDOR_TEST_ASSERT_GREATER_THAN(elapseBeforeExpire, 300000U);
}

MORDOR_UNITTEST(IOManager, busyWorkingNoTimerDelay)
{
    IOManager manager(1, true, true);
    manager.schedule(boost::bind(busyExecuting, 300000));
    unsigned long long elapseBeforeExpire = 0;
    Timer::ptr timer = manager.registerTimer(50000,
        boost::bind(onTimer,
                    boost::ref(elapseBeforeExpire),
                    Mordor::TimerManager::now()));
    manager.dispatch();
    // timer expired in an acceptable deviation
    MORDOR_TEST_ASSERT_ABOUT_EQUAL(elapseBeforeExpire, 50000U, 50000);
}
