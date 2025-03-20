

#include "./hook.h"

#include <dlfcn.h>

#include "./config.h"
#include "./fd_manager.h"
#include "./fiber.h"
#include "./iomanager.h"
#include "./log.h"
#include "./macro.h"

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

namespace mysylar {
static mysylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    mysylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
static thread_local bool t_hook_enable = false;

// 定义 XX 宏：定义一个宏，用于生成初始化钩子函数指针的语句。
#define HOOK_FUN(XX) \
  XX(sleep)          \
  XX(usleep)         \
  XX(nanosleep)      \
  XX(socket)         \
  XX(connect)        \
  XX(accept)         \
  XX(read)           \
  XX(readv)          \
  XX(recv)           \
  XX(recvfrom)       \
  XX(recvmsg)        \
  XX(write)          \
  XX(writev)         \
  XX(send)           \
  XX(sendto)         \
  XX(sendmsg)        \
  XX(close)          \
  XX(fcntl)          \
  XX(ioctl)          \
  XX(getsockopt)     \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }

// 执行初始化语句：这些初始化语句使用 dlsym
// 函数加载系统调用的地址，并赋值给对应的函数指针变量。覆盖nullptr
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener(
        [](const int &old_value, const int &new_value) {
          MYSYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                     << old_value << " to " << new_value;
          s_connect_timeout = new_value;
        });
  }
};
static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }
void set_hook_enable(bool flag) { t_hook_enable = flag; }
}  // namespace mysylar
struct timer_info {
  int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
  if (!mysylar::t_hook_enable) {
    // 完美转发允许函数模板将其参数完美地转发给另一个函数，而不改变参数的类型和值类别（左值或右值）
    return fun(fd, std::forward<Args>(args)...);
  }

  mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    return fun(fd, std::forward<Args>(args)...);
  }

  if (ctx->isClose()) {
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    return fun(fd, std::forward<Args>(args)...);
  }

  uint64_t to = ctx->getTimeout(timeout_so);
  std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  if (n == -1 && errno == EAGAIN) {
    mysylar::IOManager *iom = mysylar::IOManager::GetThis();
    mysylar::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);

    if (to != (uint64_t)-1) {
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (mysylar::IOManager::Event)(event));
          },
          winfo);
    }

    int rt = iom->addEvent(fd, (mysylar::IOManager::Event)(event));
    if (MYSYLAR_UNLIKELY(rt)) {
      MYSYLAR_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      mysylar::Fiber::YieldToHold();
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      goto retry;
    }
  }

  return n;
}

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX)
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!mysylar::t_hook_enable) {
    return sleep_f(seconds);
  }
  mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
  mysylar::IOManager *iom = mysylar::IOManager::GetThis();
  // iom->addTimer(seconds * 1000,
  // std::bind(&mysylar::IOManager::schedule<Fiber::ptr>, iom, fiber));
  // iom->addTimer(seconds * 1000, [iom, fiber]()
  //               { iom->schedule(fiber); });
  // 这行代码创建了一个定时器，当 sleep
  // 函数被调用时，定时器将在指定的秒数后触发。
  // 定时器触发时，将调用 IOManager 的 schedule 函数，将当前协程 fiber
  // 重新调度执行。 sleep
  // 函数不会阻塞整个线程，而是只让当前协程挂起，允许其他协程在同一线程上继续执行。

  // (void(mysylar::Scheduler::*)(mysylar::Fiber::ptr, int thread)) &mysylar::IOManager::schedule 成员函数指针
  // 调用与对象iom
  // 参数fiber, -1
  iom->addTimer(
      seconds * 1000,
      std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr, int thread)) &
                    mysylar::IOManager::schedule,
                iom, fiber, -1));

  mysylar::Fiber::YieldToHold();
  return 0;
}
int usleep(useconds_t usec) {
  if (!mysylar::t_hook_enable) {
    return usleep_f(usec);
  }
  mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
  mysylar::IOManager *iom = mysylar::IOManager::GetThis();
  // iom->addTimer(seconds / 1000,
  // std::bind(&mysylar::IOManager::schedule<Fiber::ptr>, iom, fiber));
  // iom->addTimer(usec / 1000, [iom, fiber]()
  //               { iom->schedule(fiber); });
  iom->addTimer(
      usec / 1000,
      std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr, int thread)) &
                    mysylar::IOManager::schedule,
                iom, fiber, -1));
  mysylar::Fiber::YieldToHold();
  return 0;
}
int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!mysylar::t_hook_enable) {
    return nanosleep_f(req, rem);
  }

  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  mysylar::Fiber::ptr fiber = mysylar::Fiber::GetThis();
  mysylar::IOManager *iom = mysylar::IOManager::GetThis();
  iom->addTimer(
      timeout_ms,
      std::bind((void(mysylar::Scheduler::*)(mysylar::Fiber::ptr, int thread)) &
                    mysylar::IOManager::schedule,
                iom, fiber, -1));
  mysylar::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (!mysylar::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  mysylar::FdMgr::GetInstance()->get(fd, true);
  return fd;
}
//     如果钩子未启用，直接调用原始的 connect 系统调用。
// 如果钩子启用，执行以下步骤：
// 获取与文件描述符 fd 相关的 FdCtx 对象。
// 如果 FdCtx 对象不存在或已关闭，设置 errno 为 EBADF 并返回 -1。
// 如果 FdCtx 对象表示的不是 socket 或已设置为非阻塞模式，直接调用原始的 connect
// 系统调用。 尝试连接，如果立即成功或失败，直接返回结果。
// 如果连接正在进行（EINPROGRESS），设置一个定时器和事件，等待连接完成或超时。
// 如果连接成功，返回 0；如果超时或出错，返回 -1 并设置 errno。
int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  if (!mysylar::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClose()) {
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket()) {
    return connect_f(fd, addr, addrlen);
  }

  if (ctx->getUserNonblock()) {
    return connect_f(fd, addr, addrlen);
  }

  int n = connect_f(fd, addr, addrlen);
  // 成功
  if (n == 0) {
    return 0;
  }
  // 失败
  else if (n != -1 || errno != EINPROGRESS) {
    return n;
  }
  // 正在进行中
  mysylar::IOManager *iom = mysylar::IOManager::GetThis();
  mysylar::Timer::ptr timer;
  std::shared_ptr<timer_info> tinfo(new timer_info);
  // 条件
  std::weak_ptr<timer_info> winfo(tinfo);
  // [winfo, fd, iom]() {} 三个变量外部变量可以访问的。
  if (timeout_ms != (uint64_t)-1) {
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, fd, iom]() {
          // 对象被销毁返回false
          auto t = winfo.lock();
          if (!t || t->cancelled) {
            return;
          }
          t->cancelled = ETIMEDOUT;
          iom->cancelEvent(fd, mysylar::IOManager::WRITE);
        },
        winfo);
  }

  int rt = iom->addEvent(fd, mysylar::IOManager::WRITE);
  if (rt == 0) {
    mysylar::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer) {
      timer->cancel();
    }
    MYSYLAR_LOG_ERROR(g_logger)
        << "connect addEvent(" << fd << ", WRITE) error";
  }

  int error = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  if (!error) {
    return 0;
  } else {
    errno = error;
    return -1;
  }
}
// connect 函数是一个钩子，用于拦截系统 connect 调用，并使用
// connect_with_timeout 函数执行带超时的连接。
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen,
                              mysylar::s_connect_timeout);
}
// accept 函数是一个钩子，它包装了原始的 accept
// 系统调用，以便在接收新的连接时执行额外的管理任务，如创建和管理文件描述符上下文。这使得程序能够跟踪每个连接的状态，并在需要时应用非阻塞和超时逻辑。
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = do_io(s, accept_f, "accept", mysylar::IOManager::READ, SO_RCVTIMEO,
                 addr, addrlen);
  if (fd >= 0) {
    mysylar::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}
