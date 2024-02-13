#ifndef __SERVER_THREAD_HPP__
#define __SERVER_THREAD_HPP__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <stdint.h>

namespace Server {

template<typename T>
class ScopedLock {
public:
    ScopedLock(T& mutex): m_mutex{mutex}{
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLock() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<typename T>
class ScopedRdLock {
public:
    ScopedRdLock(T& mutex): m_mutex{mutex}{
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ScopedRdLock() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<typename T>
class ScopedWriteLock {
public:
    ScopedWriteLock(T& mutex): m_mutex{mutex}{
        m_mutex.wrlock();
        m_locked = true;
    }

    ~ScopedWriteLock() {
        unlock();
    }

    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};


class RWMutex {
public:
    using ReadLock = ScopedRdLock<RWMutex>;
    using WriteLock = ScopedWriteLock<RWMutex>;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

class Semaphore {
public:
    Semaphore(uint32_t count = 0);

    ~Semaphore();

    void wait();
    void notify();
private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    
private:
    sem_t m_semaphore;  
};

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

    Semaphore m_semaphore;
};

}

#endif