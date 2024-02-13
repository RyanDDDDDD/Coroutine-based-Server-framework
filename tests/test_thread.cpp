#include "../source/headers.hpp"

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void func1() {
    SERVER_LOG_INFO(g_logger) << "thread name: " << Server::Thread::GetName()
                            << " this.name: " << Server::Thread::getThis()->getName()
                            << " id: " << Server::getThreadId()
                            << " this.id " << Server::Thread::getThis()->getId();
};

void func2() {

};

int main() {
    SERVER_LOG_INFO(g_logger) << "thread test begin";
    std::vector<Server::Thread::ptr> threads;

    for (int i = 0; i < 5; ++i) {
        Server::Thread::ptr thread = std::make_shared<Server::Thread>(&func1, "name_" + std::to_string(i));
        threads.push_back(thread);
    }

    for (int i = 0; i < 5; ++i) {
        threads[i]->join();
    }

    SERVER_LOG_INFO(g_logger) << "thread test end";
    return 0;
}