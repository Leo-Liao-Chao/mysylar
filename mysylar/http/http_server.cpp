#include "./http_server.h"

#include "../log.h"

namespace mysylar {
namespace http {

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, mysylar::IOManager *worker,
                       mysylar::IOManager *accept_worker)
    : TcpServer(worker, accept_worker), m_isKeepalive(keepalive) {
  m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
  MYSYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
  HttpSession::ptr session(new HttpSession(client));
  do {
    // 得到请求
    auto req = session->recvRequest();
    if (!req) {
      MYSYLAR_LOG_DEBUG(g_logger)
          << "recv http request fail, errno=" << errno
          << " errstr=" << strerror(errno) << " cliet:" << *client
          << " keep_alive=" << m_isKeepalive;
      break;
    }
    // 响应
    HttpResponse::ptr rsp(
        new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
    m_dispatch->handle(req, rsp, session);
    // req->setBody("hello sylar");
    // MYSYLAR_LOG_INFO(g_logger)<<"request:"<<std::endl<< *req;
    // MYSYLAR_LOG_INFO(g_logger)<<"response:"<<std::endl<< *rsp;
    // 发送请求
    session->sendResponse(rsp);
  } while (m_isKeepalive);
  session->close();
}

}  // namespace http
}  // namespace mysylar
