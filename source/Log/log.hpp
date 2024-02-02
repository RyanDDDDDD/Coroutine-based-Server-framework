#ifndef __SERVER_LOG_H__
#define __SERVER_LOG_H__

#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>

namespace Server {

// Log (message) Event
class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;
    LogEvent ();

private:
    const char* m_file    = nullptr; // file name
    std::string m_content = "";      // message content
    uint32_t m_elapse     = 0; // milliseconds between program start and current
    int32_t m_line        = 0; // lines' number
    uint32_t m_threadId   = 0; // thread Id
    uint32_t m_fiberId    = 0; // coroutine Id
    uint64_t m_time       = 0; // time stamp
};

// Log (message) Level
class LogLevel {
public:
    enum class Level {
        DEBUG = 1,
        INFO  = 2,
        WARN  = 3,
        ERROR = 4,
        FATAL = 5
    };
};

// Log ouput destination
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;

    virtual void log (LogLevel::Level level, const LogEvent::ptr event) = 0; // output log

    void setFormatter (LogFormatter::ptr val) {
        m_formatter = val;
    };

    LogFormatter::ptr getFormatter () {
        return m_formatter;
    }

    virtual ~LogAppender (){};

protected:
    LogFormatter::ptr m_formatter; // format output to destination
    LogLevel::Level m_level; // Minimum level of Log the Appender can process
};

// Log formatter
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;

    std::string format (LogEvent::ptr); // format event to string
private:
    class FormatItem {
    public:
        virtual ~FormatItem (){};

        virtual std::string format () = 0;
    };
};

// Logger
class Logger {
public:
    using ptr = std::shared_ptr<Logger>;

    Logger ();
    Logger (const std::string& name = "root");

    void log (LogLevel::Level level, const LogEvent::ptr event); // use appender to output log

    void debug (LogEvent::ptr event);
    void info (LogEvent::ptr event);
    void warn (LogEvent::ptr event);
    void error (LogEvent::ptr event);
    void fatal (LogEvent::ptr event);

    void addAppender (LogAppender::ptr appender);
    void delAppender (LogAppender::ptr appender);

    LogLevel::Level getLevel () const {
        return m_level;
    };

    void setLevel (LogLevel::Level val) {
        m_level = val;
    };

private:
    LogLevel::Level m_level; // Requested Level of Log which can be processed by Logger
    std::string m_name;                      // logger name
    std::list<LogAppender::ptr> m_appenders; // set of appender to output log
};

// Output to console
class StdoutLogAppender : public LogAppender {
public:
    using ptr = std::shared_ptr<StdoutLogAppender>;
    virtual void log (LogLevel::Level level, LogEvent::ptr event) override;
};

// output to file
class FileLogAppender : public LogAppender {
public:
    FileLogAppender (const std::string& filename);

    virtual void log (LogLevel::Level level, LogEvent::ptr event) override;

    bool reopen (); // if the output file is opened, close and open it

private:
    std::string m_filename;     // opened file
    std::ofstream m_filestream; // file stream binded to opened file
};

} // namespace Server

#endif