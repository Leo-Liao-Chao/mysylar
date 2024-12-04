#include "../mysylar/mysylar.h"
#include <assert.h>

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

void test_assert()
{
    MYSYLAR_LOG_INFO(g_logger) << mysylar::BacktraceToString(10);
    MYSYLAR_ASSERT2(0==1,"abcdef xx");
    // assert(0==1);
}

int main(int argc, char **argv)
{
    test_assert();
    return 0;
}