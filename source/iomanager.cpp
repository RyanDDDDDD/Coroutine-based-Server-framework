#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "iomanager.hpp"
#include "macro.hpp"
#include "log.hpp"

namespace Server {

static Server::Logger::ptr g_logger = SERVER_LOG_NAME("system");

IOManager::IOManager(size_t threads, bool useCaller, const std::string& name)
    :Scheduler(threads, useCaller, name) {
    m_epfd = epoll_create(5000);
    SERVER_ASSERT(m_epfd > 0);

    int ret = pipe(m_notifyFds);
    SERVER_ASSERT(ret);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_notifyFds[0];

    ret = fcntl(m_notifyFds[0], F_SETFL, O_NONBLOCK);
    SERVER_ASSERT(ret);

    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_notifyFds[0], &event);
    SERVER_ASSERT(ret);

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

// -1 error, 0 retry, 1 success
int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mtx);
    if (m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
    } else {
        lock.unlock();
        
        RWMutexType::WriteLock lock2(m_mtx);
        contextResize(m_fdContexts.size() * 1.5);   // TODO: resize according to fd 
        fd_ctx = m_fdContexts[fd];
    }

    fd_ctx = m_fdContexts[fd];
    FdContext::MutexType::Lock lock2(fd_ctx->mtx);
    if (static_cast<int>(fd_ctx->events) & static_cast<int>(event)) {
        // SERVER_LOG_ERROR(g_logger);
    }


};

bool IOManager::delEvent(int fd, Event event){

};

bool IOManager::cancelEvent(int fd, Event event){

};

bool IOManager::cancelAll(int fd){

};

IOManager::IOManager* getThis(){

};

}