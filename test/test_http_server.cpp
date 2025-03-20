#include "../mysylar/http/http_server.h"
#include "../mysylar/log.h"

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void run() {
    mysylar::http::HttpServer::ptr server(new mysylar::http::HttpServer(true));
    mysylar::Address::ptr addr = mysylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    // 添加服务器 /mysylar/xxx 的处理方法
    sd->addServlet("/mysylar/xx", [](mysylar::http::HttpRequest::ptr req
                ,mysylar::http::HttpResponse::ptr rsp
                ,mysylar::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });
    // 添加服务器 /mysylar/xxx 的处理方法
    sd->addGlobServlet("/mysylar/*", [](mysylar::http::HttpRequest::ptr req
                ,mysylar::http::HttpResponse::ptr rsp
                ,mysylar::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });
    server->start();
}

int main(int argc, char** argv) {
    mysylar::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
