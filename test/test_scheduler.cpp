#include "../mysylar/mysylar.h"

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void test_fiber()
{
    static int s_count = 5;
    MYSYLAR_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if (--s_count >= 0)
    {
        mysylar::Scheduler::GetThis()->schedule(&test_fiber, mysylar::GetThreadId());
    }
}

int main(int argc, char **argv)
{
    MYSYLAR_LOG_INFO(g_logger) << "main";
    mysylar::Scheduler sc(3, true, "test");
    sc.start();
    // Run
    sleep(2);
    MYSYLAR_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    MYSYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}
