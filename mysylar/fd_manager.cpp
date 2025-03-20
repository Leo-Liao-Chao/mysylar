#include "./fd_manager.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "./hook.h"

namespace mysylar {
FdCtx::FdCtx(int fd)
    : m_isInit(false),
      m_isSocket(false),
      m_sysNonblock(false),
      m_userNonblock(false),
      m_isClosed(false),
      m_fd(fd),
      m_recvTimeout(-1),
      m_sendTimeout(-1) {
  init();
}
FdCtx::~FdCtx() {}
bool FdCtx::init() {
  if (m_isInit) {
    return true;
  }
  m_recvTimeout = -1;
  m_sendTimeout = -1;

  struct stat fd_stat;
  // fstat 是一个在 Unix 和类 Unix
  // 操作系统中使用的系统调用，用于获取一个打开的文件描述符的状态信息。这个函数会填充一个stat
  // 结构体，该结构体包含了文件的各种属性，比如文件的大小、权限、所有者、最后访问和修改时间等。
  if (-1 == fstat(m_fd, &fd_stat)) {
    m_isInit = false;
    m_isSocket = false;
  } else {
    m_isInit = true;
    // S_ISSOCK 是一个宏，用于检查给定的文件描述符是否表示一个 socket。
    m_isSocket = S_ISSOCK(fd_stat.st_mode);
  }

  if (m_isSocket) {
    //  fcntl_f 函数（可能是 fcntl 的一个封装或别名）来获取文件描述符 m_fd
    //  当前的文件状态标志（file status flags）。
    // F_GETFL:GET
    int flags = fcntl_f(m_fd, F_GETFL, 0);
    if (!(flags & O_NONBLOCK)) {
      // 如果文件描述符处于阻塞模式，这行代码将调用 fcntl_f
      // 函数来设置文件描述符为非阻塞模式。 F_SETFL:SET
      fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
    }
    m_sysNonblock = true;
  } else {
    m_sysNonblock = false;
  }

  m_userNonblock = false;
  m_isClosed = false;
  return m_isInit;
}
void FdCtx::setTimeout(int type, uint64_t v) {
  // 接收
  if (type == SO_RCVTIMEO) {
    m_recvTimeout = v;
  }
  // 发送
  else {
    m_sendTimeout = v;
  }
}
uint64_t FdCtx::getTimeout(int type) {
  // 接收
  if (type == SO_RCVTIMEO) {
    return m_recvTimeout;
  }
  // 发送
  else {
    return m_sendTimeout;
  }
}

FdManager::FdManager() { m_datas.resize(64); }
FdCtx::ptr FdManager::get(int fd, bool auto_create) {
  if (fd == -1) {
    return nullptr;
  }
  RWMutexType::ReadLock lock(m_mutex);
  if ((int)m_datas.size() <= fd) {
    // 超出大小,并不自动创建
    if (auto_create == false) {
      return nullptr;
    }
  } else {
    // 大小内,存在,或者不自动创建
    if (m_datas[fd] || !auto_create) {
      return m_datas[fd];
    }
  }
  lock.unlock();

  // 自动创建(超出,不存在)
  RWMutexType::WriteLock lock2(m_mutex);
  FdCtx::ptr ctx(new FdCtx(fd));
  // 超出大小
  if (fd >= (int)m_datas.size()) {
    m_datas.resize(fd * 1.5);
  }
  m_datas[fd] = ctx;
  return ctx;
}
void FdManager::del(int fd) {
  RWMutexType::WriteLock lock(m_mutex);
  if (m_datas.size() <= 0) {
    return;
  }
  m_datas[fd].reset();
}
}  // namespace mysylar