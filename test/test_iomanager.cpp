#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "../mysylar/iomanager.h"
#include "../mysylar/mysylar.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

int sock = 0;

void test_fiber() {
  MYSYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

  // Create a new socket using IPv4, stream socket, and default protocol (TCP)
  sock = socket(AF_INET, SOCK_STREAM, 0);
  // Set the socket to non-blocking mode
  fcntl(sock, F_SETFL, O_NONBLOCK);

  // Initialize sockaddr_in structure with server IP and port (80 for HTTP)
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  // Convert IP address from text to binary form
  inet_pton(AF_INET, "183.240.98.198", &addr.sin_addr.s_addr);

  // Attempt to connect to the server
  if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
  }
  // 正在连接中大概要等i++3000多次
  else if (errno == EINPROGRESS) {
    MYSYLAR_LOG_INFO(g_logger)
        << "add event errno=" << errno << " " << strerror(errno);
    // Add read event to the event loop
    mysylar::IOManager::GetThis()->addEvent(
        sock, mysylar::IOManager::READ,
        []() { MYSYLAR_LOG_INFO(g_logger) << "read callback"; });
    // Add write event to the event loop
    // 连接完成时，触发写事件，关闭socket
    mysylar::IOManager::GetThis()->addEvent(
        sock, mysylar::IOManager::WRITE, []() {
          MYSYLAR_LOG_INFO(g_logger) << "write callback";
          // Cancel read event and close the socket
          // 触发读事件。
          mysylar::IOManager::GetThis()->cancelEvent(sock,
                                                     mysylar::IOManager::READ);
          close(sock);
        });
  } else {
    MYSYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
  }
}

void test1() {
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  mysylar::IOManager iom(2, false);
  iom.schedule(&test_fiber);
}

mysylar::Timer::ptr s_timer;
void test_timer() {
  mysylar::IOManager iom(2);
  // 添加一个定时器,每隔1s执行一次,执行3次后,每隔2s执行一次
  s_timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        MYSYLAR_LOG_INFO(g_logger) << "hello timer i = " << i;
        if (++i == 3) {
          s_timer->reset(2000, true);
          // s_timer->cancel();
        }
      },
      true);
}

int main(int argc, char **argv) {
  test1();
  // test_timer();
  return 0;
}
