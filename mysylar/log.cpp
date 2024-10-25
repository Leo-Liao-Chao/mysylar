#include "./log.h"
#include <map>
#include <time.h>
namespace mysylar
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {

        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;
            break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL)
#undef XX

        default:
            return "UNKOWN";
            break;
        }
        return "UNKOWN";
    }
    class MessageFormatItem : public LogFormatter::FormatterItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };
    class LevelFormatItem : public LogFormatter::FormatterItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatterItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatterItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << logger->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatterItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatterItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    class DataTimeFormatItem : public LogFormatter::FormatterItem
    {
    public:
        DataTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
        {
            if(m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf,sizeof(buf),m_format.c_str(),&tm);
            
            os << buf;
        }

    private:
        std::string m_format;
    };
    class FilenameFormatItem : public LogFormatter::FormatterItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };
    class LineFormatItem : public LogFormatter::FormatterItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatterItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatterItem
    {
    public:
        StringFormatItem(const std::string &str) : m_string(str)
        {
        }
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    LogEvent::LogEvent(const char *file, int32_t line, uint32_t elapse,
                       uint32_t threadId, uint64_t time) : m_file(file), m_line(line), m_elapse(elapse), m_threadId(threadId), m_time(time)
    {
    }

    Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::Level::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d [%p] <%f:%l> %m %n"));
    }
    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }

        m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender)
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

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            for (auto &i : m_appenders)
            {
                i->log(self, level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }
    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            // std::string str = m_formatter->format(logger, level, event);
            // std::cout<<str<<"****"<<std::endl;
            std::cout << m_formatter->format(logger, level, event);
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
    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }
    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    void LogFormatter::init()
    {
        // log4j
        // str,format,type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        // size_t last_pos = 0;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            int fmt_begin = 0;
            std::string str;
            std::string fmt;

            while (n < m_pattern.size())
            {
                if (!std::isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')
                {
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i);
                        fmt_status = 1; // 解析格式
                        fmt_begin = n;
                        ++n;

                        continue;
                    }
                }
                if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 2;
                        // ++n;
                        break;
                        ;
                    }
                }
                ++n;
            }
            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }

                str = m_pattern.substr(i + 1, n - i - 1);
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error" << m_pattern << "~" << m_pattern.substr() << std::endl;
                vec.push_back(std::make_tuple("<pattern_error>", fmt, 1));
            }
            else if (fmt_status == 2)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
        }
        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }
        //         static std::map<std::string, FormatterItem::ptr> s_format_items = {
        // #define XX(str, C)
        //     {
        //         #str, [](const std::string &fmt) { return FormatterItem::ptr(new C(fmt)); }
        //                                                          } XX(m, MessageFormatItem),
        //                                                          XX(p, LevelFormatItem), XX(r, ElapseFormatItem), XX(c, NameFormatItem), XX(t, ThreadIdFormatItem), XX(n, NewLineFormatItem), XX(d, DataTimeFormatItem), XX(f, FilenameFormatItem), XX(l, LineFormatItem)
        // #undef XX
        //     };
        static std::map<std::string, std::function<FormatterItem::ptr(const std::string &)>> s_format_items = {
#define XX(str, C)                                                                  \
    {                                                                               \
        #str, [](const std::string &fmt) { return FormatterItem::ptr(new C(fmt)); } \
    }
            XX(m, MessageFormatItem),
            XX(p, LevelFormatItem),
            XX(r, ElapseFormatItem),
            XX(c, NameFormatItem),
            XX(t, ThreadIdFormatItem),
            XX(n, NewLineFormatItem),
            XX(d, DataTimeFormatItem),
            XX(f, FilenameFormatItem),
            XX(l, LineFormatItem)
#undef XX
        };

        //%m -- 消息体
        //%p --level
        //%r --启动后的时间
        //%c --日志名称
        //%t --线程id
        //%n --换行回车
        //%d --时间
        //%f --文件名
        //%l --行号
        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatterItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatterItem::ptr(new StringFormatItem("<error_format % " + std::get<0>(i) + ">")));
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
        std::cout << m_items.size() << std::endl;
    }
}