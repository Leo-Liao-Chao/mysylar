#include "../mysylar/socket.h"
#include "../mysylar/mysylar.h"
#include "../mysylar/iomanager.h"

static mysylar::Logger::ptr g_looger = MYSYLAR_LOG_ROOT();

void test_socket() {
    //std::vector<mysylar::Address::ptr> addrs;
    //mysylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    //mysylar::IPAddress::ptr addr;
    //for(auto& i : addrs) {
    //    SYLAR_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<mysylar::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    // 默认ipv4 type=0, protocol=0
    mysylar::IPAddress::ptr addr = mysylar::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        MYSYLAR_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        MYSYLAR_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    mysylar::Socket::ptr sock = mysylar::Socket::CreateTCP(addr);
    addr->setPort(80);
    MYSYLAR_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if(!sock->connect(addr)) {
        MYSYLAR_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        MYSYLAR_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        MYSYLAR_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        MYSYLAR_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }
    MYSYLAR_LOG_INFO(g_looger) << "rt=" << rt ;
    buffs.resize(rt);
    MYSYLAR_LOG_INFO(g_looger) << buffs;
}

void test2() {
    mysylar::IPAddress::ptr addr = mysylar::Address::LookupAnyIPAddress("www.baidu.com:80");
    if(addr) {
        MYSYLAR_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        MYSYLAR_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    mysylar::Socket::ptr sock = mysylar::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        MYSYLAR_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        MYSYLAR_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = mysylar::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            MYSYLAR_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    MYSYLAR_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    MYSYLAR_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = mysylar::GetCurrentUS();
            MYSYLAR_LOG_INFO(g_looger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    mysylar::IOManager iom;
    // iom.schedule(&test_socket);
    iom.schedule(&test2);
    return 0;
}
