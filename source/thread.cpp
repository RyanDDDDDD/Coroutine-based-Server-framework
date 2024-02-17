#include "thread.hpp"
#include "log.hpp"
#include "util.hpp"

namespace Server {

// pointer to current thread, we use 
// static getThis() to retrieve current thread
static thread_local Thread* t_thread{ nullptr };            

// thread name, it would be called by logger when logging
static thread_local std::string t_thread_name{ "UNKNOWN" }; 

// logger to output system info
static Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");


Thread* Thread::getThis() {
    return t_thread;
};

const std::string& Thread::GetName() {
    return t_thread_name;
};

void Thread::setName(const std::string& name) {
    if (t_thread) {
        t_thread->m_name = name;
    }

    t_thread_name = name;
};

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb{ cb }
    , m_name{ name } {
    if (m_name.empty()) {
        m_name = "UNKNOWN";
    }

    // create thread and start running
    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (ret) {
        SERVER_LOG_ERROR(g_logger) << "pthread_create thread failed, ret = " << ret 
                                    << " name = " << m_name;
        throw std::logic_error("pthread_create error");
    }

    // wait unitl the thread is created in kernel and run
    m_semaphore.wait();
};

Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
    }
};

void Thread::join() {
    if (m_thread) {
        int ret = pthread_join(m_thread, nullptr);
        if (ret) {
            SERVER_LOG_ERROR(g_logger) << "pthread_join thread failed, ret = " << ret
                                        << " name = " << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
};

void* Thread::run(void* arg) {
    Thread* thread = static_cast<Thread*>(arg);

    t_thread = thread;

    // set current thread name, which could be retrieved by static GetName()
    t_thread_name = thread->m_name;

    // get thread id from kernel
    thread->m_id = Server::getThreadId();

    // set up thread name in kernel, by default, 
    // all threads inherit program name when created 
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    // exchange ownership of the m_cb, to avoid 
    // unnecessary copying smart pointer when the 
    // callable contains smart pointer
    cb.swap(thread->m_cb);

    // thread is created successfullly, start running the callback
    thread->m_semaphore.notify();

    cb();

    return 0;
};

}