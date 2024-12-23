#include "../mysylar/mysylar.h"
#include "../mysylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

int sock = 0;

void test_fiber()
{
    MYSYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    // sleep(3);

    // close(sock);
    // sylar::IOManager::GetThis()->cancelAll(sock);

    // 这行代码创建一个新的socket，使用IPv4（AF_INET），面向流的套接字（SOCK_STREAM），并设置协议为0（让系统选择默认协议，通常是TCP）。
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // 设置为非阻塞模式，这样在执行网络操作时，如果操作不能立即完成，不会阻塞线程。
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // 初始化一个 sockaddr_in 结构体，设置服务器的IP地址和端口号（80端口，通常是HTTP服务的端口）。inet_pton 函数将点分十进制的IP地址转换为网络字节序。
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // baidu
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);
    // 这行代码尝试连接到服务器。如果连接成功，将不会执行任何操作。
    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
        std::cout << "Success" << std::endl;
    }
    // 如果连接操作没有立即完成（EINPROGRESS），则记录日志，并添加读和写事件到事件循环中：
    // 读事件的回调函数记录一条信息级别的日志。
    // 写事件的回调函数记录一条信息级别的日志，然后取消读事件，并关闭socket。
    else if (errno == EINPROGRESS)
    {
        MYSYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::READ, []()
                                                { MYSYLAR_LOG_INFO(g_logger) << "read callback"; });
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::WRITE, []()
                                                {
            MYSYLAR_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            mysylar::IOManager::GetThis()->cancelEvent(sock, mysylar::IOManager::READ);
            close(sock); });
    }
    // 如果连接操作失败，并且错误不是 EINPROGRESS，则记录错误信息。
    else
    {
        MYSYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    std::cout << "EPOLLIN=" << EPOLLIN
              << " EPOLLOUT=" << EPOLLOUT << std::endl;
    mysylar::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}

// mysylar::Timer::ptr s_timer;
// void test_timer() {
//     mysylar::IOManager iom(2);
//     s_timer = iom.addTimer(1000, [](){
//         static int i = 0;
//         MYSYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
//         if(++i == 3) {
//             s_timer->reset(2000, true);
//             //s_timer->cancel();
//         }
//     }, true);
// }

int main(int argc, char **argv)
{
    test1();
    // test_timer();
    return 0;
}
