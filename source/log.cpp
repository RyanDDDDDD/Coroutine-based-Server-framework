#include "log.hpp"
#include <functional>
#include <map>

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
            return LogLevel::Level::level; \
            
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
            os << logger->getName();
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
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") :m_format{format} {};

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
        StringFormatItem(const std::string &str) : FormatItem(str), m_string(str){};

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
                        uint32_t threadId,uint32_t fiberId, uint64_t time)
        : m_file{ file },
        m_line{ line },
        m_elapse{ elapse },
        m_threadId{ threadId },
        m_fiberId{ fiberId },
        m_time{ time },
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
        m_level{LogLevel::Level::DEBUG} {
            
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%f:%l%T%m%n"));
    };

    void Logger::addAppender(LogAppender::ptr appender) {
        // if the formatter is not set, assign the logger formatter to appender
        if (!appender -> getFormatter()) {          
            appender -> setFormatter(m_formatter);
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

    void Logger::log(LogLevel::Level level, const LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            
            for (auto &appender : m_appenders) {
                appender->log(self, level, event);
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

    FileLogAppender::FileLogAppender (const std::string& filename): m_filename{ filename } {
        reopen();
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
            std::cout << m_formatter->format(logger, level, event);
        }
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
            }
        }

        if (!nstr.empty()) {        // no matching format in string
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {

#define XX(str, C) \
    {#str,[](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }}

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
            if (std::get<2>(i) == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));   // raw string
            }
            else {
                auto it = s_format_items.find(std::get<0>(i)); // current string contains pattern

                if (it == s_format_items.end()) {               // error pattern, which is <<pattern_error>>
                    m_items.push_back(FormatItem::ptr(
                        new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
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
    m_root.reset(new Logger);

    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));   // default appender for logger
};

Logger::ptr LoggerManager::getLogger(const std::string& name){
    auto it = m_loggers.find(name);
    return it == m_loggers.end() ? m_root : it->second;
};

bool LoggerManager::storeLogger(const std::string label, const Logger::ptr logger) {
    if (m_loggers.find(label) != m_loggers.end()) {
        std::cout << "label exists: " << label << std::endl;
        return false;
    } 

    m_loggers[label] = logger;
    return true;
};

}; // namespace Server
