#include <iostream>
#include "../mysylar/log.h"

int main(int argc, char **argv)
{
    std::cout << "Hello World " << std::endl;
    mysylar::Logger::ptr logger(new mysylar::Logger);
    logger->addAppender(mysylar::LogAppender::ptr(new mysylar::StdoutLogAppender));

    mysylar::LogEvent::ptr event(new mysylar::LogEvent(__FILE__, __LINE__, 0, 1, time(0)));
    event->getSS() << "Hello mysylar log";
    logger->log(mysylar::LogLevel::DEBUG, event);

    std::cout << "Hello mysylar log " << std::endl;
    return 0;
}