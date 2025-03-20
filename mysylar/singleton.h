/**
 * @file singleton.h
 * @brief 单例模式封装
 * @details 通过模板类实现单例模式
 */
#ifndef __MYSYLAR_SINGLETON_H__
#define __MYSYLAR_SINGLETON_H__

#include <memory>

namespace mysylar {
template <class T, class X = void, int N = 0>
class Singleton {
 public:
  static T *GetInstance() {
    static T v;
    return &v;
  }
};
template <class T, class X = void, int N = 0>
class Singletonptr {
 public:
  std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};
}  // namespace mysylar

#endif