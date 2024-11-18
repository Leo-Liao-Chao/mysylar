#ifndef __MYSYLAR_UTIL_H__
#define __MYSYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
namespace mysylar
{

pid_t GetThreadId();
u_int32_t GetFiberId();

}
#endif