#include "../source/config.hpp"
#include "../source/log.hpp"

Server::ConfigArg<int>::ptr g_int_value_config = 
        Server::ConfigMgr::lookUp("sysyrm.port",(int)8080, "system port");

Server::ConfigArg<float>::ptr g_float_value_config = 
        Server::ConfigMgr::lookUp("system.value", (float)10.2f, "system value");

int main() {
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->toString();

    return 0;
}