#include "../mysylar/address.h"
#include "../mysylar/log.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void test() {
    std::vector<mysylar::Address::ptr> addrs;

    // MYSYLAR_LOG_INFO(g_logger) << "begin";
    // bool v = mysylar::Address::Lookup(addrs, "localhost:3080");
    bool v = mysylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // bool v = mysylar::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    // MYSYLAR_LOG_INFO(g_logger) << "end";
    if(!v) {
        MYSYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        MYSYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = mysylar::Address::LookupAny("localhost:4080");
    if(addr) {
        MYSYLAR_LOG_INFO(g_logger) << *addr;
    } else {
        MYSYLAR_LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<mysylar::Address::ptr, uint32_t> > results;

    // 默认AF_INT,AF_INT6
    bool v = mysylar::Address::GetInterfaceAddresses(results,AF_INET6);
    if(!v) {
        MYSYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        MYSYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    // auto addr = mysylar::IPAddress::Create("www.sylar.top");
    auto addr = mysylar::IPAddress::Create("www.baidu.com");
    // auto addr = mysylar::IPAddress::Create("127.0.0.8");
    if(addr) {
        MYSYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();//pass // hints.ai_flags = AI_CANONNAME;address.cpp
    // test_iface();// 成功,ip address 查看地址,
    // test();// 成功
    return 0;
}
