#include "./bytearray.h"

#include <string.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#include "./endian.h"
#include "./log.h"
#include <cmath>

namespace mysylar {

static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}

ByteArray::Node::Node() : ptr(nullptr), next(nullptr), size(0) {}

ByteArray::Node::~Node() {
  if (ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size), // (n)每个内存块固定大小
      m_position(0), // 整个bytearray的位置
      m_capacity(base_size), // 总容量
      m_size(0), // 存储数据的大小
      m_endian(MYSYLAR_BIG_ENDIAN),
      m_root(new Node(base_size)),
      m_cur(m_root) {}

ByteArray::~ByteArray() {
  Node *tmp = m_root;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
}

bool ByteArray::isLittleEndian() const {
  return m_endian == MYSYLAR_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
  if (val) {
    m_endian = MYSYLAR_LITTLE_ENDIAN;
  } else {
    m_endian = MYSYLAR_BIG_ENDIAN;
  }
}

void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }

void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }
void ByteArray::writeFint16(int16_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
  if (m_endian != MYSYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t &v) {
  if (v < 0) {
    return ((uint32_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static uint64_t EncodeZigzag64(const int64_t &v) {
  if (v < 0) {
    return ((uint64_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static int32_t DecodeZigzag32(const uint32_t &v) { return (v >> 1) ^ -(v & 1); }

static int64_t DecodeZigzag64(const uint64_t &v) { return (v >> 1) ^ -(v & 1); }

void ByteArray::writeInt32(int32_t value) {
  writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value) {
  uint8_t tmp[5];
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

void ByteArray::writeInt64(int64_t value) {
  writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value) {
  uint8_t tmp[10];
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

void ByteArray::writeFloat(float value) {
  uint32_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint32(v);
}

void ByteArray::writeDouble(double value) {
  uint64_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string &value) {
  writeFuint16(value.size());
  write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string &value) {
  writeFuint32(value.size());
  write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string &value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string &value) {
  writeUint64(value.size());
  write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string &value) {
  write(value.c_str(), value.size());
}

int8_t ByteArray::readFint8() {
  int8_t v;
  read(&v, sizeof(v));
  return v;
}

uint8_t ByteArray::readFuint8() {
  uint8_t v;
  read(&v, sizeof(v));
  return v;
}

#define XX(type)                        \
  type v;                               \
  read(&v, sizeof(v));                  \
  if (m_endian == MYSYLAR_BYTE_ORDER) { \
    return v;                           \
  } else {                              \
    return byteswap(v);                 \
  }

int16_t ByteArray::readFint16() { XX(int16_t); }
uint16_t ByteArray::readFuint16() { XX(uint16_t); }

int32_t ByteArray::readFint32() { XX(int32_t); }

uint32_t ByteArray::readFuint32() { XX(uint32_t); }

int64_t ByteArray::readFint64() { XX(int64_t); }

uint64_t ByteArray::readFuint64() { XX(uint64_t); }

#undef XX

int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }

uint32_t ByteArray::readUint32() {
  uint32_t result = 0;
  for (int i = 0; i < 32; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint32_t)b) << i;
      break;
    } else {
      result |= (((uint32_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }

uint64_t ByteArray::readUint64() {
  uint64_t result = 0;
  for (int i = 0; i < 64; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint64_t)b) << i;
      break;
    } else {
      result |= (((uint64_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

float ByteArray::readFloat() {
  uint32_t v = readFuint32();
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

double ByteArray::readDouble() {
  uint64_t v = readFuint64();
  double value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

std::string ByteArray::readStringF16() {
  uint16_t len = readFuint16();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string ByteArray::readStringF32() {
  uint32_t len = readFuint32();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string ByteArray::readStringF64() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string ByteArray::readStringVint() {
  uint64_t len = readUint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

void ByteArray::clear() {
  m_position = m_size = 0; 
  m_capacity = m_baseSize;// node x n
  Node *tmp = m_root->next;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = NULL;
}

void ByteArray::write(const void *buf, size_t size) {
  if (size == 0) {
    return;
  }
  addCapacity(size);

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
// npos: 当前内存块的写入位置
// ncap: 当前内存块的剩余容量
// bpos: 数据缓冲区的当前位置

  while (size > 0) {
    if (ncap >= size) {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);
      if (m_cur->size == (npos + size)) {
        // 下一个块
        m_cur = m_cur->next;
      }
      m_position += size;
      bpos += size;
      size = 0;
    } else {
      // 内存足够
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
      m_position += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }

  if (m_position > m_size) {
    m_size = m_position;
  }
}

void ByteArray::read(void *buf, size_t size) {
  if (size > getReadSize()) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_position += size;
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
      m_position += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
}

void ByteArray::read(void *buf, size_t size, size_t position) const {
  if (size > (m_size - position)) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  // npos: 当前内存块的读取位置
  // ncap: 当前内存块的剩余容量
  // bpos: 数据缓冲区的当前位置  
  Node *cur = m_cur;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, cur->ptr + npos, size);
      if (cur->size == (npos + size)) {
        // 刚好读完一个内存块
        cur = cur->next;
      }
      position += size;
      bpos += size;
      size = 0;
    } else {
      // 当前内存块不够读，需要读下一个内存块
      memcpy((char *)buf + bpos, cur->ptr + npos, ncap);
      position += ncap;
      bpos += ncap;
      size -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
  }
}

void ByteArray::setPosition(size_t v) {
  if (v > m_capacity) {
    throw std::out_of_range("set_position out of range");
  }
  m_position = v;
  if (m_position > m_size) {
    m_size = m_position;
  }
  m_cur = m_root;
  while (v > m_cur->size) {
    v -= m_cur->size;
    m_cur = m_cur->next;
  }
  if (v == m_cur->size) {
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string &name) const {
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    MYSYLAR_LOG_ERROR(g_logger)
        << "writeToFile name=" << name << " error , errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }

  int64_t read_size = getReadSize();
  int64_t pos = m_position;
  Node *cur = m_cur;

  while (read_size > 0) {
    // 起始偏移
    int diff = pos % m_baseSize;
    // 当前内存块剩余可读大小
    int64_t len =
        (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
    ofs.write(cur->ptr + diff, len);
    cur = cur->next;
    pos += len;
    read_size -= len;
  }

  return true;
}

bool ByteArray::readFromFile(const std::string &name) {
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);
  if (!ifs) {
    MYSYLAR_LOG_ERROR(g_logger)
        << "readFromFile name=" << name << " error, errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }
  // 自动释放
  std::shared_ptr<char> buff(new char[m_baseSize],
                             [](char *ptr) { delete[] ptr; });
  while (!ifs.eof()) {
    // 按大小读
    ifs.read(buff.get(), m_baseSize);
    // 写入
    write(buff.get(), ifs.gcount());
  }
  return true;
}

void ByteArray::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  // 容量足够
  size_t old_cap = getCapacity();
  if (old_cap >= size) {
    return;
  }

  size = size - old_cap;
  // 需要多少个内存快
  size_t count = ceil(1.0 * size / m_baseSize);
  Node *tmp = m_root;
  while (tmp->next) {
    tmp = tmp->next;
  }
  // 扩容 tmp最后一个指针
  // 扩容第一个指针
  Node *first = NULL;
  for (size_t i = 0; i < count; ++i) {
    tmp->next = new Node(m_baseSize);
    if (first == NULL) {
      first = tmp->next;
    }
    tmp = tmp->next;
    m_capacity += m_baseSize;
  }
  // 扩容前第一个指针
  if (old_cap == 0) {
    m_cur = first;
  }
}

std::string ByteArray::toString() const {
  std::string str;
  str.resize(getReadSize());
  if (str.empty()) {
    return str;
  }
  read(&str[0], str.size(), m_position);
  return str;
}

std::string ByteArray::toHexString() const {
  std::string str = toString();
  std::stringstream ss;
  // Hello, World! This is a test of the toHexString method.
  // 48 65 6c 6c 6f 2c 20 57 6f 72 6c 64 21 20 54 68 69 73 20 69 73 20 61 20 74 65 73 74 20 6f 66 20 
// 74 68 65 20 74 6f 48 65 78 53 74 72 69 6e 67 20 6d 65 74 68 6f 64 2e 
  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i]
       << " ";
  }

  return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;

  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position % m_baseSize;
  size_t count = position / m_baseSize;
  Node *cur = m_root;
  while (count > 0) {
    cur = cur->next;
    --count;
  }

  size_t ncap = cur->size - npos;
  struct iovec iov;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }
  addCapacity(len);
  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

}  // namespace mysylar
