#include "./address.h"

#include <ifaddrs.h>
#include <netdb.h>
#include <stddef.h>

#include <sstream>

#include "./endian.h"
#include "./log.h"

namespace mysylar {

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

// CreateMask 模板函数用于创建一个掩码，其中 小于 bits 位为 1，其余位为 0。00001111
template <class T>
static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;
}
// CountBytes 模板函数用于计算一个整数的二进制表示中 1 的个数。
template <class T>
static uint32_t CountBytes(T value) {
  uint32_t result = 0;
  for (; value; ++result) {
    value &= value - 1;
  }
  return result;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen) {
  if (addr == nullptr) {
    return nullptr;
  }

  Address::ptr result;
  switch (addr->sa_family) {
    case AF_INET:
      result.reset(new IPv4Address(*(const sockaddr_in*)addr));
      break;
    case AF_INET6:
      result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
      break;
    default:
      result.reset(new UnknownAddress(*addr));
      break;
  }
  return result;
}
// Lookup 方法使用 getaddrinfo 函数根据主机名和参数查找对应的地址，并填充到
// result 向量中。
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family, int type, int protocol) {
  addrinfo hints, *results, *next;
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_addrlen = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  std::string node;
  const char* service = NULL;

  // 检查 ipv6address serivce
  // Address::Lookup(addrs, "[::1]:80"); Address::Lookup(addrs, "[::1]");
  if (!host.empty() && host[0] == '[') {
    // 查找]的位置，返回位置的指针
    const char* endipv6 =
        (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
    if (endipv6) {
      // TODO check out of range
      if (*(endipv6 + 1) == ':') {
        // 端口起点。
        service = endipv6 + 2;
      }
      // node = ::1,从第一个开始，到endipv6 - host.c_str() - 1
      node = host.substr(1, endipv6 - host.c_str() - 1);
    }
  }

  // ipv4address serivce
  // Address::Lookup(addrs, "192.168.1.1:80");www.baidu.com:80
  if (node.empty()) {
    service = (const char*)memchr(host.c_str(), ':', host.size());
    if (service) {
      // 只有一个:号,冒号后无第二个:号
      if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
        node = host.substr(0, service - host.c_str());
        ++service;
      }
    }
  }
  // 域名格式 www.baidu.com
  if (node.empty()) {
    node = host;
  }
  // 解析地址到results（包含解析后的地址信息链表）
  int error = getaddrinfo(node.c_str(), service, &hints, &results);
  if (error) {
    MYSYLAR_LOG_DEBUG(g_logger)
        << "Address::Lookup getaddress(" << host << ", " << family << ", "
        << type << ") err=" << error << " errstr=" << gai_strerror(error);
    return false;
  }

  next = results;
  while (next) {
    // 创建Address对象
    result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
    next = next->ai_next;
  }
  // 释放链表
  freeaddrinfo(results);
  return !result.empty();
}
// LookupAny 和 LookupAnyIPAddress 方法用于根据主机名或 IP 地址查找对应的
// Address 或 IPAddress 对象。
Address::ptr Address::LookupAny(const std::string& host, int family, int type,
                                int protocol) {
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol)) {
    return result[0];
  }
  return nullptr;
}
IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host, int family,
                                           int type, int protocol) {
  std::vector<Address::ptr> result;
  if (Lookup(result, host, family, type, protocol)) {
    for (auto& i : result) {
      // 将 Address 对象转换为 IPAddress 对象，返回第一个成功转换的
      IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
      if (v) {
        return v;
      }
    }
  }
  return nullptr;
}

