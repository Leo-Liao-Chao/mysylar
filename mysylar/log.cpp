#include "./log.h"

namespace mysylar
{

    Loggger::Loggger(const std::string &name) : m_name(name) {}
    void Loggger::addAppender(LogAppender::ptr appender)
    {
        m_appenders.push_back(appender);
    }
    void Loggger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Loggger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            for (auto &i : m_appenders)
            {
                i->log(level, event);
            }
        }
    }

    void Loggger::debug(LogEvent::ptr event)
    {
        debug(LogLevel::DEBUG, event);
    }
    void Loggger::info(LogEvent::ptr event)
    {
        info(LogLevel::INFO, event);
    }
    void Loggger::warn(LogEvent::ptr event)
    {
        warn(LogLevel::WARN, event);
    }
    void Loggger::error(LogEvent::ptr event)
    {
        error(LogLevel::ERROR, event);
    }
    void Loggger::fatal(LogEvent::ptr event)
    {
        fatal(LogLevel::FATAL, event);
    }

    void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::cout << m_formatter.format(event);
        }
    }
    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);

        return !m_filestream;
    }
    void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            m_filestream << m_formatter.format(event);
        }
    }
}