#include "../mysylar/mysylar.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void run_in_fiber() {
  MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
  mysylar::Fiber::YieldToHold();
  MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
  mysylar::Fiber::YieldToHold();
}

void test_fiber() {
  MYSYLAR_LOG_INFO(g_logger) << "main begin -1";
  {
    // std::cout<<" Get
    // start"<<std::endl;
    mysylar::Fiber::GetThis();
    // std::cout<<" Get
    // end"<<std::endl;
    MYSYLAR_LOG_INFO(g_logger) << "main begin";

    mysylar::Fiber::ptr fiber(new mysylar::Fiber(run_in_fiber));
    std::cout << "After Fiber start" << std::endl;
    // bug is here
    fiber->call();
    std::cout << "swapIn end" << std::endl;
    MYSYLAR_LOG_INFO(g_logger) << "main after "
                                  "swapIn";
    fiber->call();
    std::cout << "swapIn end" << std::endl;
    MYSYLAR_LOG_INFO(g_logger) << "main after end";
    fiber->call();
  }
  MYSYLAR_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char **argv) {
  mysylar::Thread::SetName("main");

  std::vector<mysylar::Thread::ptr> thrs;
  for (int i = 0; i < 1; ++i) {
    thrs.push_back(
        mysylar::Thread::ptr(new mysylar::Thread(&test_fiber, "name_" + std::to_string(i))));
  }
  for (auto i : thrs) {
    i->join();
  }
  return 0;
}
