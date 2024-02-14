#include <list>
#include <utility>
#include "config.hpp"

namespace Server {

ConfigArgBase::ptr ConfigMgr::lookUpBase(const std::string& name) {
    RWMutexType::ReadLock lock(getMutex());
    
    auto it = getData().find(name);

    return it == getData().end() ? nullptr : it->second;
};

// "A.B", 10
//  ||
// A:
//    B: 10
// parse all nodes and store into list
static void listAllMember(const std::string& key,
                        const YAML::Node& node,
                        std::list<std::pair<std::string, const YAML::Node>>& output) {
    if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
        SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "Config invalid name: " << key << " : " << node.Tag();
        return;
    }

    output.push_back(std::make_pair(key, node));
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            // store key-value as: parent.child - child-value
            // recursively call this method to process all nodes
            listAllMember(key.empty() ? it->first.Scalar() : key + '.' + it->first.Scalar(), it->second, output);
        }
    }
};

// load all configuration from ymal
void ConfigMgr::loadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node>> allNodes;
    listAllMember("", root, allNodes);
    
    for (auto& i : allNodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigArgBase::ptr arg = lookUpBase(key);
        
        if (arg) {
            if (i.second.IsScalar()) {          // int/float/string
                arg->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                arg->fromString(ss.str());
            }
        }
    }
};

void ConfigMgr::Visit(std::function<void(ConfigArgBase::ptr)> cb) {
    RWMutex::ReadLock lock(getMutex());

    ConfigArgMap& m = getData();
    for (auto it = m.begin(); it != m.end(); ++it) {
        cb(it->second);
    }
}

}
