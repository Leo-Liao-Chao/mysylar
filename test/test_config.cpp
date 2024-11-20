#include "../mysylar/config.h"
#include "../mysylar/log.h"
#include "../mysylar/util/util.h"

mysylar::Config::ConfigVarMap mysylar::Config::s_datas;
mysylar::ConfigVar<int>::ptr g_int_value_config = mysylar::Config::Lookup("system.port",(int)8080,"system_port");
mysylar::ConfigVar<float>::ptr g_float_value_config = mysylar::Config::Lookup("system.port",(float)10.2f,"system port");


int main(int argc, char **argv)
{
    
    // MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT()) << "xxx";
    auto logger = mysylar::LoggerMgr::GetInstance()->getRoot();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<g_int_value_config->getValue();
    MYSYLAR_LOG_INFO(MYSYLAR_LOG_ROOT())<<g_float_value_config->toString();
    
    return 0;
}