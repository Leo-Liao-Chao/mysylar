/**
 * @file fiber.h
 * @brief 协程封装
 * @details
 * 1. 首先通过GetThis里用无参构造函数创建一个Fiber::ptr  t_threadFiber用于存储线程状态（无任务执行）EXEC。
 * 2. 通过有参构造创建Fiber::ptr task_fiber（有任务线程）INIT。
 * 3.  利用task_fiber->call()，t_fiber置为task_fiber执行MainFunc/CallerMainFunc也就是任务。EXEC
    1. 任务完成后，状态为TERM
    2. 自动销毁task_fiber指向的Fiber实体。
    3. 切回t_threadFiber,t_fiber置为t_threadFiber.get()
 * 4. YiledToReady,YieldToHold将t_fiber，切出去切到t_threadFiber.get(),READY,HOLD
 * 5. 协程：
    1. 静态全局原子变量s_fiber_id,s_fiber_count用于统计信息，保证了线程安全。
    2. 定义了线程静态全局变量t_fiber当前的协程，t_threadFiber当前线程的协程。
 * ---------------------
 */
#ifndef __MYSYLAR_FIBER_H__
#define __MYSYLAR_FIBER_H__

#include <ucontext.h>

#include <functional>
#include <memory>

namespace mysylar {

class Scheduler;

/**
 * @brief 协程类
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

 public:
  typedef std::shared_ptr<Fiber> ptr;

  /**
   * @brief 协程状态
   */
  enum State {
    /// 初始化状态
    INIT,
    /// 暂停状态
    HOLD,
    /// 执行中状态
    EXEC,
    /// 结束状态
    TERM,
    /// 可执行状态
    READY,
    /// 异常状态
    EXCEPT
  };

 private:
  /**
   * @brief 无参构造函数，s_fiber_count++
   * @attention 每个线程第一个协程的构造
   */
  Fiber();

 public:
  /**
   * @brief 构造函数，s_fiber_count++
   * @param[in] cb 协程执行的函数
   * @param[in] stacksize 协程栈大小
   * @param[in] use_caller 是否在MainFiber（t_threadFiber）上调度
   * 默认m_state=INIT,m_stacksize=0,m_stack=nullptr
   */
  Fiber(std::function<void()> cb, size_t stacksize = 0,
        bool use_caller = false);

  /**
   * @brief 析构函数，非EXEC析构，主协程析构
   */
  ~Fiber();

  /**
   * @brief 返回协程id
   */
  uint64_t getId() const { return m_id; }

  /**
   * @brief 返回协程状态
   */
  State getState() const { return m_state; }

  /**
   * @brief 返回当前协程的总数量
   */
  static uint64_t TotalFibers();

  /**
   * @brief 获取当前协程的id
   */
  static uint64_t GetFiberId();

  /**
   * @brief 设置当前线程的运行协程t_fiber
   * @param[in] f 运行协程
   */
  static void SetThis(Fiber *f);

  /**
   * @brief 返回当前所在的协程t_fiber
   */
  static Fiber::ptr GetThis();

 public:
  /**
   * @brief 重置协程执行函数,并设置状态
   * @pre getState() 为 INIT, TERM, EXCEPT
   * @post getState() = INIT
   */
  void reset(std::function<void()> cb);

  /**
   * @brief 将当前协程切换到运行状态
   * @pre getState() != EXEC
   * @post getState() = EXEC
   */
  void swapIn();

  /**
   * @brief 将当前协程切换到后台
   */
  void swapOut();

  /**
   * @brief 将当前线程切换到执行状态 t_fiber = this
   * @pre 执行的为当前线程的主协程
   */
  void call();

  /**
   * @brief 将当前线程切换到后台，t_fiber = t_threadFiber.get()
   * @pre 执行的为该协程
   * @post 返回到线程的主协程
   */
  void back();

  /**
   * @brief 将当前协程切换到后台,并设置为READY状态->swapOut()
   * @post getState() = READY
   */
  static void YieldToReady();

  /**
   * @brief 将当前协程切换到后台,并设置为HOLD状态->swapOut() EXEC=HOLD
   * @post getState() = HOLD
   */
  static void YieldToHold();

  /**
   * @brief 协程执行函数->swapOut()->Scheduler::GetMainFiber()
   * @post 执行完成返回到线程主协程
   */
  static void MainFunc();

  /**
   * @brief 协程执行函数->back()->t_threadFiber.fiber线程主fiber
   * @post 执行完成返回到线程调度协程
   */
  static void CallerMainFunc();

 private:
  /// 协程id
  uint64_t m_id = 0;
  /// 协程运行栈大小
  uint32_t m_stacksize = 0;
  /// 协程状态
  State m_state = INIT;
  /// 协程上下文
  ucontext_t m_ctx;
  /// 协程运行栈指针
  void *m_stack = nullptr;
  /// 协程运行函数
  std::function<void()> m_cb;
};

}  // namespace mysylar

#endif
