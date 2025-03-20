/**
 * @brief
 * 1. 采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。支持级别格式的数据类型，支持STL容器(vector,list,set,map等等),支持自定义类型的支持（需要实现序列化和反序列化方法)。
 */

#ifndef __MYSYLAR_CONFIG_H__
#define __MYSYLAR_CONFIG_H__

#include <yaml-cpp/yaml.h>

#include <boost/lexical_cast.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "./log.h"
#include "./thread.h"

namespace mysylar {
class ConfigVarBase {
 public:
  typedef std::shared_ptr<ConfigVarBase> ptr;
  /**
   * @brief 构造函数
   * @param[in] name 变量名
   * @param[in] description 变量描述
   */
  ConfigVarBase(const std::string &name, const std::string &description = " ")
      : m_name(name), m_description(description) {
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  };
  virtual ~ConfigVarBase() {}
  /**
   * @brief 返回配置参数的名称
   */
  const std::string &getName() const { return m_name; }
  /**
   * @brief 返回配置参数的描述
   */
  const std::string &getDescription() const { return m_description; }

  virtual std::string toString() = 0;
  virtual bool fromString(const std::string &val) = 0;
  virtual std::string getTypeName() const = 0;

 protected:
  // 配置参数的名称
  std::string m_name;
  // 配置参数的描述
  std::string m_description;
};

// F from_type,T to_type
// 模板类LexicalCast，用于类型转换
template <class F, class T>
class LexicalCast {
 public:
  T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

// std::string to std::vector<T>
template <class T>
class LexicalCast<std::string, std::vector<T>> {
 public:
  std::vector<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};
// std::vector<T> to std::string
template <class T>
class LexicalCast<std::vector<T>, std::string> {
 public:
  std::string operator()(const std::vector<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// std::string to std::list<T>
template <class T>
class LexicalCast<std::string, std::list<T>> {
 public:
  std::list<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> list;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      list.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return list;
  }
};
// std::list<T> to std::string
template <class T>
class LexicalCast<std::list<T>, std::string> {
 public:
  std::string operator()(const std::list<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// std::string to std::set<T>
template <class T>
class LexicalCast<std::string, std::set<T>> {
 public:
  std::set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::set<T> set;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      set.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return set;
  }
};
// std::set<T> to std::string
template <class T>
class LexicalCast<std::set<T>, std::string> {
 public:
  std::string operator()(const std::set<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// std::string to std::unordered_set<T>
template <class T>
class LexicalCast<std::string, std::unordered_set<T>> {
 public:
  std::unordered_set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_set<T> set;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      set.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return set;
  }
};
// std::unordered_set<T> to std::string
template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
 public:
  std::string operator()(const std::unordered_set<T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// std::string to std::map<std::string, T>
template <class T>
class LexicalCast<std::string, std::map<std::string, T>> {
 public:
  std::map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::map<std::string, T> map;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << it->second;
      map.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return map;
  }
};
// std::map<std::string, T> to std::string
template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::map<std::string, T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};
// std::string to std::unordered_map<std::string, T>
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
 public:
  std::unordered_map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_map<std::string, T> map;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); it++) {
      ss.str("");
      ss << it->second;
      map.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return map;
  }
};
// std::unordered_map<std::string, T> to std::string
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::unordered_map<std::string, T> &v) {
    YAML::Node node;
    for (auto &i : v) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// FromStr T operator() {const std::string&}
// ToStr std::string operator()(const T&)
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
 public:
  typedef RWMutex RWMutexType;
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_value, const T &new_value)>
      on_change_cb;
  /**
   * @brief 构造函数
   * @param[in] name 配置参数名称
   * @param[in] default_value 配置参数的默认值
   * @param[in] description 配置参数的描述
   */
  ConfigVar(const std::string &name, const T &default_value,
            const std::string &description)
      : ConfigVarBase(name, description), m_val(default_value) {}
  /**
   * @brief 将配置参数转成YAML String
   */
  std::string toString() override {
    try {
      RWMutexType::ReadLock lock(m_mutex);
      // 模板类型转化为string
      return ToStr()(m_val);
    } catch (std::exception &e) {
      MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT())
          << "ConfigVar:toString exception" << e.what()
          << "convert: " << typeid(m_val).name() << "to string";
    }
    return "";
  }
  /**
   * @brief 从YAML String转成配置参数
   * @param[in] val YAML String
   */
  bool fromString(const std::string &val) override {
    try {
      setValue(FromStr()(val));
      return true;
    } catch (std::exception &e) {
      MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT())
          << "ConfigVar:toString exception" << e.what() << "convert:string to "
          << typeid(m_val).name();
      return false;
    }
  }
  /**
   * @brief 获取配置参数的值
   */
  const T getValue() {
    RWMutexType::ReadLock lock(m_mutex);
    return m_val;
  }
  /**
   * @brief 设置配置参数的值,触发回调函数
   * @param[in] v 新的配置参数值
   */
  void setValue(const T &v) {
    {
      RWMutexType::ReadLock lock(m_mutex);
      if (v == m_val) {
        return;
      }
      // 触发回调函数
      for (auto &i : m_cbs) {
        i.second(m_val, v);
      }
    }
    RWMutexType::WriteLock lock(m_mutex);
    m_val = v;
  }
  /**
   * @brief 获取配置参数的名字
   */
  std::string getTypeName() const override { return typeid(T).name(); }
  /**
   * @brief 添加变化回调函数
   * @param[in] cb 回调函数
   */
  uint64_t addListener(on_change_cb cb) {
    static uint64_t s_fun_id = 0;
    RWMutexType::WriteLock lock(m_mutex);
    ++s_fun_id;
    m_cbs[s_fun_id] = cb;
    return s_fun_id;
  }
  /**
   * @brief 删除回调函数
   * @param[in] key 回调函数的key
   */
  void delListener(uint64_t key) {
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.erase(key);
  }
  /**
   * @brief 获取回调函数
   * @param[in] key 回调函数的key
   */
  on_change_cb getListener(uint64_t key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_cbs.find(key);
    return it == m_cbs.end() ? nullptr : it->second;
  }
  /**
   * @brief 清除所有回调函数
   */
  void clearListener() {
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.clear();
  }

 private:
  RWMutex m_mutex;
  T m_val;
  // change callback group key,only one,use hash
  std::map<uint64_t, on_change_cb> m_cbs;
};
class Config {
 public:
  typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  typedef RWMutex RWMutexType;
  /**
   * @brief 查找配置参数
   * @param[in] name 配置参数名称
   */
  template <class T>
  static typename ConfigVar<T>::ptr Lookup(
      const std::string &name, const T &default_value,
      const std::string &description = "") {
    RWMutexType::WriteLock lock(GetMutex());
    auto it = GetDatas().find(name);
    // 如果找到了对应的配置参数
    if (it != GetDatas().end()) {
      // 智能指针转换,如果转换失败返回nullptr
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp) {
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())
            << "Lookup name=" << name << " exists" << std::endl;
        return tmp;
      } else {
        MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT())
            << "Lookup name=" << name << " exists but type not "
            << typeid(T).name() << " real_type=" << it->second->getTypeName()
            << " " << it->second->toString();
        return nullptr;
      }
    }
    // 如果没有找到对应的配置参数,创建配置参数
    // 检查name是否合法
    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") !=
        std::string::npos) {
      MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT())
          << "Lookup name invalid " << std::endl;
      throw std::invalid_argument(name);
    }
    // 创建配置参数
    typename ConfigVar<T>::ptr v(
        new ConfigVar<T>(name, default_value, description));
    GetDatas()[name] = v;
    return v;
  }
  /**
   * @brief 查找配置参数
   * @param[in] name 配置参数名称
   */
  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }
  static ConfigVarBase::ptr LookupBase(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
  }
  /**
   * @brief 加载配置文件
   * @param[in] root 根节点
   */
  static void LoadFromYaml(const YAML::Node &root);
  /**
   * @brief 访问每一个配置参数,触发函数
   * @param[in] cb 回调函数
   */
  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

 private:
  /**
   * @brief 返回配置参数的数据,map
   */
  static ConfigVarMap &GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }
  /**
   * @brief 返回配置参数的读写锁
   */
  static RWMutexType &GetMutex() {
    static RWMutexType s_mutex;
    return s_mutex;
  }
};
}  // namespace mysylar
#endif
