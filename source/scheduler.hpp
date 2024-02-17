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
    /*
        threads: number of threads in thread pool
        userCaller: true when coroutine needs caller thread, false otherwise
        name: name of scheduler
    */
    Scheduler(size_t threads = 1, bool useCaller = true, const std::string& name = "");

    virtual ~Scheduler();

    /*
        retrieve name of scheduler
    */ 
    const std::string& getName() const { return m_name; };

    /*
        retrieve scheduler
    */ 
    static Scheduler* getThis();

    /*
        retrieve main coroutine
    */ 
    static Fiber* getMainFiber();

    void start();

    void stop();


    /*
        schedule functional object or coroutine
    */ 
    template<typename FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool needTickle = false;
        {
            MutexType::Lock lock(m_mutex);
            needTickle = scheduleNoLock(FiberOrCb fc, int thread);   
        }

        if (needTickle) {
            tickle();
        }
    }

    template<typename InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool needTickle = false;

        {
            MutexType::Lock lock(m_mutex);
            while (begin++ != end) {
                needTickle = scheduleNoLock(&*begin) || needTickle;
            }
        }

        if (needTickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();
    virtual bool stopped();
    virtual void idle();
    void run();
    void setThis();

private:
    template<typename FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool needTickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        
        return needTickle;
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
    MutexType m_mutex;
    // thread pool managed by scheduler
    std::vector<Thread::ptr> m_threads;

    // message queue storing either coroutine or functional obejct
    std::list<FiberAndThread> m_fibers;
    
    // main coroutine
    Fiber::ptr m_rootFiber;

    // name of scheduler
    std::string m_name;

protected:
    std::vector<int> m_threadIds;
    
    size_t m_threadCount = 0;

    std::atomic<size_t> m_activeThreatCount{ 0 };
    
    std::atomic<size_t> m_idleThreadCount{ 0 };
    
    // if scheduler is stopped
    bool m_isRunning = false;
    
    // if scheduler is automatically stopped
    bool m_autoStop = false;
    // main thread id
    int m_rootThread = 0;   
};




}

#endif