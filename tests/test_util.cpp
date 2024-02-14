#include "../source/headers.hpp"

Server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_assert() {
    
    SERVER_LOG_INFO(g_logger) << "\n" << Server::BacktraceToString(10);

    SERVER_ASSERT(false);
}

int main() {
    test_assert();
    
    return 0;
}