#include "log.hpp"

namespace Server {
Logger::Logger (){};
Logger::Logger (const std::string& name = "root")
 : m_name{ name } {};

void Logger::addAppender (LogAppender::ptr appender) {
    m_appenders.push_back (appender);
};

void Logger::delAppender (LogAppender::ptr appender) {
    for (auto it = m_appenders.begin (); it != m_appenders.end (); it++) {
        if (*it == appender) {
            m_appenders.erase (it);
            break;
        }
    }
};

void Logger::log (LogLevel::Level level, const LogEvent::ptr event) {
    if (level >= m_level) {
        for (auto& appender : m_appenders) {
            appender->log (level, event);
        }
    }
};

void Logger::debug (LogEvent::ptr event) {
    debug (LogLevel::DEBUG, event);
};

void Logger::info (LogEvent::ptr event) {
    debug (LogLevel::INFO, event);
};

void Logger::warn (LogEvent::ptr event) {
    debug (LogLevel::ERROR, event);
};

void Logger::fatal (LogEvent::ptr event) {
    debug (LogLevel::FATAL, event);
};

FileLogAppender::FileLogAppender (const std::string& filename)
 : m_filename{ filename } {

 };

bool FileLogAppender::reopen () {
    if (m_filestream) {
        m_filestream.close ();
    }

    m_filestream.open (m_filename);
    return !m_filestream; // file is opened successfully
};

void StdoutLogAppender::log (LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        std::cout << m_formatter->format (event) << std::endl;
    }
};

void FileLogAppender::log (LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        m_filestream << m_formatter->format (event); // output the formatted string into file
    }
};

}; // namespace Server
