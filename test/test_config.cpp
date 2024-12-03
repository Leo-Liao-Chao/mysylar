#include "../mysylar/config.h"
#include "../mysylar/log.h"
#include "../mysylar/util/util.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

mysylar::ConfigVar<int>::ptr g_int_value_config = mysylar::Config::Lookup("system.port", (int)8080, "system_port");
mysylar::ConfigVar<float>::ptr g_int_valuex_config = mysylar::Config::Lookup("system.port", (float)8080, "system_port");
mysylar::ConfigVar<float>::ptr g_float_value_config = mysylar::Config::Lookup("system.value", (float)10.2f, "system value");
mysylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = mysylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");
mysylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config = mysylar::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int list");
mysylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config = mysylar::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int set");
mysylar::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config = mysylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int uset");
mysylar::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config = mysylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k", 2}}, "system str int map");
mysylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_str_int_umap_value_config = mysylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 1}}, "system str int umap");



// mysylar::Config::ConfigVarMap mysylar::Config::s_datas;

//

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
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();
#define XX(g_var, name, prefix)                                                                   \
    {                                                                                             \
        auto &v = g_var->getValue();                                                              \
        for (auto &i : v)                                                                         \
        {                                                                                         \
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << #prefix " " #name ": " << i;                  \
        }                                                                                         \
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }
#define XX_M(g_var, name, prefix)                                                                                   \
    {                                                                                                               \
        auto &v = g_var->getValue();                                                                                \
        for (auto &i : v)                                                                                           \
        {                                                                                                           \
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second << "}"; \
        }                                                                                                           \
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString();                   \
    }
    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_unordered_set_value_config, int_unordered_set, before);

    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log.yaml");
    mysylar::Config::LoadFromYaml(root);

    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    // v = g_int_vec_value_config->getValue();
    // for(auto&i : v)
    // {
    //     MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"after int_vec: "<< i;
    // }
    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_unordered_set_value_config, int_unordered_set, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

class Person
{

public:
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;
    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex << "]";
        return ss.str();
    }
    bool operator==(const Person& oth)const 
    {
        return m_name == oth.m_name && m_age ==oth.m_age && m_sex ==oth.m_sex;
    } 
};

namespace mysylar
{
    template < >
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            std::stringstream ss;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &p)
        {
            YAML::Node node;
             node["name"]= p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

}

mysylar::ConfigVar<Person>::ptr g_person = mysylar::Config::Lookup("class.person", Person(), "system person");

mysylar::ConfigVar<std::map<std::string, Person>>::ptr g_str_person_map_value_config = mysylar::Config::Lookup("class.map", std::map<std::string, Person>(), "system str person map");

mysylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_str_vec_person_map_value_config = mysylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system str vecperson map");
void test_class()
{
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix)                                                                           \
    {                                                                                                  \
        auto m = g_var->getValue();                                                                    \
        for (auto &i : m)                                                                              \
        {                                                                                              \
            MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << prefix << i.first << " - " << i.second.toString(); \
        }                                                                                              \
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << prefix << ": siez= " << m.size();                      \
    }
    g_person->addListener([](const Person& old_value,const Person& new_value)
    {
        MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<"old_value ="<<old_value.toString()<<"new_value ="<<new_value.toString();
    });

    
    XX_PM(g_str_person_map_value_config,"before");

    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"before: "<< g_str_vec_person_map_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log.yaml");
    mysylar::Config::LoadFromYaml(root);
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();

    XX_PM(g_str_person_map_value_config,"after");
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) <<"after: "<< g_str_vec_person_map_value_config->toString();

}



void test_log()
{
    static mysylar::Logger::ptr system_log = MYSYLAR_LOG_NAME("system");
    MYSYLAR_LOG_INFO(system_log) << "hello system";
    std::cout << mysylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log.yaml");
    mysylar::Config::LoadFromYaml(root);
    std::cout << "=======" << std::endl;
    std::cout << mysylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=======" << std::endl;
    std::cout << root << std::endl;
    MYSYLAR_LOG_INFO(system_log) << "hello system " << std::endl;

    system_log->setFormatter("%d - %m%n");
    MYSYLAR_LOG_INFO(system_log) << "hello system " << std::endl;
}

int main(int argc, char **argv)
{
    // test_config();
    // test_class();
    // std::cout << "Debug 1" <<std::endl;
    test_log();
    mysylar::Config::Visit([](mysylar::ConfigVarBase::ptr var)
                           { MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "name = " << var->getName() << " description=" << var->getDescription() << " typename=" << var->getTypeName() << " value=" << var->toString(); });
    return 0;
}