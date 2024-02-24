#ifndef __SERVER_IOMANAGER_HPP__
#define __SERVER_IOMANAGER_HPP__

#include "scheduler.hpp"

namespace Server {

class IOManager : public Scheduler {
public:
    using ptr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x2
    };

private:
    class FdContext  {
    public:
        using MutexType = Mutex;
        class EventContext {
        public:
            // pointer to scheduler to process event
            Scheduler* scheduler = nullptr;

            // event coroutine
            Fiber::ptr fiber;

            // event callback
            std::function<void()> cb;
        };      

        EventContext& getContext(Event event);

        void resetContext(EventContext& ctx);

        void triggerEvent(Event event);

        // file descriptor of the event
        int fd;

        // read event
        EventContext read;

        // write event
        EventContext write;

        // registered event
        Event events = Event::NONE;

        // read-write lock
        MutexType mtx;
    };

public:
    IOManager(size_t threads = 1, bool useCaller = true, const std::string& name = "");
    ~IOManager();  

    // -1 error, 0 retry, 1 success
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    static IOManager* getThis();

protected:
    // notify task which can be executed
    void notify() override;

    // determine if the scheduler is stopped and executes all tasks in thread pool
    bool stopped() override;

    // used for a idle coroutine when no events in thread pool
    void idle() override;

    void contextResize(size_t size);

private:
    int m_epfd{ 0 };
    int m_notifyFds[2];

    std::atomic<size_t> m_pendingEventCount{ 0 };
    MutexType m_mtx;
    std::vector<FdContext*> m_fdContexts;
};

}

#endif