/**
 * @file iomanager.h
 * @brief 基于Epoll的IO协程调度器
 * 继承与协程调度器，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）,支持Socket读写时间的添加，删除，取消功能。支持一次性定时器，循环定时器，条件定时器等功能
 */
#ifndef __MYSYLAR_IOMANAGER_H__
#define __MYSYLAR_IOMANAGER_H__

#include "./scheduler.h"
#include "./timer.h"

namespace mysylar {

/**
 * @brief 基于Epoll的IO协程调度器
 */
class IOManager : public Scheduler, public TimerManager {
 public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  /**
   * @brief IO事件(NONE,READ,WRITE)
   */
  enum Event {
    /// 无事件
    NONE = 0x0,
    /// 读事件(EPOLLIN)
    READ = 0x1,
    /// 写事件(EPOLLOUT)
    WRITE = 0x4,
  };

 private:
  /**
   * @brief Socket事件上线文类
   */
  struct FdContext {
    typedef Mutex MutexType;
    /**
     * @brief 事件上线文类
     */
    struct EventContext {
      /// 事件执行的调度器
      Scheduler* scheduler = nullptr;
      /// 事件协程
      Fiber::ptr fiber;
      /// 事件的回调函数
      std::function<void()> cb;
    };

    /**
     * @brief 获取事件上下文类
     * @param[in] event 事件类型
     * @return 返回对应事件的上线文
     */
    EventContext& getContext(Event event);

    /**
     * @brief 重置事件上下文
     * @param[in, out] ctx 待重置的上下文类
     */
    void resetContext(EventContext& ctx);

    /**
     * @brief 触发事件
     * @param[in] event 事件类型
     */
    void triggerEvent(Event event);

    /// 读事件上下文
    EventContext read;
    /// 写事件上下文
    EventContext write;
    /// 事件关联的句柄
    int fd = 0;
    /// 当前的事件(NONE,READ,WRITE)
    Event events = NONE;
    /// 事件的Mutex
    MutexType mutex;
  };

 public:
  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否将调用线程包含进去
   * @param[in] name 调度器的名称
   */
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string& name = "");

  /**
   * @brief 析构函数
   */
  ~IOManager();

  /**
   * @brief 添加事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @param[in] cb 事件回调函数
   * @return 添加成功返回0,失败返回-1
   */
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

  /**
   * @brief 删除事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @attention 不会触发事件
   */
  bool delEvent(int fd, Event event);

  /**
   * @brief 取消事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @attention 如果事件存在则触发事件
   */
  bool cancelEvent(int fd, Event event);

  /**
   * @brief 取消所有事件
   * @param[in] fd socket句柄
   */
  bool cancelAll(int fd);

  /**
   * @brief 返回当前的IOManager
   */
  static IOManager* GetThis();

 protected:
  /**
   * @brief 没有空闲线程return，有空闲线程，写端发一个信号。
   */
  void tickle() override;
  /**
   * @brief 正在停止
   */
  bool stopping() override;
  /**
   * @brief 设定了一个events来监听事件序列
   * 进入while循环，
   * 判断是否正在停止，停止就结束。
   * 进入第二个监听，每次监听一定时间，监听到内容就跳出循环，没有就继续循环。监听到的内容给events。
   * 处理到期的事件，加入scheduler，清空。
   * 遍历监听事件
   *  获取上下文，如果是错误或者挂起，且事件存在，那么就标志为读写事件。
   * 获取真实事件real_events.
   * 判断剩余事件left_events.注册fd
   * 根据真实事件，触发事件。
   * 触发完成切除线程。
   */
  void idle() override;
  /**
   * @brief
   */
  void onTimerInsertedAtFront() override;

  /**
   * @brief 重置socket句柄上下文的容器大小
   * @param[in] size 容量大小
   */
  void contextResize(size_t size);

  /**
   * @brief 判断是否可以停止
   * @param[out] timeout 最近要出发的定时器事件间隔
   * @return 返回是否可以停止
   */
  bool stopping(uint64_t& timeout);

 private:
  /// epoll 文件句柄
  int m_epfd = 0;
  /// pipe 文件句柄
  int m_tickleFds[2];
  /// 当前等待执行的事件数量
  std::atomic<size_t> m_pendingEventCount = {0};
  /// IOManager的Mutex
  RWMutexType m_mutex;
  /// socket事件上下文的容器
  std::vector<FdContext*> m_fdContexts;
};

}  // namespace mysylar

#endif