ssize_t read(int fd, void *buf, size_t count) {
  return do_io(fd, read_f, "read", mysylar::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", mysylar::IOManager::READ, SO_RCVTIMEO, iov,
               iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return do_io(sockfd, recv_f, "recv", mysylar::IOManager::READ, SO_RCVTIMEO,
               buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return do_io(sockfd, recvfrom_f, "recvfrom", mysylar::IOManager::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", mysylar::IOManager::READ,
               SO_RCVTIMEO, msg, flags);
}
ssize_t write(int fd, const void *buf, size_t count) {
  return do_io(fd, write_f, "write", mysylar::IOManager::WRITE, SO_SNDTIMEO,
               buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", mysylar::IOManager::WRITE, SO_SNDTIMEO,
               iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
  return do_io(s, send_f, "send", mysylar::IOManager::WRITE, SO_SNDTIMEO, msg,
               len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen) {
  return do_io(s, sendto_f, "sendto", mysylar::IOManager::WRITE, SO_SNDTIMEO,
               msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
  return do_io(s, sendmsg_f, "sendmsg", mysylar::IOManager::WRITE, SO_SNDTIMEO,
               msg, flags);
}

int close(int fd) {
  if (!mysylar::t_hook_enable) {
    return close_f(fd);
  }
  mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
  if (ctx) {
    auto iom = mysylar::IOManager::GetThis();
    if (iom) {
      iom->cancelAll(fd);
    }
    mysylar::FdMgr::GetInstance()->del(fd);
  }
  return close_f(fd);
}
int fcntl(int fd, int cmd, ... /* arg */) {
  va_list va;
  va_start(va, cmd);
  switch (cmd) {
    case F_SETFL: {
      int arg = va_arg(va, int);
      va_end(va);
      mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
      if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return fcntl_f(fd, cmd, arg);
      }
      ctx->setUserNonblock(arg & O_NONBLOCK);
      if (ctx->getSysNonblock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFL: {
      va_end(va);
      int arg = fcntl_f(fd, cmd);
      mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(fd);
      if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return arg;
      }
      if (ctx->getUserNonblock()) {
        return arg | O_NONBLOCK;
      } else {
        return arg & ~O_NONBLOCK;
      }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
      int arg = va_arg(va, int);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
      va_end(va);
      return fcntl_f(fd, cmd);
    } break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK: {
      struct flock *arg = va_arg(va, struct flock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    } break;
    case F_GETOWN_EX:
    case F_SETOWN_EX: {
      struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
      va_end(va);
      return fcntl_f(fd, cmd, arg);
    } break;
    default:
      va_end(va);
      return fcntl_f(fd, cmd);
  }
}

int ioctl(int d, unsigned long int request, ...) {
  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);

  if (FIONBIO == request) {
    bool user_nonblock = !!*(int *)arg;
    mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(d);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
      return ioctl_f(d, request, arg);
    }
    ctx->setUserNonblock(user_nonblock);
  }
  return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (!mysylar::t_hook_enable) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      mysylar::FdCtx::ptr ctx = mysylar::FdMgr::GetInstance()->get(sockfd);
      if (ctx) {
        const timeval *v = (const timeval *)optval;
        ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
