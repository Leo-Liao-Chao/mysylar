#include "../mysylar/mysylar.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void run_in_fiber()
{
    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    // mysylar::Fiber::GetThis()->swapOut();
    mysylar::Fiber::YieldToHold();
    MYSYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    mysylar::Fiber::YieldToHold();
}

void test_fiber()
{
    MYSYLAR_LOG_INFO(g_logger) << "main after end -1";
    {
        mysylar::Fiber::GetThis();
        MYSYLAR_LOG_INFO(g_logger) << "main begin";
        mysylar::Fiber::ptr fiber(new mysylar::Fiber(run_in_fiber));
        fiber->swapIn();
        MYSYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        MYSYLAR_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    MYSYLAR_LOG_INFO(g_logger) << "main after end2";
}

int main(int agrc, char **argv)
{
    mysylar::Thread::SetName("main");
    std::vector<mysylar::Thread::ptr> thrs;
    for(int i=0;i<3;i++)
    {
        thrs.push_back(mysylar::Thread::ptr(new mysylar::Thread(&test_fiber,"name_"+std::to_string(i))));
    }
    for(auto i:thrs)
    {
        i->join();
    }

    return 0;
}