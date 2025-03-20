#include "./thread.h"

#include "./log.h"
#include "./util/util.h"

namespace mysylar {

static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");
/**
 * @brief 获取当前线程指针
 */
Thread *Thread::GetThis() { return t_thread; }
/**
 * @brief 获取当前线程名称
 */
const std::string &Thread::GetName() { return t_thread_name; }
/**
 * @brief 设置当前线程名称
 * @param[in] name 线程名称
 */
void Thread::SetName(const std::string &name) {
  if (name.empty()) {
    return;
  }
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name) {
  if (name.empty()) {
    m_name = "UNKNOW";
  }
  // 创建线程
  int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (rt) {
    MYSYLAR_LOG_ERROR(g_logger)
        << "pthread_create thread fail, rt=" << rt << " name=" << name;
    throw std::logic_error("pthread_create error");
  }
  // 等待线程真正开始
  m_semaphore.wait();
}

Thread::~Thread() {
  if (m_thread) {
    pthread_detach(m_thread);
  }
}

void Thread::join() {
  if (m_thread) {
    int rt = pthread_join(m_thread, nullptr);
    if (rt) {
      MYSYLAR_LOG_ERROR(g_logger)
          << "pthread_join thread fail, rt=" << rt << " name=" << m_name;
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void *Thread::run(void *arg) {
  Thread *thread = (Thread *)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = mysylar::GetThreadId();
  // 设置线程名称
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->m_cb);
  // 释放信号量
  thread->m_semaphore.notify();

  cb();
  return 0;
}

}  // namespace mysylar
