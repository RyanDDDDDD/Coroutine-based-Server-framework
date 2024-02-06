#include "../source/config.hpp"
#include "../source/log.hpp"

#include <yaml-cpp/yaml.h>

Server::ConfigArg<int>::ptr g_int_value_config = 
        Server::ConfigMgr::lookUp("sysyrm.port",(int)8080, "system port");

Server::ConfigArg<float>::ptr g_float_value_config = 
        Server::ConfigMgr::lookUp("system.value", (float)10.2f, "system value");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); it++) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); i++) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level);
        }
    }
};

void test_yaml() {
    YAML::Node root = YAML::LoadFile("config/config.yaml");

    print_yaml(root, 0);

};

int main() {

    // SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->getValue();
    // SERVER_LOG_INFO(SERVER_LOG_ROOT()) << g_int_value_config->toString();

    test_yaml();


    return 0;
}