#ifndef __SERVER_SCHEDULER_HPP__
#define __SERVER_SCHEDULER_HPP__

#include <memory>
#include <vector>
#include <list>

#include "fiber.hpp"
#include "mutex.hpp"
#include "thread.hpp"

namespace Server {

class Scheduler {
public:
    using ptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;
    
    
    // Constructor
    // threads: number of threads in thread pool
    // userCaller: true when coroutine needs caller thread, false otherwise
    // name: name of scheduler
    Scheduler(size_t threads = 1, bool useCaller = true, const std::string& name = "");

    virtual ~Scheduler();
    
    // retrieve name of scheduler
    const std::string& getName() const { return m_name; };

    // retrieve pointer to scheduler
    static Scheduler* getThis();

    // retrieve pointer to main coroutine
    static Fiber* getMainFiber();
    
    // start scheduler
    void start();

    // stop scheduler
    void stop();

    // schedule a functional object or coroutine
    // fc: functional object or coroutine to be executed
    // thread: specify thread to execute task     
    template<typename FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool needNotify = false;
        {
            MutexType::Lock lock(m_mutex);
            needNotify = scheduleNoLock(fc, thread);   
        }

        if (needNotify) {
            notify();
        }
    }

    template<typename InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool needNotify = false;

        {
            MutexType::Lock lock(m_mutex);
            while (begin++ != end) {
                needNotify = scheduleNoLock(&*begin) || needNotify;
            }
        }

        if (needNotify) {
            notify();
        }
    }

protected:
    // notify task which can be executed
    virtual void notify();

    // determine if the scheduler is stopped and executes all tasks in thread pool
    virtual bool stopped();

    // used for a idle coroutine when no events in thread pool
    virtual void idle();
    void run();
    void setThis();

    bool hasIdleThreads() {return m_idleThreadCount > 0;}

private:
    template<typename FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool needNotify = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        
        return needNotify;
    }

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr):fiber{ f }, thread{ thr } {}

        FiberAndThread(Fiber::ptr* f, int thr):thread{ thr } {
            fiber = std::move(*f);
        }

        FiberAndThread(std::function<void()> f, int thr):cb{ f }, thread{ thr } {}

        FiberAndThread(std::function<void()>* f, int thr): thread{ thr } {
            cb = std::move(*f);
        }

        FiberAndThread(): thread{ -1 } {

        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    // mutex
    MutexType m_mutex;

    // thread pool managed by scheduler
    std::vector<Thread::ptr> m_threads;

    // message queue storing either coroutine or functional obejct
    std::list<FiberAndThread> m_fibers;
    
    // pointer to main coroutine
    Fiber::ptr m_rootFiber;

    // name of scheduler
    std::string m_name;

protected:
    // a list of thread id in thread pool
    std::vector<int> m_threadIds;
    
    // number of threads in thread pool
    size_t m_threadCount{ 0 };

    // number of active thread
    std::atomic<size_t> m_activeThreadCount{ 0 };
    
    // number of idle thread
    std::atomic<size_t> m_idleThreadCount{ 0 };
    
    // if scheduler is stopped
    bool m_isRunning{ false };
    
    // flag to determine if scheduler is stopped by stop()
    bool m_autoStop{ false };

    // main thread id
    int m_rootThread{ 0 };   
};




}

#endif