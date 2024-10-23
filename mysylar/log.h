#ifndef __MYSYLAR_LOG_H__
#define __MYSYLAR_LOG_H__

#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <stringstream>
#include <fstream>

namespace mysylar
{
    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent();

    private:
        const char *m_file = nullptr; // 文件名
        int32_t m_line = 0;           // 行号
        uint32_t m_elapse = 0;        // 程序启动到现在的毫秒数，单位 ms
        uint32_t m_threadId = 0;      // 线程Id
        uint32_t m_fiberId = 0;       // 协程Id
        uint64_t m_time = 0;          // 时间戳
        std::string m_content;        // 内容
    };
    // 日志输出等级
    class LogLevel
    {
    public:
        enum Level
        {
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

    private:
    };
    // 日志输出格式
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        std::string format(LogEvent::ptr event);

    private:
    };
    // 日志输出地
    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {};
        virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

        void setFormatter(LogFormatter::ptr val) { m_formatter = val };

    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;
    };
    // 日志器
    class Loggger
    {
    public:
        typedef std::shared_ptr<Loggger> ptr;

        Loggger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() { return m_level; };
        void setLevel(LogLevel::Level val) { m_level = val; };

    private:
        std::string m_name;                 // 日志名称
        LogLevel::Level m_level;            // 日志等级
        std::list<LogAppender> m_appenders; // Appender集合
    };
    // 定义输出到控制它的Appender
    class StdoutLogAppender : public LogAppender
    {
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        virtual void log(LogLevel::Level level, LogEvent::ptr event) override;
    };
    // 定义输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        virtual void log(LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();
    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

}

#endif
