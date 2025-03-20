#include "./http_session.h"

#include "./http_parser.h"

namespace mysylar {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
  HttpRequestParser::ptr parser(new HttpRequestParser);
  uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
  // uint64_t buff_size = 100;
  std::shared_ptr<char> buffer(new char[buff_size],
                               [](char* ptr) { delete[] ptr; });
  char* data = buffer.get();
  // 读取数据
  int offset = 0;
  do {
    // 从套接字读取数据
    int len = read(data + offset, buff_size - offset);
    if (len <= 0) {
      close();
      return nullptr;
    }
    len += offset;
    // 解析数据nparse，有效数据位，移除了已解析的数据
    size_t nparse = parser->execute(data, len);
    if (parser->hasError()) {
      close();
      return nullptr;
    }
    // 未完全解析，消息体偏移
    offset = len - nparse;
    if (offset == (int)buff_size) {
      close();
      return nullptr;
    }
    // 解析完成退出
    if (parser->isFinished()) {
      break;
    }
  } while (true);
  // 获取消息体长度
  int64_t length = parser->getContentLength();
  if (length > 0) {
    std::string body;
    body.resize(length);

    int len = 0;
    if (length >= offset) {
      // 消息体
      memcpy(&body[0], data, offset);
      len = offset;
    } else {
      // 消息体
      memcpy(&body[0], data, length);
      len = length;
    }
    length -= offset;
    // 继续读取消息体，读不到东西，直接关闭（异常）
    if (length > 0) {
      if (readFixSize(&body[len], length) <= 0) {
        close();
        return nullptr;
      }
    }
    // 设置消息体
    parser->getData()->setBody(body);
  }
  // 继续解析消息体
  parser->getData()->init();
  return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}

}  // namespace http
}  // namespace mysylar
