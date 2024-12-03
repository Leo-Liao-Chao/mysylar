#include "../mysylar/mysylar.h"

mysylar::Logger::ptr g_logger = MYSYLAR_LOG_ROOT();

int count = 0;
// mysylar::RWMutex s_mutex;
mysylar::Mutex s_mutex;

void fun1()
{
    MYSYLAR_LOG_INFO(g_logger) <<" name: "<<mysylar::Thread::GetName()
                               <<" this.name "<<mysylar::Thread::GetThis()->getName()
                               <<" id: " <<mysylar::GetThreadId()
                               <<" this.id "<<mysylar::Thread::GetThis()->getId();
    for(int i = 0;i<100000;i++)
    {
        // mysylar::RWMutex::WriteLock lock(s_mutex);
        mysylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2()
{
    while(true)
    {
        MYSYLAR_LOG_INFO(g_logger)<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
    
    
}
void fun3()
{
    while (true)
    {
        MYSYLAR_LOG_INFO(g_logger)<<" ==========================================";
    }
    
}

int main(int argc,char** argv)
{
    std::vector<mysylar::Thread::ptr> thrs;
    MYSYLAR_LOG_INFO(g_logger) <<"thread test begin";
    YAML::Node root = YAML::LoadFile("/home/liaochao/workspace/mysylar/bin/config/log2.yaml");
    mysylar::Config::LoadFromYaml(root);
    for(int i=0;i<1;++i)
    {
        mysylar::Thread::ptr thr(new mysylar::Thread(&fun2,"name_"+std::to_string(i*2)));
        // mysylar::Thread::ptr thr2(new mysylar::Thread(&fun3,"name_"+std::to_string(i*2+1)));
        thrs.push_back(thr);
        // thrs.push_back(thr2);
    }
    for(size_t i=0;i<thrs.size();i++)
    {
        thrs[i]->join();
    }
    MYSYLAR_LOG_INFO(g_logger) <<"thread test end";
    MYSYLAR_LOG_INFO(g_logger) <<"count = "<<count;

    return 0;
}