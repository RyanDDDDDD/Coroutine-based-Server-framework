#include "scheduler.hpp"
#include "log.hpp"
#include "macro.hpp"

namespace Server {
static Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool useCaller, const std::string& name): m_name{ name } {
    SERVER_ASSERT(threads > 0);

    if (useCaller) {
        // set the pointer to current main coroutine points to current thread(main coroutine)
        Server::Fiber::getThis();
        --threads;

        SERVER_ASSERT(getThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        Server::Thread::setName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThread = Server::getThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
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
    SERVER_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this),
                                    m_name + "_" + std::to_string(i));
        
        m_threadIds.push_back(m_threads[i]->getId());
    }

    lock.unlock();
    // if (m_rootFiber) {
    //     m_rootFiber->call();
    //     SERVER_LOG_INFO(g_logger) << "call out " << static_cast<int>(m_rootFiber->getState());
    // }
};

void Scheduler::stop() {
    m_autoStop = true;
    // check if main coroutine is terminated
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

    // bool exitOnThisFiber = false;
    
    if (m_rootThread != -1) {
        // caller thread
        SERVER_ASSERT(getThis() == this);
        
    } else {
        // callee thread
        SERVER_ASSERT(getThis() != this);

    }

    m_isRunning = false;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        // while (!stopped()) {
        //     if (m_rootFiber->getState() == Fiber::State::TERM
        //         || m_rootFiber->getState() == Fiber::State::EXCEPT) {
        //             m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //             SERVER_LOG_INFO(g_logger) << "root fiber is term, reset";
        //             t_fiber = m_rootFiber.get();
        //         }

        //     m_rootFiber->call();
        // }
        if (!stopped()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> threads;
    {
        MutexType::Lock lock(m_mutex);
        threads.swap(m_threads);
    }

    for (auto& t : threads) {
        t->join();
    }

    // if (exitOnThisFiber) {}

};

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    SERVER_LOG_INFO(g_logger) << "run";
    setThis();

    if (Server::getThreadId() != m_rootThread) {
        t_fiber = Fiber::getThis().get();
    } 

    Fiber::ptr idleFiber(new Fiber(std::bind(&Scheduler::idle,this)));
    Fiber::ptr cbFiber;
    
    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickleMe = false;
        bool isActive = false;
        {
            // retrieve message from message queue
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                if (it->thread != -1 && it->thread != Server::getThreadId()) {
                    ++it;
                    tickleMe = true;
                    continue;
                }

                SERVER_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::State::EXEC) {
                    ++it;
                    continue;
                }

                // retrieve next message from MQ
                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreatCount;
                isActive = true;
                break;
            }
        }

        if (tickleMe) {
            tickle();
        }

        if (ft.fiber && (ft.fiber->getState() != Fiber::State::TERM
                        && ft.fiber->getState() != Fiber::State::EXCEPT)) {
            // next message is a coroutine
            ft.fiber->swapIn();
            --m_activeThreatCount;
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
                cbFiber.reset(new Fiber(ft.cb));
                ft.cb = nullptr;
            }
            ft.reset();
            cbFiber->swapIn();
            --m_activeThreatCount;
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
                --m_activeThreatCount;
                continue;
            }

            // message queue is empty, we are in idle state
            if (idleFiber->getState() == Fiber::State::TERM) {
                SERVER_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            idleFiber->swapIn();
            --m_idleThreadCount;
            if(idleFiber->getState() != Fiber::State::TERM
                && idleFiber->getState() != Fiber::State::EXCEPT) {
                idleFiber->m_state = Fiber::State::HOLD;
            }
        }
    }
};

void Scheduler::tickle(){
    SERVER_LOG_INFO(g_logger) << "tickle";
};

bool Scheduler::stopped(){
    MutexType::Lock lock(m_mutex);
    return m_autoStop 
        && !m_isRunning
        && m_fibers.empty()
        && m_activeThreatCount == 0;
};

void Scheduler::idle(){
    SERVER_LOG_INFO(g_logger) << "idle";
    while(!stopped()) {
        Server::Fiber::yieldToHold();
    }
};

}