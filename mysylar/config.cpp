#include "./config.h"
namespace mysylar {

static void ListAllMember(
    const std::string &prefix, const YAML::Node &node,
    std::list<std::pair<std::string, const YAML::Node>> &output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") !=
      std::string::npos) {
    MYSYLAR_LOG_ERROR(MYSYLAR_LOG_ROOT())
        << "Config invalid name: " << prefix << " : " << node;
    return;
  }
  // 加载配置参数(name , value)
  output.push_back(std::make_pair(prefix, node));

  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); it++) {
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}
void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  RWMutexType::ReadLock lock(GetMutex());
  ConfigVarMap &m = GetDatas();
  for (auto it = m.begin(); it != m.end(); it++) {
    cb(it->second);
  }
}
void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllMember("", root, all_nodes);

  for (auto &i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }
    // 转换为小写
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);
    // 如果配置参数存在
    if (var) {
      if (i.second.IsScalar()) {
        var->fromString(i.second.Scalar());
      } else {
        // 不存在,创建变量
        std::stringstream ss;
        ss << i.second;
        var->fromString(ss.str());
      }
    }
  }
}
}  // namespace mysylar