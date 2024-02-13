#ifndef __SERVER_THREAD_HPP__
#define __SERVER_THREAD_HPP__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>

namespace Server {
class Thread {
public:
    using ptr = std::shared_ptr<Thread>;
    
    Thread(std::function<void()> cb, const std::string& name);

    ~Thread();

    pid_t getId() const { return m_id; };
    const std::string& getName() const { return m_name; };

    void join();

    /* static method to config/retrieve thread info */ 
    // Get current thread
    static Thread* getThis();

    // Get the name of thread (used by logger)
    static const std::string& GetName();

    // Set thread name
    static void setName(const std::string& name);

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    // accept and run a thread 
    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;
};

}

#endif