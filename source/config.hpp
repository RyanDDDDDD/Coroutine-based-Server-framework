#ifndef __SERVER_CONFIG_HPP__
#define __SERVER_CONFIG_HPP__

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <exception>

#include "log.hpp"

namespace Server {

using boost::lexical_cast;

class ConfigArgBase {
public:
    using ptr = std::shared_ptr<ConfigArgBase>;

    ConfigArgBase(const std::string& name, const std::string& description = "")
        : m_name{ name },
        m_description { description }
    {};

    virtual ~ConfigArgBase () {};

    const std::string& getName () const { return m_name; };
    const std::string& getDescription () const { return m_description; };

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;

protected:
    std::string m_name;        // name of argument

    std::string m_description; // argument description

};

template<typename T>
class ConfigArg : public ConfigArgBase {
public:
    using ptr = std::shared_ptr<ConfigArg>;

    ConfigArg(const std::string& name,
            const T& default_value,
            const std::string& description = "") 
            : ConfigArgBase(name, description),
            m_val(default_value) {
    }

    // convert to argument into string format 
    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch (std::exception& e) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigArg::toString() exception " 
                                                << e.what() << " convert: " 
                                                << typeid(m_val).name() << " to string";
        }

        return "";
    }

    // setup argument from string
    bool fromString(const std::string& val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
        } catch (std::exception& e) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "ConfigArg::fromString() exception " 
                                                << e.what() << " convert string to " 
                                                << typeid(m_val).name();
            return false;
        }

        return true;
    }

    const T getValue() const { return m_val; };
private:
    T m_val;    // argument could be differnt types
};

// Manager to store all configuration parameters
class ConfigMgr {
public:
    using ConfigArgMap = std::map<std::string, ConfigArgBase::ptr>;

    template<typename T>
    static typename ConfigArg<T>::ptr lookUp(const std::string& name, 
                                            const T& default_value, 
                                            const std::string& description = "") 
    {             
        if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos) {
            SERVER_LOG_ERROR(SERVER_LOG_ROOT()) << "requestted name is invalid " << name;
            throw std::invalid_argument(name) ;
        }

        auto tmp = lookUp<T>(name);
        if (tmp) {
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "Lookup name = " << name << " exists ";
            return tmp;
        }

        // store argument into mgr
        typename ConfigArg<T>::ptr nArg(new ConfigArg<T>(name, default_value, description));
        m_data[name] = nArg;
        
        return nArg;
    };  

    template<typename T> 
    static typename ConfigArg<T>::ptr lookUp(const std::string& name){
        auto it = m_data.find(name);
        if (it == m_data.end()) {
            return nullptr;
        } 
        
        return std::dynamic_pointer_cast<ConfigArg<T>>(it->second);
    }

private:
    static ConfigArgMap m_data;
};

};

#endif