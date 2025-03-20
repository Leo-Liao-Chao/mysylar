#ifndef __MYSYLAR_ENDIAN_H__
#define __MYSYLAR_ENDIAN_H__

#define MYSYLAR_LITTLE_ENDIAN 1
#define MYSYLAR_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

namespace mysylar {
/**
 * @brief 8字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(
    T value) {
  return (T)bswap_64((uint64_t)value);
}

/**
 * @brief 4字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(
    T value) {
  return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 2字节类型的字节序转化
 */
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(
    T value) {
  return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define MYSYLAR_BYTE_ORDER MYSYLAR_BIG_ENDIAN
#else
#define MYSYLAR_BYTE_ORDER MYSYLAR_LITTLE_ENDIAN
#endif

#if MYSYLAR_BYTE_ORDER == MYSYLAR_BIG_ENDIAN

/**
 * @brief 字节序转化成网络字节序，大端机器上的无转换（大端无需转换，小端需要转换（但是不知道是大端小端，所以，写一个小端转换。但是由于机器是大端，所以不会改变什么））
 */
template <class T>
T byteswapOnLittleEndian(T t) {
  return t;
}

/**
 * @brief 大端机器上的字节序转化，转换
 */
template <class T>
T byteswapOnBigEndian(T t) {
  return byteswap(t);
}
#else

/**
 * @brief 小端机器上的字节序转化，转换
 */
template <class T>
T byteswapOnLittleEndian(T t) {
  return byteswap(t);
}

/**
 * @brief 小端机器上的字节序转化，无转换
 */
template <class T>
T byteswapOnBigEndian(T t) {
  return t;
}
#endif

}  // namespace mysylar
#endif