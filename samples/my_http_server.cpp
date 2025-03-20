#include "../mysylar/http/http_server.h"
#include "../mysylar/log.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();
mysylar::IOManager::ptr worker;
void run() {
    // g_logger->setLevel(mysylar::LogLevel::INFO);
    mysylar::Address::ptr addr = mysylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        MYSYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    // mysylar::http::HttpServer::ptr http_server(new mysylar::http::HttpServer(true, worker.get()));
    // 支持长连接
    mysylar::http::HttpServer::ptr http_server(new mysylar::http::HttpServer(true));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        MYSYLAR_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
        //http_server->loadCertificates("/home/apps/soft/mysylar/keys/server.crt", "/home/apps/soft/mysylar/keys/server.key");
    }

    http_server->start();
}

int main(int argc, char** argv) {
    mysylar::IOManager iom(1);
    // worker.reset(new mysylar::IOManager(4, false));
    iom.schedule(run);
    return 0;
}
