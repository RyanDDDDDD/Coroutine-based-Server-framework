#include "log.hpp"
#include "config.hpp"
#include <map>
#include <functional>
#include <iostream>
#include <time.h>
#include <string.h>

namespace Server {

    const char *LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
#define XX(name)                \
    case LogLevel::Level::name: \
        return #name;
            break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKNOWN";
        }

        return "UNKNOWN";
    };


    LogLevel::Level LogLevel::FromString(const std::string& str) {
    #define XX(level, v)                \
        if (str == #v) \
            return LogLevel::Level::level \
            
            XX(DEBUG, debug);
            XX(INFO, info);
            XX(WARN, warn);
            XX(ERROR, error);
            XX(FATAL, fatal);

            XX(DEBUG, DEBUG);
            XX(INFO, INFO);
            XX(WARN, WARN);
            XX(ERROR, ERROR);
            XX(FATAL, FATAL);
    #undef XX
        
        return LogLevel::Level::UNKNOWN;
    };

    LogEventWrap::LogEventWrap(LogEvent::ptr e): m_event{ e } {};
    
    std::stringstream& LogEventWrap::getSS() {
        return m_event->getSS();
    }

    LogEventWrap::~LogEventWrap() {
        // output log when deconstuct the wrapper
        m_event->getLogger()->log(m_event->getLevel(), m_event);    
    };

    LogFormatter::LogFormatter (const std::string& pattern): m_pattern{ pattern } {
        init();
    };

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        MessageFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        LevelFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem {
    public:
        NameFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        FiberIdFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") :m_format{ format } {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        };

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            
            // Converts given time since epoch into calendar time, expressed in local time
            struct tm tm;
            time_t time = event->getTime(); 
            localtime_r(&time, &tm);

            // returns a string representing date and time using pre-defined format
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);

            os << buf;
        };

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem {
    public:
        FilenameFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        };
    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        };
    };

    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string& str = ""){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
        };
    };

    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string &str) : m_string(str){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        };

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string &str){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << '\t';
        };
    };

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                        const char* file, int32_t line, uint32_t elapse, 
                        uint32_t threadId,uint32_t fiberId, uint64_t time,
                        const std::string& threadName)
        : m_file{ file },
        m_line{ line },
        m_elapse{ elapse },
        m_threadId{ threadId },
        m_fiberId{ fiberId },
        m_time{ time },
        m_threadName{ threadName },
        m_logger{ logger },
        m_level{ level }
    {};
    
    // accepet variable arguments to pass into format(const char* fmt, va_list al), rewrite later, using variadic template
    void LogEvent::format(const char* fmt, ...) {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    // provide user interface to setup user-defined format
    void LogEvent::format(const char* fmt, va_list al) {
        char* buf = nullptr;
        
        int len = vasprintf(&buf, fmt, al); // 

        if (len != -1) {
            m_ss << std::string(buf, len);
            free(buf);
        }
    };

    Logger::Logger (const std::string& name)
        : m_name{ name },
        m_level{LogLevel::Level::DEBUG},
        m_formatter{std::make_shared<LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")} {
        // m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

    };

    void Logger::setFormatter(const std::string& val) {
        Server::LogFormatter::ptr newVal(std::make_shared<Server::LogFormatter>(val));
        // this is an error pattern
        if (newVal->isError()) {
            std::cout << "Logger setFormatter failed, name = " << m_name
                    << " value = " << val << " invalid formatter"
                    << std::endl;
            return;
        }

        setFormatter(newVal);
    };

    void Logger::setFormatter(LogFormatter::ptr val) {
        m_formatter = val;

        for (auto& i : m_appenders) {
            // if the appender doesnt contain formatter, assign it with logger' formatter
            if (!i->m_hasFormatter){
                i->m_formatter = m_formatter;
            }
        }
    };

    // convert Logger member to yaml style string
    std::string Logger::toYamlString() {
        YAML::Node node;
        node["name"] = m_name;
        
        if(m_level != LogLevel::Level::UNKNOWN) {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }

        for (auto& i : m_appenders) {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }

        std::stringstream ss;
        ss << node;
        
        return ss.str();
    };

    LogFormatter::ptr Logger::getFormatter() {
        return m_formatter;
    };

    void Logger::addAppender(LogAppender::ptr appender) {
        // if the formatter is not set, assign the logger formatter to appender
        if (!appender -> getFormatter()) {          
            appender->m_formatter = m_formatter;
            // appender->setFormatter(m_formatter);
        }

        m_appenders.push_back(appender);
    };

    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    };

    void Logger::clearAppenders() {
        m_appenders.clear();
    };

    void Logger::log(LogLevel::Level level, const LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            
            if (!m_appenders.empty()) {
                // output event to different destination
                for (auto &appender : m_appenders) {
                    appender->log(self, level, event);
                }
            } else if (m_root) {
                m_root->log(level, event);
            }
        }
    };

    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::Level::DEBUG, event);
    };

    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::Level::INFO, event);
    };

    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::Level::WARN, event);
    };

    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::Level::ERROR, event);
    };

    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::Level::FATAL, event);
    };

    void LogAppender::setFormatter(LogFormatter::ptr val) {
        m_formatter = val;
        if (m_formatter) {
            m_hasFormatter = true;
        } else {
            m_hasFormatter = false;
        }
    }

    LogFormatter::ptr LogAppender::getFormatter() {
        return m_formatter;
    }

    FileLogAppender::FileLogAppender (const std::string& filename): m_filename{ filename } {
        reopen();
    };

    std::string FileLogAppender::toYamlString() {
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        
        if (m_level != LogLevel::Level::UNKNOWN) {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };

    bool FileLogAppender::reopen() {
        if (m_filestream) {
            m_filestream.close();
        }

        m_filestream.open(m_filename);  // open file
        return !m_filestream;           // file is opened successfully
    };

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_formatter->format(std::cout, logger, level, event);
        }
    };

    std::string StdoutLogAppender::toYamlString() {
        YAML::Node node;
        node["type"] = "StdoutLogAppender";

        if (m_level != LogLevel::Level::UNKNOWN) {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            if(!m_formatter->format(m_filestream, logger, level, event)) {  // output the formatted string into file
                std::cout << "error" << std::endl;
            }
        }
    };

    // stdout formatter
    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        
        for (auto &i : m_items) { // use FormatItem to format event and store into ss
            i->format(ss, logger, level, event); 
        }

        return ss.str();
    }

    // file output formatter
    std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        for(auto& i : m_items) {
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }

    // set up formatItem according to the given pattern
    // would be rewrite by using boost::regex later
    void LogFormatter::init() {
        std::vector<std::tuple<std::string, std::string, int>> vec; // str, format ,type, if the type is 1, which means, string is %xxx or %xxx{xxx}

        std::string nstr;

        for (size_t i = 0; i < m_pattern.size(); ++i) {
            if (m_pattern[i] != '%') {
                nstr += m_pattern[i];
                continue;
            }

            if ((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%') { // real %
                nstr += '%';
                continue;
            }

            // current string format is %xxx
            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str = "";
            std::string fmt = "";
            while (n < m_pattern.size()) {
                if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' // end of %xxx
                    && m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
               
                if (fmt_status == 0) {
                    if (m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1;             // start parsing string
                        fmt_begin = n;              // start position to parse string
                        ++n;
                        continue;
                    }
                }

                if (fmt_status == 1) {
                    if (m_pattern[n] == '}') {      // end of parsing
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }

                ++n;

                if (n == m_pattern.size()){
                    if (str.empty()){
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0) { // we never meet {}
                if (!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }

                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;      // update pointer to the last pos
            }
            else if (fmt_status == 1) {
                std::cout << "pattern error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
                m_error = true;
            }
        }

        if (!nstr.empty()) {        // no matching format in string
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {

#define XX(str, C) \
    {#str,[](const std::string &fmt) { return FormatItem::ptr(std::make_shared<C>(fmt)); }}

            XX(m, MessageFormatItem),   // %m --- message body
            XX(p, LevelFormatItem),     // %p --- priority level
            XX(r, ElapseFormatItem),    // %r --- number of milliseconds elapsed since the logger created
            XX(c, NameFormatItem),      // %c --- name of logger
            XX(t, ThreadIdFormatItem),  // %t --- thread id
            XX(n, NewLineFormatItem),   // %n --- newline char
            XX(d, DateTimeFormatItem),  // %d --- time stamp
            XX(f, FilenameFormatItem),  // %f --- file name
            XX(l, LineFormatItem),      // %l --- line number
            XX(T, TabFormatItem),       // %T --- Tab
            XX(F, FiberIdFormatItem)    // %F --- Coroutine Id
#undef XX

        };

        // start parsing the split string and convert into the given format
        for (auto &i : vec) {
            if (std::get<2>(i) == 0) { // raw string
                m_items.push_back(FormatItem::ptr(std::make_shared<StringFormatItem>(std::get<0>(i))));   
            }
            else {
                auto it = s_format_items.find(std::get<0>(i)); // current string contains pattern

                if (it == s_format_items.end()) {               // error pattern, which is <<pattern_error>>
                    m_items.push_back(FormatItem::ptr(
                        std::make_shared<StringFormatItem>("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }

            // used for debug
            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - ("
            //           << std::get<2>(i) << ")" << std::endl;
        }
    }; 

LoggerManager::LoggerManager() {
    // if (m_root == nullptr)  {std::cout << "root is null" << std::endl;} else {std::cout << "root is not null" << std::endl;};
    // m_root.reset(new Logger);
    m_root = std::make_shared<Logger>();
    m_root->addAppender(LogAppender::ptr(std::make_shared<StdoutLogAppender>()));   // default appender for logger
    
    m_loggers[m_root->m_name] = m_root;                             // store default logger

    init();
};

Logger::ptr LoggerManager::getLogger(const std::string& name){
    auto it = m_loggers.find(name);

    if (it != m_loggers.end()) {
        return it->second;
    }

    // create and store an new logger with name
    Logger::ptr logger = std::make_shared<Logger>(name);
    logger->m_root = m_root;
    m_loggers[name] = logger;

    return logger;
};

bool LoggerManager::storeLogger(const std::string label, const Logger::ptr logger) {
    if (m_loggers.find(label) != m_loggers.end()) {
        std::cout << "label exists: " << label << std::endl;
        return false;
    } 

    m_loggers[label] = logger;
    return true;
};

struct LogAppenderDefine {
    int type = 0; // 1 File, 2 Stdout
    LogLevel::Level level = LogLevel::Level::UNKNOWN;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};
    
struct LogDefine {
    std::string name ;
    LogLevel::Level level = LogLevel::Level::UNKNOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
        && level == oth.level
        && formatter == oth.formatter
        && appenders == oth.appenders;
    }
    
    bool operator!=(const LogDefine& oth) const {
        return !(*this == oth);
    }


    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

// fully specialized template for conversion from string to LogDefine
template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v){
        // accept yaml format string and convert to yaml node / node sequence
        YAML::Node node = YAML::Load(v);
        
        LogDefine ld;
        if (!node["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << node
                    << std::endl;
            throw std::logic_error("log config name is null");
        }
                
        ld.name = node["name"].as<std::string>();
        ld.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
        if (node["formatter"].IsDefined()) {
            ld.formatter = node["formatter"].as<std::string>();
        }
        
        if (node["appenders"].IsDefined()) {
            // add each appender into logDefine
            for (size_t x = 0; x < node["appenders"].size(); ++x) {
                auto a = node["appenders"][x];
                if (!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null" << a
                            << std::endl;
                    continue;
                }

                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender") {
                    lad.type = 1;
                    if (!a["file"].IsDefined()) {
                        std::cout << "log config error: fileAppender file is null, " << a
                                << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if (type == "StdoutLogAppender") {
                    lad.type = 2;
                    if (a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "Log config error: appender type is invalid, " << a
                            << std::endl;
                    continue;
                }
                
                ld.appenders.push_back(lad);
            }
        }

        return ld;
    };
};

// fully specialized template for conversion from LogDefine to string
template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i){
        // accept set(Yaml sequence) and convert to yaml format string

        YAML::Node node;
        node["name"] = i.name;
        
        if (i.level != LogLevel::Level::UNKNOWN) {
            node["level"] = LogLevel::ToString(i.level);
        }

        if (!i.formatter.empty()) {
            node["formatter"] = i.formatter;
        }

        for (auto& a : i.appenders) {
            YAML::Node na;
            if (a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if (a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
        
            if (a.level != LogLevel::Level::UNKNOWN) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if (!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            node["appenders"].push_back(na);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    };
};

// define loggers configuration before main()
Server::ConfigArg<std::set<LogDefine>>::ptr g_log_defines = 
    Server::ConfigMgr::lookUp("logs", std::set<LogDefine>(), "logs config");

// intialize before main() to config logger, using logs.yaml
struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& oldValue, 
                                            const std::set<LogDefine>& newValue) {
            
            SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "Logger configuration loaded";

            // new log config added/modified
            for (auto& i : newValue) {
                auto it = oldValue.find(i);
                Server::Logger::ptr logger;
                if (it == oldValue.end()) {
                    // new logger, create an new logger setup in logger manager
                    logger = SERVER_LOG_NAME(i.name);
                } else {
                    if (i != *it) {
                        // logger is modified, retrieve it from logger manager and modify its content
                        logger = SERVER_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                
                logger->setLevel(i.level);
                if (!i.formatter.empty()) {
                    // new formatter in config.yaml
                    logger->setFormatter(i.formatter);
                }
                
                // reset all appenders
                logger->clearAppenders();
                for (auto& a : i.appenders) {
                    Server::LogAppender::ptr ap;

                    if (a.type == 1) {
                        ap.reset(std::make_shared<FileLogAppender>(a.file).get());
                    } else if (a.type == 2) {
                        ap.reset(std::make_shared<StdoutLogAppender>().get());                    
                    }

                    ap->setLevel(a.level);
                    if (!a.formatter.empty()) {
                        LogFormatter::ptr fmt(std::make_shared<LogFormatter>(a.formatter));
                        if (!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "logname = " << i.name << " appender type = " << a.type
                                    << " formatter = " << a.formatter << " is invalid " << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for (auto& i : oldValue) {
                auto it = newValue.find(i);
                if (it == newValue.end()) {
                    // the logger is deleted
                    auto logger = SERVER_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }

        });
    }
};

static LogIniter __log_init;

void LoggerManager::init() {

};

std::string LoggerManager::toYamlString() {
    YAML::Node node;
    for (auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

}; // namespace Server
