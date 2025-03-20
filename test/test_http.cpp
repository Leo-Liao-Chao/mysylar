#include "../mysylar/http/http.h"
#include "../mysylar/log.h"

void test_request() {
    mysylar::http::HttpRequest::ptr req(new mysylar::http::HttpRequest);
    req->setHeader("host" , "www.sylar.top");
    req->setBody("hello sylar");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    mysylar::http::HttpResponse::ptr rsp(new mysylar::http::HttpResponse);
    rsp->setHeader("X-X", "sylar");
    // rsp->setBody("hello sylar");
    rsp->setStatus((mysylar::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    std::cout<<"-------------------"<<std::endl;
    test_response();
    return 0;
}
