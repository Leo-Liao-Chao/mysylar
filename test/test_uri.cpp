#include "../mysylar/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    //mysylar::Uri::ptr uri = mysylar::Uri::Create("http://www.sylar.top/test/uri?id=100&name=sylar#frg");
    mysylar::Uri::ptr uri = mysylar::Uri::Create("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
    // mysylar::Uri::ptr uri = mysylar::Uri::Create("http://admin@www.sylar.top");
    //mysylar::Uri::ptr uri = mysylar::Uri::Create("http://www.sylar.top/test/uri");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
