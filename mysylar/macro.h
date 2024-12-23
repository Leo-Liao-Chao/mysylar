#ifndef __MYSYLAR_MACRO_H__
#define __MYSYLAR_MACRO_H__

#include <string.h>
#include <assert.h>
#include "./util/util.h"
#include "./log.h"

/**
 * @brief
 * 宏+Logger封装
 */

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define MYSYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define MYSYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MYSYLAR_LIKELY(x) (x)
#define MYSYLAR_UNLIKELY(x) (x)
#endif

#define MYSYLAR_ASSERT(x)                                                                    \
    if (!(x))                                                                                \
    {                                                                                        \
        MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT()) << " ASSERTION: " #x                           \
                                              << "\nbacktrace:\n"                            \
                                              << mysylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                           \
    }
#define MYSYLAR_ASSERT2(x, w)                                                                \
    if (!(x))                                                                                \
    {                                                                                        \
        MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT()) << " ASSERTION: " #x                           \
                                              << "\n"                                        \
                                              << w                                           \
                                              << "\nbacktrace:\n"                            \
                                              << mysylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                           \
    }

#endif