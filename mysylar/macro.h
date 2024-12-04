#ifndef __MYSYLAR_MACRO_H__
#define __MYSYLAR_MACRO_H__

#include <string.h>
#include <assert.h>
#include "./util/util.h"
#include "./log.h"
#define MYSYLAR_ASSERT(x)                                                                    \
    if (!(x))                                                                                \
    {                                                                                        \
        MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT()) << " ASSERTION: " #x                           \
                                              << "\nbacktrace:\n"                            \
                                              << mysylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                           \
    }
#define MYSYLAR_ASSERT2(x, w)                                                                 \
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