// GetInterfaceAddresses 方法用于获取本机所有网络接口的地址或指定接口的地址。
bool Address::GetInterfaceAddresses(
    std::multimap<std::string, std::pair<Address::ptr, uint32_t> >& result,
    int family) {
  struct ifaddrs *next, *results;
  // 获取本机所有网络接口的地址
  if (getifaddrs(&results) != 0) {
    MYSYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddresses getifaddrs "
                                   " err="
                                << errno << " errstr=" << strerror(errno);
    return false;
  }

  try {
    for (next = results; next; next = next->ifa_next) {
      Address::ptr addr;
      uint32_t prefix_len = ~0u;
      // AF_UNSPEC，未指定协议族，指定协议IPV4 IPV6
      if (family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
        continue;
      }
      switch (next->ifa_addr->sa_family) {
        // ipv4
        case AF_INET: {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in));
          // 8x4
          uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
          prefix_len = CountBytes(netmask);
        } break;
        // ipv6
        case AF_INET6: {
          addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
          in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
          prefix_len = 0;
          // 8x16
          for (int i = 0; i < 16; ++i) {
            prefix_len += CountBytes(netmask.s6_addr[i]);
          }
        } break;
        default:
          break;
      }

      if (addr) {
        result.insert(
            std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
      }
    }
  } catch (...) {
    MYSYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses exception";
    freeifaddrs(results);
    return false;
  }
  freeifaddrs(results);
  return !result.empty();
}
bool Address::GetInterfaceAddresses(
    std::vector<std::pair<Address::ptr, uint32_t> >& result,
    const std::string& iface, int family) {
      // 未指定网卡名字，名字为*，返回假地址。
  if (iface.empty() || iface == "*") {
    if (family == AF_INET || family == AF_UNSPEC) {
      result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
    }
    if (family == AF_INET6 || family == AF_UNSPEC) {
      result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
    }
    return true;
  }

  std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

  if (!GetInterfaceAddresses(results, family)) {
    return false;
  }
  // equal_range 返回一组迭代器(与iface名字一样的所有地址)
  auto its = results.equal_range(iface);
  for (; its.first != its.second; ++its.first) {
    result.push_back(its.first->second);
  }
  return !result.empty();
}

int Address::getFamily() const { return getAddr()->sa_family; }
std::string Address::toString() const {
  std::stringstream ss;
  insert(ss);
  return ss.str();
}

bool Address::operator<(const Address& rhs) const {
  socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
  int result = memcmp(getAddr(), rhs.getAddr(), minlen);
  if (result < 0) {
    return true;
  } else if (result > 0) {
    return false;
  } else if (getAddrLen() < rhs.getAddrLen()) {
    return true;
  }
  return false;
}
bool Address::operator==(const Address& rhs) const {
  return getAddrLen() == rhs.getAddrLen() &&
         memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}
bool Address::operator!=(const Address& rhs) const { return !(*this == rhs); }

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
  addrinfo hints, *results;
  memset(&hints, 0, sizeof(addrinfo));

  // changed By Leo 0 (1)
  // AI_NUMERICHOST 用于指示 getaddrinfo 期望一个数值 IP 地址，并且避免 DNS
  // 查找，而 AI_CANONNAME 用于获取主机的规范化名称。 hints.ai_flags =
  // AI_CANONNAME;
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_UNSPEC;

  int error = getaddrinfo(address, NULL, &hints, &results);
  if (error) {
    MYSYLAR_LOG_DEBUG(g_logger)
        << "IPAddress::Create(" << address << ", " << port
        << ") error=" << error << " errno=" << errno
        << " errstr=" << strerror(errno);
    return nullptr;
  }

  try {
    IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
        Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
    if (result) {
      result->setPort(port);
    }
    freeaddrinfo(results);
    return result;
  } catch (...) {
    freeaddrinfo(results);
    return nullptr;
  }
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
  IPv4Address::ptr rt(new IPv4Address);
  // 主机字节序 -> 网络字节序
  // 例如在小端系统上:
  // 端口 80 (0x0050)
  // 主机字节序: 0x0050
  // 网络字节序: 0x5000
  rt->m_addr.sin_port = byteswapOnLittleEndian(port);
  // 将点分十进制IP地址转换为网络字节序的二进制形式
  int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
  if (result <= 0) {
    MYSYLAR_LOG_DEBUG(g_logger)
        << "IPv4Address::Create(" << address << ", " << port
        << ") rt=" << result << " errno=" << errno
        << " errstr=" << strerror(errno);
    return nullptr;
  }
  return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address) { m_addr = address; }

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

sockaddr* IPv4Address::getAddr() { return (sockaddr*)&m_addr; }

const sockaddr* IPv4Address::getAddr() const { return (sockaddr*)&m_addr; }

socklen_t IPv4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream& IPv4Address::insert(std::ostream& os) const {
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "."
     << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
  os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr |=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr &=
      byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr =
      ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t v) {
  m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
  IPv6Address::ptr rt(new IPv6Address);
  rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
  int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
  if (result <= 0) {
    MYSYLAR_LOG_DEBUG(g_logger)
        << "IPv6Address::Create(" << address << ", " << port
        << ") rt=" << result << " errno=" << errno
        << " errstr=" << strerror(errno);
    return nullptr;
  }
  return rt;
}

IPv6Address::IPv6Address() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address) { m_addr = address; }

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian(port);
  memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

sockaddr* IPv6Address::getAddr() { return (sockaddr*)&m_addr; }

const sockaddr* IPv6Address::getAddr() const { return (sockaddr*)&m_addr; }

