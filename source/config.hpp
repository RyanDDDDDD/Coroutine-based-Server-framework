#ifndef __SERVER_CONFIG_HPP__
#define __SERVER_CONFIG_HPP__

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <yaml-cpp/yaml.h>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <functional>

#include "log.hpp"

namespace Server {

using boost::lexical_cast;

// Base class to store info of arguments,the 
// actual value with specific types will be 
// stored into derived class
class ConfigArgBase {
public:
    using ptr = std::shared_ptr<ConfigArgBase>;

    ConfigArgBase(const std::string& name, const std::string& description = "")
        : m_name{ name },
        m_description { description }
    {   
        // convert all uppercase char to lowercase
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    };

    virtual ~ConfigArgBase () {};

    const std::string& getName () const { return m_name; };
    const std::string& getDescription () const { return m_description; };

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;        // name of argument

    std::string m_description; // argument description
};

// template class for basic type convertion (F fromType, T toType )
template<typename F, typename T>
class LexicalCast {
public:
    T operator() (const F& v) {
        return boost::lexical_cast<T>(v);
    };
};

// partial specialized template for conversion from string to vector
template<typename T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            // recursively call LexicalCast to parse any nested data types
            vec.push_back(LexicalCast<std::string, T>() (ss.str()));
        }

        return vec;
    };
};

// partial specialized template for conversion from vector to string
template<typename T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};

// partial specialized template for conversion from string to list
template<typename T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::list<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            // recursively call LexicalCast
            vec.push_back(LexicalCast<std::string, T>() (ss.str()));
        }

        return vec;
    };
};

// partial specialized template for conversion from list to string
template<typename T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};


// partial specialized template for conversion from string to set
template<typename T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            // recursively call LexicalCast
            vec.insert(LexicalCast<std::string, T>() (ss.str()));
        }

        return vec;
    };
};

// partial specialized template for conversion from set to string
template<typename T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};

// partial specialized template for conversion from string to unordered_set
template<typename T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            // recursively call LexicalCast
            vec.insert(LexicalCast<std::string, T>() (ss.str()));
        }

        return vec;
    };
};

// partial specialized template for conversion from unordered_set to string
template<typename T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};

// partial specialized template for conversion from string to map
template<typename T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            // recursively call LexicalCast
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str())));
        }

        return vec;
    };
};

// partial specialized template for conversion from map to string
template<typename T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};


// partial specialized template for conversion from string to unordered_map
template<typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);

        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            // recursively call LexicalCast
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str())));
        }

        return vec;
    };
};

// partial specialized template for conversion from unordered_map to string
template<typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v){
        // accept vecotor(Yaml sequence) and convert to yaml format string
        YAML::Node node(YAML::NodeType::Sequence);

        for(auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};

// FromStr T operator() (const std::string&)
// ToStr std::string operator() (const T&)
template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string>>
class ConfigArg : public ConfigArgBase {
public:
    using ptr = std::shared_ptr<ConfigArg>;
    using onChangeCallBack = std::function<void (const T& oldValue, const T& newValue)>;

    ConfigArg(const std::string& name,
            const T& default_value,
            const std::string& description = "") 
            : ConfigArgBase(name, description),
            m_val(default_value) {
    };

    // convert to argument into yaml string format 
    std::string toString() override {
        try {
            return ToStr()(m_val);
        } catch (std::exception& e) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigArg::toString() exception " 
                                                << e.what() << " convert: " 
                                                << typeid(m_val).name() << " to string";
        }
        return "";
    };

    // store arguments from string
    bool fromString(const std::string& val) override {
        try {
            setValue(FromStr()(val));
        } catch (std::exception& e) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigArg::fromString() exception " 
                                                << e.what() << " convert string to " 
                                                << typeid(m_val).name();
            return false;
        }

        return true;
    };

    const T getValue() const { return m_val; };

    void setValue(const T& val) { ;
        if (m_val == val) {
            return;
        }
        
        // process each callback when reading config from yaml
        for (auto& i : m_cbs) {
            i.second(m_val, val);
        }

        m_val = val;
    };

    std::string getTypeName() const override { return typeid(T).name(); };

    void addListener(onChangeCallBack cb) { 
        static uint64_t funcId = 0;
        funcId++;
        m_cbs[funcId] = cb; 
    }

    void delListener(uint64_t key) { m_cbs.erase(key); }

    onChangeCallBack getListener(uint64_t key) {
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    void clearListener() { m_cbs.clear(); }

private:
    T m_val;    // argument could be differnt types

    std::map<uint64_t, onChangeCallBack> m_cbs;
};

// Manager to store all configuration parameters
class ConfigMgr {
public:
    using ConfigArgMap = std::unordered_map<std::string, ConfigArgBase::ptr>;

    // check if config para exists, if not, store into Mgr
    template<typename T>
    static typename ConfigArg<T>::ptr lookUp(const std::string& name, 
                                            const T& default_value, 
                                            const std::string& description = "") 
    {             
        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "requestted name is invalid " << name;
            throw std::invalid_argument(name) ;
        }

        auto it = getData().find(name);
        if (it != getData().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigArg<T>>(it->second);
            if (tmp) {
                SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "Lookup name = " << name << " exists ";
                return tmp;                
            } else {
                // different types of vlaue, cannot be converted to corresponding shared_ptr
                SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "Lookup name = " 
                            << name << " exists but type conflict: current type: <" <<  typeid(T).name()
                            <<  ">, real type: <" << it->second->getTypeName() << ">, " << it->second->toString();
                return nullptr;
            }
        }


        // store argument into mgr
        typename ConfigArg<T>::ptr nArg(new ConfigArg<T>(name, default_value, description));
        getData()[name] = nArg;
        
        return nArg;
    };  

    template<typename T> 
    static typename ConfigArg<T>::ptr lookUp(const std::string& name){
        auto it = getData().find(name);
        if (it == getData().end()) {
            return nullptr;
        }
        
        // convert shared_ptr to base class to a shared_ptr to derived class
        // Note: this would return nullptr when trying to return configurations with same key but different value types
        return std::dynamic_pointer_cast<ConfigArg<T>>(it->second);
    };
    
    // recursively load configuration parameters from yaml
    static void loadFromYaml(const YAML::Node& root);

    // look up the pointer to base arg
    static ConfigArgBase::ptr lookUpBase(const std::string& name);

private:
    static ConfigArgMap& getData() {
        static ConfigArgMap m_data;
        return m_data;
    }
};

};

#endif