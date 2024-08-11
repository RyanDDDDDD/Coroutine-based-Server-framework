#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include "iomanager.hpp"
#include "macro.hpp"
#include "log.hpp"

namespace Server {

static Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SERVER_ASSERT_INFO(false, "getContext");
    }
};

void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
};

void IOManager::FdContext::triggerEvent(Event event){
    SERVER_ASSERT(events & event);

    events = static_cast<Event>(events & ~event);
    EventContext& ctx = getContext(event);

    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }

    // reset ctx after the ctx is scheduled into scheduler
    ctx.scheduler = nullptr;
};

IOManager::IOManager(size_t threads, bool useCaller, const std::string& name)
    :Scheduler(threads, useCaller, name) {
    m_epfd = epoll_create(5000);
    SERVER_ASSERT(m_epfd > 0);

    int ret = pipe(m_notifyFds);
    SERVER_ASSERT(!ret);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_notifyFds[0];

    ret = fcntl(m_notifyFds[0], F_SETFL, O_NONBLOCK);
    SERVER_ASSERT(!ret);

    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_notifyFds[0], &event);
    SERVER_ASSERT(!ret);

    // initialize fd context 
    contextResize(32);

    start();
};

IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_notifyFds[0]);
    close(m_notifyFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
};  

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

// -1 error, 0 success
int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    FdContext* fdCtx = nullptr;
    RWMutexType::ReadLock lock(m_mtx);
    if (static_cast<int>(m_fdContexts.size()) > fd) {
        fdCtx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        // update m_fdContexts using write lock
        RWMutexType::WriteLock lock2(m_mtx);
        contextResize(fd * 1.5);   // TODO: resize according to fd 
        fdCtx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fdCtx->mtx);
    if (fdCtx->events & event) {
        SERVER_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                                << " event = " << event
                                << " fdCtx.event = " << fdCtx->events;
        
        SERVER_ASSERT(!(fdCtx->events & event));
    }

    int op = fdCtx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event ep_event;
    ep_event.events = EPOLLET | fdCtx->events | event;
    ep_event.data.ptr = fdCtx;

    int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
    if (ret) {
        // errno, macro indicate error conditions within program when dealing with system calls and library function
        SERVER_LOG_ERROR(g_logger) << "epoll_ctl (" << m_epfd << ", "
                                << op << "," << fd << "," << ep_event.events << "):"
                                << ret << " (" << errno << ") (" << strerror(errno) << ")";

        return -1;
    }

    ++m_pendingEventCount;
    fdCtx->events = static_cast<Event>(fdCtx->events | event);
    FdContext::EventContext& eventCtx = fdCtx->getContext(event);
    SERVER_ASSERT(!(eventCtx.scheduler || eventCtx.fiber || eventCtx.cb));

    eventCtx.scheduler = Scheduler::getThis();
    if (cb) {
        eventCtx.cb.swap(cb);
    } else {
        eventCtx.fiber = Fiber::getThis();
        SERVER_ASSERT(eventCtx.fiber->getState() == Fiber::State::EXEC);
    }

    return 0;
};

bool IOManager::delEvent(int fd, Event event){
    RWMutexType::ReadLock lock(m_mtx);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mtx);
    if(!(fd_ctx->events | event)) {
        return false;
    }

    Event new_event = static_cast<Event>(fd_ctx->events & ~event);
    
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ep_event;
    ep_event.events = EPOLLET | new_event;
    ep_event.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
    if (ret) {
        SERVER_LOG_ERROR(g_logger) << "epoll_ctl (" << m_epfd << ", "
                                << op << "," << fd << "," << ep_event.events << "):"
                                << ret << " (" << errno << ") (" << strerror(errno) << ")";

        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
};

bool IOManager::cancelEvent(int fd, Event event){
    RWMutexType::ReadLock lock(m_mtx);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mtx);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_event = static_cast<Event>(fd_ctx->events & ~event);
    
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ep_event;
    ep_event.events = EPOLLET | new_event;
    ep_event.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
    if (ret) {
        SERVER_LOG_ERROR(g_logger) << "epoll_ctl (" << m_epfd << ", "
                                << op << "," << fd << "," << ep_event.events << "):"
                                << ret << " (" << errno << ") (" << strerror(errno) << ")";

        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;

    SERVER_ASSERT(fd_ctx->events == NONE);
    return true;
};

bool IOManager::cancelAll(int fd){
    RWMutexType::ReadLock lock(m_mtx);
    if (static_cast<int>(m_fdContexts.size()) < fd) {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    
    FdContext::MutexType::Lock lock2(fd_ctx->mtx);
    if(!fd_ctx->events ) {
        return false;
    }
    
    int op = EPOLL_CTL_DEL;
    epoll_event ep_event;
    ep_event.events = 0;
    ep_event.data.ptr = fd_ctx;

    int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
    if (ret) {
        SERVER_LOG_ERROR(g_logger) << "epoll_ctl (" << m_epfd << ", "
                                << op << "," << fd << "," << ep_event.events << "):"
                                << ret << " (" << errno << ") (" << strerror(errno) << ")";

        return false;
    }

    // process event before cancel
    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SERVER_ASSERT(fd_ctx->events == NONE);
    return true;
};

IOManager* IOManager::getThis(){
    // retrieve smart pointer from Schduler::getThis() (i.e return a static pointer to scheduler)
    return dynamic_cast<IOManager*>(Scheduler::getThis());
};

void IOManager::notify() {
    if (!hasIdleThreads()) {
        return;
    }

    int ret = write(m_notifyFds[1], "T", 1);
    SERVER_ASSERT(ret == 1);
}

bool IOManager::stopped() {
    return Scheduler::stopped()
        && m_pendingEventCount == 0;
}

void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        if (stopped()) {
            SERVER_LOG_INFO(g_logger) << "name=" << getName() << " idle stopped exit";
            break;
        }

        int ret = 0;

        do {
            static const int MAX_TIMEOUT = 5000;
            ret = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);
            
            if (ret < 0 && errno == EINTR) {

            } else {
                break;
            }
        } while (true);

        for (int i = 0; i < ret; ++i) {
            epoll_event& event = events[i];

            if (event.data.fd == m_notifyFds[0]) {
                uint8_t dummy;
                while (read(m_notifyFds[0], &dummy, 1) == 1);
                continue;
            }

            FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);
            FdContext::MutexType::Lock lock(fd_ctx->mtx);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }

            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if ((fd_ctx->events & real_events) == NONE) 
                continue;

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int ret2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (ret2) {
                SERVER_LOG_ERROR(g_logger) << "epoll_ctl (" << m_epfd << ", "
                                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                                    << ret2 << " (" << errno << ") (" << strerror(errno) << ")";

                continue;
            }

            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }

            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::getThis();
        auto raw_ptr = cur.get();
        cur.reset();        

        raw_ptr->swapOut();
    }
}

}