socklen_t IPv6Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream& IPv6Address::insert(std::ostream& os) const {
  os << "[";
  uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
  bool used_zeros = false;
  for (size_t i = 0; i < 8; ++i) {
    if (addr[i] == 0 && !used_zeros) {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zeros) {
      os << ":";
      used_zeros = true;
    }
    if (i) {
      os << ":";
    }
    os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
  }
  // 5. 处理特殊情况：末尾为0
  if (!used_zeros && addr[7] == 0) {
    os << "::";
  }
  // 6. 添加端口号
  os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
  return os;
  // [2001:db8:85a3::8a2e:370:7334]:80
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  // 2. 处理前缀边界字节
    // prefix_len = 42时:
    // 42 / 8 = 5 (定位到第6个字节)
    // 42 % 8 = 2 (在第6个字节中的位置)
  baddr.sin6_addr.s6_addr[prefix_len / 8] |=
      CreateMask<uint8_t>(prefix_len % 8);
  // 3. 将剩余字节全部置为1
    // 从前缀后的第一个完整字节开始，到最后一个字节
    // 设置为0xff (二进制:11111111)
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }
  return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  // 2. 处理前缀边界字节
  // 例如：prefix_len = 42
  // 42 / 8 = 5 (第6个字节)
  // 42 % 8 = 2 (第6个字节中的位置)
  // CreateMask创建掩码，与操作将超出前缀长度的位置0
  baddr.sin6_addr.s6_addr[prefix_len / 8] &=
      CreateMask<uint8_t>(prefix_len % 8);
  // 3. 将剩余字节全部置为0
  // 从前缀后的第一个完整字节开始，到最后一个字节
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0x00;
  }
  return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in6 subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;
  // 2. 设置边界字节
  // 例如: prefix_len = 42
  // 42 / 8 = 5 (第6个字节)
  // 42 % 8 = 2 (第6个字节中的位置)
  // ~CreateMask 创建反掩码，将前缀位置1，其余位置0
  subnet.sin6_addr.s6_addr[prefix_len / 8] =
      ~CreateMask<uint8_t>(prefix_len % 8);

  for (uint32_t i = 0; i < prefix_len / 8; ++i) {
    subnet.sin6_addr.s6_addr[i] = 0xff;
  }
  return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const {
  return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t v) {
  m_addr.sin6_port = byteswapOnLittleEndian(v);
}
// 常量用于限制Unix域套接字路径的最大长度 107
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
  // 1. 清零整个结构体
  // 防止未初始化内存可能带来的安全问题
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  // 3. 计算地址结构总长度
  // offsetof(sockaddr_un, sun_path): sun_path成员偏移量
  // MAX_PATH_LEN: 路径最大长度
  // 用于bind/connect等系统调用
  m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = path.size() + 1;

  if (!path.empty() && path[0] == '\0') {
    --m_length;
  }

  if (m_length > sizeof(m_addr.sun_path)) {
    throw std::logic_error("path too long");
  }
  memcpy(m_addr.sun_path, path.c_str(), m_length);
  m_length += offsetof(sockaddr_un, sun_path);
}

void UnixAddress::setAddrLen(uint32_t v) { m_length = v; }

sockaddr* UnixAddress::getAddr() { return (sockaddr*)&m_addr; }

const sockaddr* UnixAddress::getAddr() const { return (sockaddr*)&m_addr; }

socklen_t UnixAddress::getAddrLen() const { return m_length; }

std::string UnixAddress::getPath() const {
  std::stringstream ss;
  if (m_length > offsetof(sockaddr_un, sun_path) &&
      m_addr.sun_path[0] == '\0') {
    ss << "\\0"
       << std::string(m_addr.sun_path + 1,
                      m_length - offsetof(sockaddr_un, sun_path) - 1);
  } else {
    ss << m_addr.sun_path;
  }
  return ss.str();
}

std::ostream& UnixAddress::insert(std::ostream& os) const {
  if (m_length > offsetof(sockaddr_un, sun_path) &&
      m_addr.sun_path[0] == '\0') {
    return os << "\\0"
              << std::string(m_addr.sun_path + 1,
                             m_length - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress(int family) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) { m_addr = addr; }

sockaddr* UnknownAddress::getAddr() { return (sockaddr*)&m_addr; }

const sockaddr* UnknownAddress::getAddr() const { return &m_addr; }

socklen_t UnknownAddress::getAddrLen() const { return sizeof(m_addr); }

std::ostream& UnknownAddress::insert(std::ostream& os) const {
  os << "[UnknownAddress family=" << m_addr.sa_family << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
  return addr.insert(os);
}

}  // namespace mysylar
