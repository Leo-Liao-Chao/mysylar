#ifndef __MYSYLAR_UTIL_H__
#define __MYSYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
namespace mysylar
{

    pid_t GetThreadId();
    u_int32_t GetFiberId();
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");
}
#endif