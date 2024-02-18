#include "../source/headers.hpp"

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_fiber() {
    SERVER_LOG_INFO(g_logger) << "test in fiber";

    static int s_count = 5;
    sleep(1);
    if(--s_count >= 0) {
        Server::Scheduler::getThis()->schedule(&test_fiber);
    }
}

int main() {
    SERVER_LOG_INFO(g_logger) << "main";
    Server::Scheduler sc(3, false, "test");
    sc.start();
    SERVER_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    SERVER_LOG_INFO(g_logger) << "over";
    return 0;
}