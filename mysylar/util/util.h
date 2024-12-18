#ifndef __MYSYLAR_UTIL_H__
#define __MYSYLAR_UTIL_H__

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>
namespace mysylar
{
    /**
     * @brief 获取线程id
     */
    pid_t GetThreadId();
    /**
     * @brief 获取协程id
     */
    u_int32_t GetFiberId();
    /**
     * @brief 获取回溯
     */
    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);
    /**
     * @brief 获取回溯转string
     */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");
    /**
     * @brief 获取当前ms
     */
    uint64_t GetCurrentMS();
    /**
     * @brief 获取当前us (not use)
     */
    uint64_t GetCurrentUS();
} // namespace mysylar
#endif