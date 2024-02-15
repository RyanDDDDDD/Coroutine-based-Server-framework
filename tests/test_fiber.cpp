#include "../source/headers.hpp"

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void run_in_fiber() {
    SERVER_LOG_INFO(g_logger) << "run_in_fiber begin";
    Server::Fiber::yieldToHold();
    SERVER_LOG_INFO(g_logger) << "run_in_fiber end";
    Server::Fiber::yieldToHold();
}

void test_fiber() {
    SERVER_LOG_INFO(g_logger) << "main begin - 1";
    {
        // create main coroutine
        Server::Fiber::getThis();
        SERVER_LOG_INFO(g_logger) << "main begin";
        Server::Fiber::ptr fiber = std::make_shared<Server::Fiber>(run_in_fiber);
        fiber->swapIn();
        SERVER_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        SERVER_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    SERVER_LOG_INFO(g_logger) << "main after end 2";
}

int main() {
    Server::Thread::setName("main");
    std::vector<Server::Thread::ptr> threads;

    for (int i = 0; i < 3; ++i) {
        threads.push_back(std::make_shared<Server::Thread>(&test_fiber, "name_" + std::to_string(i)));
    }

    for (auto i : threads) {
        i->join();
    }

    return 0;
}
