#include "source/headers.hpp"

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

int count = 0;
Server::RWMutex rw_mutex;
Server::Mutex mutex;

void func1() {
    SERVER_LOG_INFO(g_logger) << "thread name: " << Server::Thread::GetName()
                            << " this.name: " << Server::Thread::getThis()->getName()
                            << " id: " << Server::getThreadId()
                            << " this.id " << Server::Thread::getThis()->getId();
    
    // for (int i = 0; i < 10000; ++i) {
    //     Server::Mutex::Lock lock(mutex);
    //     ++count;
    // }
};

void func2() {
    while(true) {
        SERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
};

void func3() {
    while(true) {
        SERVER_LOG_INFO(g_logger) << "===================================";
    }
};

int main() {
    SERVER_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/ruidong/Desktop/ServerFramework/bin/config/log.yaml");
    Server::ConfigMgr::loadFromYaml(root);

    std::vector<Server::Thread::ptr> threads;
    
    // test thread safety (if ouput doesn't mixed)
    for (int i = 0; i < 1; ++i) {
        Server::Thread::ptr thread = std::make_shared<Server::Thread>(&func2, "name_" + std::to_string(i * 2));
        Server::Thread::ptr thread_2 = std::make_shared<Server::Thread>(&func3, "name_" + std::to_string(i * 2 + 1));
        threads.push_back(thread);
        threads.push_back(thread_2);
    }

    for (int i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }

    SERVER_LOG_INFO(g_logger) << "thread test end";
    SERVER_LOG_INFO(g_logger) << "count = " << count;
    return 0;
}