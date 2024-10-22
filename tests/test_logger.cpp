#include "source/log.hpp"
#include "source/util.hpp"
#include <iostream>
#include <thread>

int main(){
    // Server::Logger::ptr logger(new Server::Logger); 

    // logger->addAppender(Server::LogAppender::ptr(new Server::StdoutLogAppender));   // output to console

    // Server::FileLogAppender::ptr file_appender(new Server::FileLogAppender("./log.txt"));
    // Server::LogEvent::ptr event(new Server::LogEvent(__FILE__,__LINE__, 0, Server::getThreadId(), Server::getFiberId(), time(0)));

    // Server::LogFormatter::ptr fmt(new Server::LogFormatter("%d%T%p%T%m%n"));

    // file_appender->setFormatter(fmt);

    // file_appender->setLevel(Server::LogLevel::Level::ERROR);

    // logger->addAppender(file_appender);

    // event->getSS() << "Hello Log";
    // logger->log(Server::LogLevel::Level::DEBUG, event);

    // SERVER_LOG_DEBUG(logger) << "test macro";
    // SERVER_LOG_ERROR(logger) << "test macro error";
    
    // SERVER_LOG_FMT_ERROR(logger, "test macro fmt error %s %s %s", "aa", "xx", "bb");

    // get a logger, if there is no logger labeled as "xx", return a default one
    auto l = Server::LoggerMgr::getInstance()->getLogger("xx");
    
    SERVER_LOG_INFO(l) << "xxx";

    return 0;
}