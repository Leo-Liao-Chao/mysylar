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

void test_fiber() {
    MYSYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    //sleep(3);

    //close(sock);
    //sylar::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    // baidu 
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        MYSYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::READ, [](){
            MYSYLAR_LOG_INFO(g_logger) << "read callback";
        });
        mysylar::IOManager::GetThis()->addEvent(sock, mysylar::IOManager::WRITE, [](){
            MYSYLAR_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            mysylar::IOManager::GetThis()->cancelEvent(sock, mysylar::IOManager::READ);
            close(sock);
        });
    } else {
        MYSYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }

}

void test1() {
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

int main(int argc, char** argv) {
    test1();
    // test_timer();
    return 0;
}
