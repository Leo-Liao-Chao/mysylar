#include "./log.h"
#include "./config.h"

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
            return "UNKNOWN";
            break;
        }
        return "UNKNOWN";
    }
    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL)
        return LogLevel::UNKOWN;
#undef XX
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                       uint32_t threadId, uint32_t fiberId, uint64_t time) : m_file(file), m_line(line), m_elapse(elapse), m_threadId(threadId), m_fiberId(fiberId), m_time(time), m_logger(logger), m_level(level)
    {
    }
    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }
    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }
    
    LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e)
    {
    }
    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }
    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
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
            os << event->getLogger()->getName();
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
            if (m_format.empty())
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
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);

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
    class TabFormatItem : public LogFormatter::FormatterItem
    {
    public:
        TabFormatItem(const std::string &str = "")
        {
        }
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::Level::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }
    void Logger::setFormatter(LogFormatter::ptr val)
    {
        m_formatter = val;

        for (auto &i : m_appenders)
        {
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }
    void Logger::setFormatter(const std::string &val)
    {
        mysylar::LogFormatter::ptr new_val(new mysylar::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name=" << m_name << " value=" << val << " invalid formatter" << std::endl;
            return;
        }
        // m_formatter = new_val;
        setFormatter(new_val);
    }
    std::string Logger::toYamlString()
    {
        YAML::Node node;
        node["name"] = m_name;
        if (m_level != LogLevel::UNKOWN)
        {
            node["level"] = LogLevel::ToString(m_level);
        }

        if (m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
    LogFormatter::ptr Logger::getFormatter()
    {
        return m_formatter;
    }
    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->m_formatter = m_formatter;
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
    void Logger::clearAppenders()
    {
        m_appenders.clear();
    }
    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            if (!m_appenders.empty())
            {
                for (auto &i : m_appenders)
                {
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                m_root->log(level, event);
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
        // str,format,type（1代表格式化字符串，0代表无意字符串）
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            // 如果当前字符不是%，则追加到nstr。
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // 如果遇到%，检查下一个字符是否也是%。如果是，将%%视为普通文本追加到nstr。
            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }
            // 如果下一个字符不是%，则开始解析格式化项。解析包括一个可选的字符串（str）和一个格式（fmt）
            size_t n = i + 1;
            int fmt_status = 0;
            int fmt_begin = 0;
            std::string str;
            std::string fmt;

            while (n < m_pattern.size())
            {
                // 没有进入解析状态，并且不是字母，不是{}的所有字符串
                if (!fmt_status && (!std::isalpha(m_pattern[n])) && m_pattern[n] != '{' && m_pattern[n] != '}')
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                // 如果格式项以{开始，以}结束，那么解析这个范围内的文本作为格式字符串。
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        // 进入解析状态，{前 所有字符串str
                        str = m_pattern.substr(i + 1, n - i - 1);
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
                        // 格式化字符串fmt
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1); //{格式字符串}
                        fmt_status = 2;
                        ++n;
                        break;
                    }
                }
                ++n;
            }
            // 如果fmt_status为0，表示没有解析到格式化项，将累积的nstr作为一个普通字符串项添加到vec。
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
            // 如果fmt_status为1或2，表示解析到了格式化项，将str和fmt添加到vec。
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error" << m_pattern << "~" << m_pattern.substr() << std::endl;
                m_error = true;
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
        // 使用std::map定义了一个名为s_format_items的静态变量，它将格式化项的标识符映射到一个函数，这个函数根据格式字符串创建具体的格式化项对象。
        static std::map<std::string, std::function<FormatterItem::ptr(const std::string &str)>> s_format_items = {
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
            XX(l, LineFormatItem),
            XX(T, TabFormatItem),
            XX(F, FiberIdFormatItem)
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
            // 无意字符串 string(%s) string(内容) 0
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatterItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                // 如果在vec中找到的项在s_format_items中没有对应的函数，创建一个表示错误格式的StringFormatItem对象。
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatterItem::ptr(new StringFormatItem("<error_format % " + std::get<0>(i) + ">")));
                    m_error = true;
                }
                else
                {
                    // 根据str创建对应Item
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
        // std::cout << m_items.size() << std::endl;
    }

    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        m_formatter = val;
        if (m_formatter)
        {
            m_hasFormatter = true;
        }
        else
        {
            m_hasFormatter = false;
        }
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
    std::string StdoutLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKOWN)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename)
    {
        reopen();
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
    std::string FileLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if (m_level != LogLevel::UNKOWN)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (m_hasFormatter && m_formatter)
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

        m_loggers[m_root->m_name] = m_root;

        init();
    }
    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }
    std::string LoggerManager::toYamlString()
    {
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
    void LoggerManager::init()
    {
    }
    //Load yaml 
    struct LogAppenderDefine
    {
        int type = 0; // 1 File, 2 Stdout
        LogLevel::Level level = LogLevel::UNKOWN;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type && level == oth.type && file == oth.file;
        }
    };
    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKOWN;
        std::string formatter;

        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
        }
        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }
    };
    template <>
    class LexicalCast<std::string, std::set<LogDefine>>
    {
    public:
        std::set<LogDefine> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::set<LogDefine> vec;
            for (size_t i = 0; i < node.size(); i++)
            {
                auto n = node[i];
                if (!n["name"].IsDefined())
                {
                    std::cout << " log config error: name is null, " << n << std::endl;
                    continue;
                }
                LogDefine ld;
                // if(n["name"].IsScalar())
                ld.name = n["name"].as<std::string>();
                ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
                if (n["formatter"].IsDefined())
                {
                    ld.formatter = n["formatter"].as<std::string>();
                }
                if (n["appenders"].IsDefined())
                {
                    // std::cout<<"=="<<ld.name<<" = "<<n["appenders"].size()<<std::endl;
                    for (size_t x = 0; x < n["appenders"].size(); x++)
                    {
                        auto a = n["appenders"][x];
                        if (!a["type"].IsDefined())
                        {
                            std::cout << " log config error: appender is null, " << a << std::endl;
                            continue;
                        }

                        std::string type = a["type"].as<std::string>();
                        LogAppenderDefine lad;
                        if (type == "FileLogAppender")
                        {
                            lad.type = 1;
                            if (!a["file"].IsDefined())
                            {
                                std::cout << " log config error: fileappender file is null, " << a << std::endl;
                                continue;
                            }
                            lad.file = a["file"].as<std::string>();
                            if (a["formatter"].IsDefined())
                            {
                                lad.formatter = a["formatter"].as<std::string>();
                            }
                        }
                        else if (type == "StdoutLogAppender")
                        {
                            lad.type = 2;
                        }
                        else
                        {
                            std::cout << " log config error: appender type is invalid, " << a << std::endl;
                            continue;
                        }
                        ld.appenders.push_back(lad);
                    }
                }
                // std::cout<<"---"<<ld.name<<" - "<<ld.appenders.size()<<std::endl;

                vec.insert(ld);
            }
            return vec;
        }
    };
    template <>
    class LexicalCast<std::set<LogDefine>, std::string>
    {
    public:
        std::string operator()(const std::set<LogDefine> &v)
        {
            YAML::Node node;
            for (auto &i : v)
            {
                YAML::Node n;
                n["name"] = i.name;
                if (i.level != LogLevel::UNKOWN)
                {
                    n["level"] = LogLevel::ToString(i.level);
                }
                if (!i.formatter.empty())
                {
                    n["formatter"] = i.formatter;
                }
                for (auto &a : i.appenders)
                {
                    YAML::Node na;
                    if (a.type == 1)
                    {
                        na["type"] = "FileLogAppender";
                        na["file"] = a.file;
                    }
                    else if (a.type == 2)
                    {
                        na["type"] = "StdoutLogAppender";
                    }
                    if (a.level != LogLevel::UNKOWN)
                    {
                        na["level"] = LogLevel::ToString(a.level);
                    }

                    if (!a.formatter.empty())
                    {
                        n["formatter"] = a.formatter;
                    }

                    n["appenders"].push_back(na);
                }
                node.push_back(n);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    mysylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = mysylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");
    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value)
                                       {
                                           MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "on_logger_conf_changed";
                                           for (auto &i : new_value)
                                           {
                                               auto it = old_value.find(i);
                                               mysylar::Logger::ptr logger;
                                               if (it == old_value.end())
                                               {
                                                   // new logger
                                                   logger = MYSYLAR_LOG_NAME(i.name);
                                               }
                                               else
                                               {
                                                   if (!(i == *it))
                                                   {
                                                       // changed logger
                                                       mysylar::Logger::ptr logger = MYSYLAR_LOG_NAME(i.name);
                                                   }
                                               }
                                               logger->setLevel(i.level);
                                               if (!i.formatter.empty())
                                               {
                                                   logger->setFormatter(i.formatter);
                                               }
                                               logger->clearAppenders();
                                               for (auto &a : i.appenders)
                                               {
                                                   mysylar::LogAppender::ptr ap;
                                                   if (a.type == 1)
                                                   {
                                                       ap.reset(new FileLogAppender(a.file));
                                                   }
                                                   else if (a.type == 2)
                                                   {
                                                       ap.reset(new StdoutLogAppender);
                                                   }
                                                   ap->setLevel(a.level);
                                                   if(!a.formatter.empty())
                                                   {
                                                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                                                        if(fmt->isError())
                                                        {
                                                            ap->setFormatter(fmt);
                                                        }
                                                        else{
                                                            std::cout<<"logger name="<<i.name<<"appender type = "<<a.type<<"formatter ="<<a.formatter<<" is invalid"<<std::endl;
                                                        }
                                                   }
                                                   logger->addAppender(ap);
                                               }
                                           }
                                           for (auto &i : old_value)
                                           {
                                               auto it = new_value.find(i);
                                               if (it == new_value.end())
                                               {
                                                   // delete logger
                                                   auto logger = MYSYLAR_LOG_NAME(i.name);
                                                   logger->setLevel((LogLevel::Level)100); // Todo:
                                                   logger->clearAppenders();
                                               }
                                           } });
        }
    };
    static LogIniter __log_init;

    
}