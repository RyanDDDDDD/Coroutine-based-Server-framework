#include "scheduler.hpp"
#include "log.hpp"
#include "macro.hpp"

namespace Server {
static Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

// t_scheduler and t_fiber can be retrieved by client using Scheduler::getThis(), Scheduler::getMainFiber()
// pointer to scheduler
static thread_local Scheduler* t_scheduler{ nullptr };

// pointer to the coroutine that is currently working on thread
static thread_local Fiber* t_fiber{ nullptr };

Scheduler::Scheduler(size_t threads, bool useCaller, const std::string& name): m_name{ name } {
    SERVER_ASSERT(threads > 0);

    if (useCaller) {
        // set the pointer to current main coroutine points to current thread(main coroutine)
        Server::Fiber::getThis();
        --threads;

        // scheduler should not be initialized before
        SERVER_ASSERT(getThis() == nullptr);
        t_scheduler = this;

        m_rootFiber = std::make_shared<Fiber>(std::bind(&Scheduler::run, this), 0, true);
        
        // set current thread name as the name of scheduler
        Server::Thread::setName(m_name);

        // setup static pointer to main coroutine
        t_fiber = m_rootFiber.get();

        m_rootThread = Server::getThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        // we won't use main coroutine to perform tasks
        m_rootThread = -1;
    }

    m_threadCount = threads;
};

Scheduler::~Scheduler() {
    SERVER_ASSERT(!m_isRunning);

    if (getThis() == this) {
        t_scheduler = nullptr;
    }
};

Scheduler* Scheduler::getThis() {
    return t_scheduler;
};

Fiber* Scheduler::getMainFiber() {
    return t_fiber;
};

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (m_isRunning){
        return;
    }

    m_isRunning = true;

    // the thread pool should be empty, no thread running in thread pool
    SERVER_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        // create a number of threads in thread pool, to perform tasks concurrently
        m_threads[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this),
                                    m_name + "_" + std::to_string(i));
        // record thread id
        m_threadIds.push_back(m_threads[i]->getId());
    }

    lock.unlock();
};

void Scheduler::stop() {
    m_autoStop = true;
    // check if main coroutine is terminated (thread pool is stopped)
    if (m_rootFiber
        && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::State::TERM
            || m_rootFiber->getState() == Fiber::State::INIT)
        )
    {
        SERVER_LOG_INFO(g_logger) << this << " stopped";
        m_isRunning = false;
        
        if (stopped()) {
            return;
        }
    }
    
    // there are still some threads running in thread pool
    if (m_rootThread != -1) {
        // caller thread, which should contain main coroutine
        SERVER_ASSERT(getThis() == this);
    } else {
        // callee thread
        SERVER_ASSERT(getThis() != this);
    }

    m_isRunning = false;
    for (size_t i = 0; i < m_threadCount; ++i) {
        notify();
    }

    if (m_rootFiber) {
        notify();
    }

    if (m_rootFiber) {
        if (!stopped()) {
            // call the callback of main coroutine
            m_rootFiber->call();
        }
    }

    // wait until all threads finish
    std::vector<Thread::ptr> threads;
    {
        MutexType::Lock lock(m_mutex);
        threads.swap(m_threads);
    }

    for (auto& t : threads) {
        t->join();
    }
};

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    SERVER_LOG_INFO(g_logger) << m_name << "run";
    setThis();

    // a sub thread in thread pool
    if (Server::getThreadId() != m_rootThread) {
        // set up sub coroutine 
        t_fiber = Fiber::getThis().get();
    } 

    Fiber::ptr idleFiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle,this));
    Fiber::ptr cbFiber = nullptr;
    
    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool notifyMe = false;
        bool isActive = false;
        {
            // retrieve message from message queue
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // current coroutine is assigned to specific thread
                if (it->thread != -1 && it->thread != Server::getThreadId()) {
                    ++it;
                    notifyMe = true;
                    continue;
                }

                SERVER_ASSERT(it->fiber || it->cb);
                // skip if next message is current coroutine which is executed by other threads
                if (it->fiber && it->fiber->getState() == Fiber::State::EXEC) {
                    ++it;
                    continue;
                }
                
                // retrieve next message from MQ
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                isActive = true;
                break;
            }
        }

        if (notifyMe) {
            notify();
        }

        if (ft.fiber && (ft.fiber->getState() != Fiber::State::TERM
                        && ft.fiber->getState() != Fiber::State::EXCEPT)) {
            // next message is a coroutine
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::State::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::State::TERM
                    && ft.fiber->getState() != Fiber::State::EXCEPT){
                ft.fiber->m_state = Fiber::State::HOLD;
            }

            ft.reset();
        } else if (ft.cb) {
            // next message is a callback
            if (cbFiber) {
                cbFiber->reset(ft.cb);
            } else {
                cbFiber = std::make_shared<Fiber>(ft.cb);
                ft.cb = nullptr;
            }
            ft.reset();
            cbFiber->swapIn();
            --m_activeThreadCount;
            if (cbFiber->getState() == Fiber::State::READY) {
                schedule(cbFiber);
                cbFiber.reset();
            } else if (cbFiber->getState() == Fiber::State::EXCEPT
                    || cbFiber->getState() == Fiber::State::TERM) {
                cbFiber->reset(nullptr);
            } else {
                cbFiber->m_state = Fiber::State::HOLD;
                cbFiber.reset();
            }
        } else {
            if (isActive) {
                --m_activeThreadCount;
                continue;
            }

            // message queue is empty, we are in idle state
            if (idleFiber->getState() == Fiber::State::TERM) {
                SERVER_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            
            ++m_idleThreadCount;
            idleFiber->swapIn();
            --m_idleThreadCount;
            if(idleFiber->getState() != Fiber::State::TERM
                && idleFiber->getState() != Fiber::State::EXCEPT) {
                idleFiber->m_state = Fiber::State::HOLD;
            }
        }
    }
};

void Scheduler::notify(){
    SERVER_LOG_INFO(g_logger) << "notify";
};

bool Scheduler::stopped(){
    MutexType::Lock lock(m_mutex);
    return m_autoStop 
        && !m_isRunning
        && m_fibers.empty()
        && m_activeThreadCount == 0;
};

void Scheduler::idle(){
    SERVER_LOG_INFO(g_logger) << "idle";
    while(!stopped()) {
        Server::Fiber::yieldToHold();
    }
};

}