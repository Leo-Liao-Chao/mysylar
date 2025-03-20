#include "../mysylar/tcp_server.h"
#include "../mysylar/iomanager.h"
#include "../mysylar/log.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void run() {
    auto addr = mysylar::Address::LookupAny("0.0.0.0:8033");
    auto addr2 = mysylar::UnixAddress::ptr(new mysylar::UnixAddress("/tmp/unix_addr"));
    std::vector<mysylar::Address::ptr> addrs;
    addrs.push_back(addr);
    addrs.push_back(addr2);

    mysylar::TcpServer::ptr tcp_server(new mysylar::TcpServer);
    std::vector<mysylar::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    mysylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
