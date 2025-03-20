/**
 * @file http_connection.h
 * @brief HTTP客户端类
 */
#ifndef __MYSYLAR_HTTP_CONNECTION_H__
#define __MYSYLAR_HTTP_CONNECTION_H__

#include <list>

#include "../streams/socket_stream.h"
#include "../thread.h"
#include "../uri.h"
#include "./http.h"

namespace mysylar {
namespace http {

/**
 * @brief HTTP响应结果
 */
struct HttpResult {
  /// 智能指针类型定义
  typedef std::shared_ptr<HttpResult> ptr;
  /**
   * @brief 错误码定义
   */
  enum class Error {
    /// 正常
    OK = 0,
    /// 非法URL
    INVALID_URL = 1,
    /// 无法解析HOST
    INVALID_HOST = 2,
    /// 连接失败
    CONNECT_FAIL = 3,
    /// 连接被对端关闭
    SEND_CLOSE_BY_PEER = 4,
    /// 发送请求产生Socket错误
    SEND_SOCKET_ERROR = 5,
    /// 超时
    TIMEOUT = 6,
    /// 创建Socket失败
    CREATE_SOCKET_ERROR = 7,
    /// 从连接池中取连接失败
    POOL_GET_CONNECTION = 8,
    /// 无效的连接
    POOL_INVALID_CONNECTION = 9,
  };

  /**
   * @brief 构造函数
   * @param[in] _result 错误码
   * @param[in] _response HTTP响应结构体
   * @param[in] _error 错误描述
   */
  HttpResult(int _result, HttpResponse::ptr _response,
             const std::string& _error)
      : result(_result), response(_response), error(_error) {}

  /// 错误码
  int result;
  /// HTTP响应结构体
  HttpResponse::ptr response;
  /// 错误描述
  std::string error;

  std::string toString() const;
};

class HttpConnectionPool;
/**
 * @brief HTTP客户端类
 */
class HttpConnection : public SocketStream {
  friend class HttpConnectionPool;

 public:
  /// HTTP客户端类智能指针
  typedef std::shared_ptr<HttpConnection> ptr;

  /**
   * @brief 构造函数
   * @param[in] sock Socket类
   * @param[in] owner 是否掌握所有权
   */
  HttpConnection(Socket::ptr sock, bool owner = true);

  /**
   * @brief 析构函数
   */
  ~HttpConnection();

  /**
   * @brief 接收HTTP响应
   */
  HttpResponse::ptr recvResponse();

  /**
   * @brief 发送HTTP请求
   * @param[in] req HTTP请求结构
   */
  int sendRequest(HttpRequest::ptr req);

  /**
   * @brief 发送HTTP的GET请求
   * @param[in] url 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoGet(
      const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP的GET请求
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoGet(
      Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP的POST请求
   * @param[in] url 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoPost(
      const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP的POST请求
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoPost(
      Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] method 请求类型
   * @param[in] uri 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoRequest(
      HttpMethod method, const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] method 请求类型
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoRequest(
      HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] req 请求结构体
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                   uint64_t timeout_ms);

 private:
  uint64_t m_createTime = 0;
  uint64_t m_request = 0;
};

class HttpConnectionPool {
 public:
  typedef std::shared_ptr<HttpConnectionPool> ptr;
  typedef Mutex MutexType;

  static HttpConnectionPool::ptr Create(const std::string& uri,
                                        const std::string& vhost,
                                        uint32_t max_size,
                                        uint32_t max_alive_time,
                                        uint32_t max_request);

  HttpConnectionPool(const std::string& host, const std::string& vhost,
                     uint32_t port, bool is_https, uint32_t max_size,
                     uint32_t max_alive_time, uint32_t max_request);

  HttpConnection::ptr getConnection();

  /**
   * @brief 发送HTTP的GET请求
   * @param[in] url 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms,
                        const std::map<std::string, std::string>& headers = {},
                        const std::string& body = "");

  /**
   * @brief 发送HTTP的GET请求
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms,
                        const std::map<std::string, std::string>& headers = {},
                        const std::string& body = "");

  /**
   * @brief 发送HTTP的POST请求
   * @param[in] url 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms,
                         const std::map<std::string, std::string>& headers = {},
                         const std::string& body = "");

  /**
   * @brief 发送HTTP的POST请求
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms,
                         const std::map<std::string, std::string>& headers = {},
                         const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] method 请求类型
   * @param[in] uri 请求的url
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doRequest(
      HttpMethod method, const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] method 请求类型
   * @param[in] uri URI结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @param[in] headers HTTP请求头部参数
   * @param[in] body 请求消息体
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doRequest(
      HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
   * @brief 发送HTTP请求
   * @param[in] req 请求结构体
   * @param[in] timeout_ms 超时时间(毫秒)
   * @return 返回HTTP结果结构体
   */
  HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

 private:
  static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

 private:
  // 目标主机
  std::string m_host;
  // 虚拟主机
  std::string m_vhost;
  // 端口
  uint32_t m_port;
  // 最大连接数
  uint32_t m_maxSize;
  // 最大存活时间
  uint32_t m_maxAliveTime;
  // 最大请求数
  uint32_t m_maxRequest;
  // 是否https
  bool m_isHttps;

  MutexType m_mutex;
  // 连接池
  std::list<HttpConnection*> m_conns;
  // 总连接数
  std::atomic<int32_t> m_total = {0};
};

}  // namespace http
}  // namespace mysylar

#endif
