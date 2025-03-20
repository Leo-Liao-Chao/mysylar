本项目基于bilibili up主sylar-yin，自己学习并实现了80集的视频内容。

哔哩哔哩：[\[C++高级教程\]从零开始开发服务器框架(sylar)_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV184411s7qF/?vd_source=739c28b5fa3568496fa4742b9c6b5cb5)

GitHub：GitHub - sylar-yin/sylar: [C++高性能分布式服务器框架,webserver,websocket server,自定义tcp_server（包含日志模块，配置模块，线程模块，协](https://github.com/sylar-yin/sylar)

- 开始时间：2024/10/23
- 进度：80/80

开发环境
- Ubuntu20.04
- Cmake:3.16.3
- Gcc:9.4.0
环境配置：
- sudo apt-get update
- sudo apt-get install cmake g++：编译器安装

---
Config模块
- sudo apt-get install libboost-all-dev：boost库安装，Boost 是一个广泛使用的 C++ 库集合，提供了许多功能，包括数据结构、算法、并发编程、文件系统和网络编程等。
- yaml-cpp:mkidr build && cd build && cmake ..&&make install
CmakeList.txt
find_package(yaml-cpp REQUIRED)    
include_directories(${YAML_CPP_INCLUDE_DIR})   
target_link_libraries(...  yaml-cpp) 

Socket模块
- sudo apt install libssl-dev:OpenSSL库安装，OpenSSL 是一个强大的工具，用于加密和安全通信，支持 SSL/TLS 协议。

Http模块
- ragel：http11_common.h  http11_parser.h  httpclient_parser.h http11_parser.rl httpclient_parser.rlhttp解析
  - ragel:sudo apt-get install ragel
  - http:sudo ragel -G2 -C http11_parser.rl -o http11_parser.cc,sudo ragel -G2 -C httpclient_parser.rl -o httpclient_parser.cc
    - 注释.rl中，//#include <dbg.h>
    - 在httpclient_parser.rl,加入#include <cerrno>,#define check(A, M, ...) if(!(A)) { /*log_err(M, ##**__VA_ARGS*__*);*/ errno=0; goto error; }
    - mv http11_parser.cc http11_parser.rl.cc,mv httpclient_parser.cc httpclient_parser.rl.cc
    - 在头文件.h中修改#include <http11/http11_common.h>为#include "./http11_common.h"
编译
- 在mysylar目录下cmake . 和make

[本人记录的笔记如下：供学习，有误请指正。](https://hxindxrjc6m.feishu.cn/docx/HuByd9Flqo14tXxVp92ckpgnnyh?from=from_copylink)

联系方式：liao__chao@163.com