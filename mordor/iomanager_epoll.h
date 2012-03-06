#ifndef __MORDOR_IOMANAGER_EPOLL_H__
#define __MORDOR_IOMANAGER_EPOLL_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <boost/scoped_ptr.hpp>

#include "scheduler.h"
#include "semaphore.h"
#include "timer.h"
#include "version.h"

#ifndef LINUX
#error IOManagerEPoll is Linux only
#endif

namespace Mordor {

class Fiber;

class IOManager : public Scheduler, public TimerManager
{
public:
    enum Event {
        NONE  = 0x0000,
        READ  = 0x0001,
        WRITE = 0x0004,
        CLOSE = 0x2000
    };

private:
    struct AsyncState : boost::noncopyable
    {
        AsyncState();
        ~AsyncState();

        struct EventContext
        {
            EventContext() : scheduler(NULL) {}
            Scheduler *scheduler;
            boost::shared_ptr<Fiber> fiber;
            boost::function<void ()> dg;
        };

        EventContext &contextForEvent(Event event);
        bool triggerEvent(Event event, size_t *pendingEventCount = NULL,
            boost::shared_ptr<Fiber> *fiber = NULL,
            boost::function<void ()> *dg = NULL);

        int m_fd;
        EventContext m_in, m_out, m_close;
        Event m_events;
        boost::mutex m_mutex;
    };

public:
    /// @note in default behavior, every working thread will call idle()
    /// when no more suitable fibers to be executed. However in a busy
    /// system, all working threads are busy with execution. There could be
    /// chances that idle() doesn't have a chance to get called for seconds,
    /// in this situation, epoll events or expired timers will be delayed for
    /// a while depends on the load of the system. If @enableEventThread set to
    /// false, all threads will execute jobs or enter eventLoopIdle() to
    /// handle I/O events. If @enableEventThread sets to true,  all the threads
    /// created by Scheduler will do workerPoolIdle(), these threads are just
    /// the same as workerpool threads, one additional thread is created and
    /// dedicated to run eventLoopIdle() which isn't visible to Scheduler.
    IOManager(size_t threads = 1, bool useCaller = true, bool enableEventThread = false);
    ~IOManager();

    bool stopping();

    void registerEvent(int fd, Event events,
        boost::function<void ()> dg = NULL);
    /// Will not cause the event to fire
    /// @return If the event was successfully unregistered before firing normally
    bool unregisterEvent(int fd, Event events);
    /// Will cause the event to fire
    bool cancelEvent(int fd, Event events);

protected:
    bool stopping(unsigned long long &nextTimeout);
    void idle();
    void tickle();

    void onTimerInsertedAtFront() { tickle(); }
    void workerPoolIdle();
    void eventLoopIdle();
    void eventLoop();

private:
    int m_epfd;
    int m_tickleFds[2];
    size_t m_pendingEventCount;
    boost::mutex m_mutex;
    std::vector<AsyncState *> m_pendingEvents;
    Semaphore m_semaphore;
    boost::scoped_ptr<Thread> m_eventThread;
};

}

#endif
