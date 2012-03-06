#ifndef __MORDOR_IOMANAGER_EPOLL_H__
#define __MORDOR_IOMANAGER_EPOLL_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <sys/types.h>
#include <sys/event.h>

#include <map>
#include <boost/scoped_ptr.hpp>

#include "scheduler.h"
#include "semaphore.h"
#include "timer.h"
#include "version.h"

#ifndef BSD
#error IOManagerKQueue is BSD only
#endif

namespace Mordor {

class IOManager : public Scheduler, public TimerManager
{
public:
    enum Event {
        READ,
        WRITE,
        CLOSE
    };

private:
    struct AsyncEvent
    {
        struct kevent event;

        Scheduler *m_scheduler, *m_schedulerClose;
        boost::shared_ptr<Fiber> m_fiber, m_fiberClose;
        boost::function<void ()> m_dg, m_dgClose;

        bool operator<(const AsyncEvent &rhs) const
        { if (event.ident < rhs.event.ident) return true; return event.filter < rhs.event.filter; }
    };

public:
    IOManager(size_t threads = 1, bool useCaller = true, bool enableEventThread = false);
    ~IOManager();

    bool stopping();

    void registerEvent(int fd, Event events, boost::function<void ()> dg = NULL);
    void cancelEvent(int fd, Event events);
    void unregisterEvent(int fd, Event events);

protected:
    bool stopping(unsigned long long &nextTimeout);
    void idle();
    void tickle();

    void onTimerInsertedAtFront() { tickle(); }
    void workerPoolIdle();
    void eventLoopIdle();
    void eventLoop();

private:
    int m_kqfd;
    int m_tickleFds[2];
    std::map<std::pair<int, Event>, AsyncEvent> m_pendingEvents;
    boost::mutex m_mutex;
    Semaphore m_semaphore;
    boost::scoped_ptr<Thread> m_eventThread;
};

}

#endif

