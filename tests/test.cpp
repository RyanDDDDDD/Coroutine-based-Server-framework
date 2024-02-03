#include <iostream>
#include "../source/Log/log.hpp"

int main(){
    Server::Logger::ptr logger(new Server::Logger); 

    logger->addAppender(Server::LogAppender::ptr(new Server::StdoutLogAppender));   // output to console

    Server::LogEvent::ptr event(new Server::LogEvent(__FILE__,__LINE__, 0, 1, 2, time(0)));

    event->getSS() << "Hello Log";

    logger->log(Server::LogLevel::Level::DEBUG, event);

    std::cout << "Hello Server" << std::endl;

    return 0;
}