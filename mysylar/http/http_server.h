/**
 * @file http_server.h
 * @brief HTTP服务器封装
 */

#ifndef __MYSYLAR_HTTP_HTTP_SERVER_H__
#define __MYSYLAR_HTTP_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "./http_session.h"
#include "./servlet.h"

namespace mysylar {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
 public:
  /// 智能指针类型
  typedef std::shared_ptr<HttpServer> ptr;

  /**
   * @brief 构造函数
   * @param[in] keepalive 是否长连接
   * @param[in] worker 工作调度器
   * @param[in] accept_worker 接收连接调度器
   */
  HttpServer(bool keepalive = false,
             mysylar::IOManager *worker = mysylar::IOManager::GetThis(),
             mysylar::IOManager *accept_worker = mysylar::IOManager::GetThis());

  ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
  void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

 protected:
  virtual void handleClient(Socket::ptr client) override;

 private:
  /// 是否支持长连接
  bool m_isKeepalive;
  ServletDispatch::ptr m_dispatch;
};

}  // namespace http
}  // namespace mysylar

#endif
