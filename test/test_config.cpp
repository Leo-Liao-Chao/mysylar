#include "../mysylar/config.h"
#include "../mysylar/log.h"
#include "../mysylar/util/util.h"
#include <yaml-cpp/yaml.h>

mysylar::Config::ConfigVarMap mysylar::Config::s_datas;
mysylar::ConfigVar<int>::ptr g_int_value_config = mysylar::Config::Lookup("system.port", (int)8080, "system_port");
mysylar::ConfigVar<float>::ptr g_float_value_config = mysylar::Config::Lookup("system.port", (float)10.2f, "system port");

void print_yaml(const YAML::Node &node, int level)
{
    if (node.IsScalar())
    {
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
        /* code */
    }
    else if (node.IsNull())
    {
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL" << " - " << node.Type() << " - " << level;
    }
    else if (node.IsMap())
    {
        for (auto it = node.begin(); it != node.end(); it++)
        {
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); i++)
        {
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log.yaml");

    print_yaml(root, 0);

    // MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<< root;
}
void test_config()
{
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"before: "<< g_int_value_config->getValue();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"before: "<< g_float_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log.yaml");
    mysylar::Config::LoadFromYaml(root);

    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"after: "<< g_int_value_config->getValue();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"after: "<< g_float_value_config->toString();
}

int main(int argc, char **argv)
{
    test_config();
    return 0;
}