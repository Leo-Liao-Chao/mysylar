#include <iostream>
#include "../mysylar/log.h"
#include "../mysylar/util/util.h"

int main(int argc, char **argv)
{
    mysylar::Logger::ptr logger(new mysylar::Logger);
    logger->addAppender(mysylar::LogAppender::ptr(new mysylar::StdoutLogAppender));

    mysylar::FileLogAppender::ptr file_appender(new mysylar::FileLogAppender("./log.txt"));
    mysylar::LogFormatter::ptr fmt(new mysylar::LogFormatter("%d%T%m%n"));
    file_appender->setFormatter(fmt);
    logger->addAppender(file_appender);
    file_appender->setLevel(mysylar::LogLevel::ERROR);

    // logger->addAppender(file_appender);

    
    // mysylar::LogEvent::ptr event(new mysylar::LogEvent(logger,mysylar::LogLevel::DEBUG,__FILE__, __LINE__, 0, mysylar::GetThreadId(), mysylar::GetFiberId(), time(0)));
    // event->getSS() << "Hello mysylar log";
    // logger->log(mysylar::LogLevel::DEBUG, event);

    // std::cout << "Hello mysylar log " << std::endl;
    MYSYLAR_LOG_INFO(logger) << "test macro";
    // MYSYLAR_LOG_FMT_DEBUG(logger,"test macro fmt debug %s","aa");
    MYSYLAR_LOG_FMT_ERROR(logger,"test macro fmt debug %s","aa");

    auto l = mysylar::LoggerMgr::GetInstance()->getLogger("xx");
    MYSYLAR_LOG_INFO(l) <<"xxx";
    return 0;
}