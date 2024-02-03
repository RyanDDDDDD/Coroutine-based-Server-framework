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
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            :m_format{format} {
            if (m_format.empty()){
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
        StringFormatItem(const std::string &str) : FormatItem(str), m_string(str){};

        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        };

    private:
        std::string m_string;
    };

    LogEvent::LogEvent(const char* file, int32_t line, uint32_t elapse, 
                uint32_t threadId,uint32_t fiberId, uint64_t time)
        : m_file{ file },
        m_line{ line },
        m_elapse{ elapse },
        m_threadId{ threadId },
        m_fiberId{ fiberId },
        m_time{ time }
    {};

    Logger::Logger (const std::string& name)
        : m_name{ name },
        m_level{LogLevel::Level::DEBUG} {
            
        m_formatter.reset(new LogFormatter("%d  [%p] <%f:%l> %m %n"));
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

    bool FileLogAppender::reopen() {
        if (m_filestream) {
            m_filestream.close();
        }

        m_filestream.open(m_filename);
        return !m_filestream; // file is opened successfully
    };

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            std::cout << m_formatter->format(logger, level, event);
        }
    };

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(logger, level, event); // output the formatted string into file
        }
    };

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for (auto &i : m_items) { // use FormatItem to format event and store into ss
            i->format(ss, logger, level, event); 
        }

        return ss.str();
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
                if (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}') {    // end of %xxx
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
                        fmt_status = 2;
                        break;
                    }
                }

                ++n;
            }

            if (fmt_status == 0) { // we never meet {}
                if (!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }

                str = m_pattern.substr(i + 1, n - i - 1); // store string with no pattern
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1) {
                std::cout << "pattern error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;

                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
            else if (fmt_status == 2) {
                if (!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }

                vec.push_back(std::make_tuple(str, fmt, 1)); // store whole string with the matching pattern
                i = n - 1;
            }
        }

        if (!nstr.empty()) { // no matching format in string
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
            XX(l, LineFormatItem)       // %l --- line number
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
            std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - ("
                      << std::get<2>(i) << ")" << std::endl;
        }

    }; // namespace Server   
}; // namespace Server
