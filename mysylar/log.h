#ifndef __MYSYLAR_LOG_H__
#define __MYSYLAR_LOG_H__

#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <functional>
#include <map>
#include <time.h>
#include <stdarg.h>

#include "./util/util.h"
#include "./singleton.h"

#define MYSYLAR_LOG_LEVEL(logger, level)                                                                                             \
    if (logger->getLevel() <= level)                                                                                                 \
    mysylar::LogEventWrap(mysylar::LogEvent::ptr(new mysylar::LogEvent(logger, level, __FILE__, __LINE__, 0, mysylar::GetThreadId(), \
                                                                       mysylar::GetFiberId(), time(0))))                             \
        .getSS()

#define MYSYLAR_LOG_DEBUG(logger) MYSYLAR_LOG_LEVEL(logger, mysylar::LogLevel::DEBUG)
#define MYSYLAR_LOG_INFO(logger) MYSYLAR_LOG_LEVEL(logger, mysylar::LogLevel::INFO)
#define MYSYLAR_LOG_WARN(logger) MYSYLAR_LOG_LEVEL(logger, mysylar::LogLevel::WARN)
#define MYSYLAR_LOG_ERROR(logger) MYSYLAR_LOG_LEVEL(logger, mysylar::LogLevel::ERROR)
#define MYSYLAR_LOG_FATAL(logger) MYSYLAR_LOG_LEVEL(logger, mysylar::LogLevel::FATAL)

#define MYSYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                                   \
    if (logger->getLevel() <= level)                                                                                                     \
    {                                                                                                                                    \
        mysylar::LogEventWrap(mysylar::LogEvent::ptr(new mysylar::LogEvent(logger, level, __FILE__, __LINE__, 0, mysylar::GetThreadId(), \
                                                                           mysylar::GetFiberId(), time(0))))                             \
            .getEvent()                                                                                                                  \
            ->format(fmt, __VA_ARGS__);                                                                                                  \
    }

#define MYSYLAR_LOG_FMT_DEBUG(logger, fmt, ...) MYSYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define MYSYLAR_LOG_FMT_INFO(logger, fmt, ...) MYSYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define MYSYLAR_LOG_FMT_WARN(logger, fmt, ...) MYSYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define MYSYLAR_LOG_FMT_ERROR(logger, fmt, ...) MYSYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define MYSYLAR_LOG_FMT_FATAL(logger, fmt, ...) MYSYLAR_LOG_FMT_LEVEL(logger, mysylar::LogLevel::FATAL, fmt, __VA_ARGS__)

namespace mysylar
{
    class Logger;

    // 日志输出等级
    class LogLevel
    {
    public:
        enum Level
        {
            UNKOWN = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };
        static const char *ToString(LogLevel::Level level);

    private:
    };
    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, uint64_t time);

        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }
        const char *getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }     // 程序启动到现在的毫秒数，单位 ms
        uint32_t getThreadId() const { return m_threadId; } // 线程Id
        uint32_t getFiberId() const { return m_fiberId; }   // 协程Id
        uint64_t getTime() const { return m_time; }         // 时间戳

        std::stringstream &getSS() { return m_ss; }
        std::string getContent() const { return m_ss.str(); } // 内容

        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr; // 文件名
        int32_t m_line = 0;           // 行号
        uint32_t m_elapse = 0;        // 程序启动到现在的毫秒数，单位 ms
        uint32_t m_threadId = 0;      // 线程Id
        uint32_t m_fiberId = 0;       // 协程Id
        uint64_t m_time = 0;          // 时间戳
        std::stringstream m_ss;       // 内容

        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };
    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr e);
        ~LogEventWrap();

        std::stringstream &getSS();
        LogEvent::ptr getEvent() const { return m_event; }

    private:
        LogEvent::ptr m_event;
    };
    // 日志输出格式
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    public:
        class FormatterItem
        {
        public:
            typedef std::shared_ptr<FormatterItem> ptr;
            // FormatterItem(const std::string = "");
            virtual ~FormatterItem() {};
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };
        void init();

    private:
        std::string m_pattern;
        std::vector<FormatterItem::ptr> m_items;
    };
    // 日志输出地
    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {};
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        LogFormatter::ptr getFormatter() const { return m_formatter; }
        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
    };
    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string getName() const { return m_name; }

    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志等级
        std::list<LogAppender::ptr> m_appenders; // Appender集合
        LogFormatter::ptr m_formatter;
    };
    // 定义输出到控制它的Appender
    class StdoutLogAppender : public LogAppender
    {
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    };
    // 定义输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };
    // logger管理器
    class LoggerManager
    {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);
        void init();

    private:
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };
    typedef mysylar::Singleton<LoggerManager> LoggerMgr;

}

#endif
