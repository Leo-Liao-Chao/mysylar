#ifndef __MYSYLAR_UTIL_H__
#define __MYSYLAR_UTIL_H__

#include <cxxabi.h>
#include <json/json.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
namespace mysylar {
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
std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string &prefix = "");
/**
 * @brief 获取当前ms
 */
uint64_t GetCurrentMS();
/**
 * @brief 获取当前us (not use)
 */
uint64_t GetCurrentUS();
std::string ToUpper(const std::string &name);

std::string ToLower(const std::string &name);

std::string Time2Str(time_t ts = time(0),
                     const std::string &format = "%Y-%m-%d %H:%M:%S");
time_t Str2Time(const char *str, const char *format = "%Y-%m-%d %H:%M:%S");

class StringUtil {
 public:
  static std::string Format(const char *fmt, ...);
  static std::string Formatv(const char *fmt, va_list ap);

  static std::string UrlEncode(const std::string &str,
                               bool space_as_plus = true);
  static std::string UrlDecode(const std::string &str,
                               bool space_as_plus = true);

  static std::string Trim(const std::string &str,
                          const std::string &delimit = " \t\r\n");
  static std::string TrimLeft(const std::string &str,
                              const std::string &delimit = " \t\r\n");
  static std::string TrimRight(const std::string &str,
                               const std::string &delimit = " \t\r\n");

  static std::string WStringToString(const std::wstring &ws);
  static std::wstring StringToWString(const std::string &s);
};
template <class T>
const char *TypeToName() {
  static const char *s_name =
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  return s_name;
}

}  // namespace mysylar
#endif