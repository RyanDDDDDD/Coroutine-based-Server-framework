#include "source/headers.hpp"
#include "source/iomanager.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

int sock = 0;

void test_fiber() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "142.251.221.68", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        SERVER_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        Server::IOManager::getThis()->addEvent(sock, Server::IOManager::READ, [&](){
            SERVER_LOG_INFO(g_logger) << "read callback";
        });
        Server::IOManager::getThis()->addEvent(sock, Server::IOManager::WRITE, [&](){
            SERVER_LOG_INFO(g_logger) << "write callback";
            Server::IOManager::getThis()->cancelEvent(sock, Server::IOManager::READ);
            close(sock);
        });
    } else {
        SERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    
    Server::IOManager ioMgr;
    ioMgr.schedule(&test_fiber);
}

int main(int argc, char** argv) {
    test1(); 
   
    return 0;
}