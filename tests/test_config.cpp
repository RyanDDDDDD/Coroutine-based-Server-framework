#include "../source/config.hpp"
#include "../source/log.hpp"

#include <vector>
#include <yaml-cpp/yaml.h>

// store two args into Mgr
Server::ConfigArg<int>::ptr g_int_value_config = 
        Server::ConfigMgr::lookUp("system.port",(int)8080, "system port");

// Server::ConfigArg<float>::ptr g_int_valuex_config = 
        // Server::ConfigMgr::lookUp("system.port",(float)8080, "system port");

Server::ConfigArg<float>::ptr g_float_value_config = 
        Server::ConfigMgr::lookUp("system.value", (float)10.2f, "system value");

Server::ConfigArg<std::vector<int>>::ptr g_int_vec_value_config =  
        Server::ConfigMgr::lookUp("system.int_vec", std::vector<int>{1,2}, "system int vec");

Server::ConfigArg<std::list<int>>::ptr g_int_list_value_config =  
        Server::ConfigMgr::lookUp("system.int_list", std::list<int>{1,2}, "system int list");

Server::ConfigArg<std::set<int>>::ptr g_int_set_value_config =  
        Server::ConfigMgr::lookUp("system.int_set", std::set<int>{1,2}, "system int set");

Server::ConfigArg<std::unordered_set<int>>::ptr g_int_unordered_set_value_config =  
        Server::ConfigMgr::lookUp("system.int_unordered_set", std::unordered_set<int>{1,2}, "system int unordered_set");

Server::ConfigArg<std::map<std::string, int>>::ptr g_str_int_map_value_config =  
        Server::ConfigMgr::lookUp("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");

Server::ConfigArg<std::unordered_map<std::string, int>>::ptr g_str_int_unordered_map_value_config =  
        Server::ConfigMgr::lookUp("system.str_int_unordered_map", std::unordered_map<std::string, int>{{"k", 2}}, "system str int map");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ') 
                << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()) {
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ')
                << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); it++) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ')
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); i++) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level);
        }
    }
};

void test_yaml() {
    YAML::Node root = YAML::LoadFile("config/config.yaml");
    print_yaml(root, 0);

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << root.Scalar();
};

void test_config() {
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix)     \
    {                               \
        auto v = g_var->getValue(); \
        for (auto& i : v) {         \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix)     \
    {                               \
        auto v = g_var->getValue(); \
        for (auto& i : v) {         \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        } \
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }


    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);
    XX_M(g_str_int_map_value_config, int_map, before);
    XX_M(g_str_int_unordered_map_value_config, str_int_unordered_map, before);

    YAML::Node root = YAML::LoadFile("config/config.yaml");
    Server::ConfigMgr::loadFromYaml(root);

    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_unordered_set, after);
    XX_M(g_str_int_map_value_config, int_map, after);
    XX_M(g_str_int_unordered_map_value_config, str_int_unordered_map, after);

#undef XX 
#undef XX_M
}

class Person {
public:
    std::string m_name = "";
    int m_age = 0;
    bool m_sex = false;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name = "  << m_name
            << " age = " << m_age
            << "  sex = " << m_sex
            << "]";
        
        return ss.str();
    }
};

namespace Server {

// fully specialized template for conversion from string to Person
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        Person p;
        
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();

        return p;
    };
};

// fully specialized template for conversion from Person to string
template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node;

        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};
}

Server::ConfigArg<Person>::ptr g_person = 
    Server::ConfigMgr::lookUp("class.person", Person(), "system admin");

Server::ConfigArg<std::map<std::string, Person>>::ptr g_person_map = 
    Server::ConfigMgr::lookUp("class.map", std::map<std::string, Person>(), "system admin");

Server::ConfigArg<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map = 
    Server::ConfigMgr::lookUp("class.vec_map", std::map<std::string, std::vector<Person>>(), "system admin");

void test_class() {
    // SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    {   \
        auto m = g_person_map->getValue(); \
        for (auto& i : m) { \
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix << ": " << i.first << " - " << i.second.toString(); \
        }   \
        SERVER_LOG_INFO(SERVER_LOG_ROOT()) << #prefix << ": size = " << m.size(); \
    }   \

    // XX_PM(g_person_map, "class.map before");
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("config/config.yaml");
    Server::ConfigMgr::loadFromYaml(root);

    // SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    // XX_PM(g_person_map, "class.map after");
    SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
#undef XX_PM
};

int main() {

    // test_yaml();
    // test_config();
    test_class();

    return 0;
}