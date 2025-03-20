cmake_minimum_required(VERSION 2.8)
project(mysylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# add_definitions(-DNDEBUG)

set(LIB_SRC
    mysylar/log.cpp
    mysylar/util/util.cpp
    mysylar/config.cpp
    mysylar/thread.cpp
    mysylar/fiber.cpp
    mysylar/scheduler.cpp
    mysylar/iomanager.cpp
    mysylar/timer.cpp
    mysylar/mutex.cpp

    mysylar/hook.cpp
    mysylar/fd_manager.cpp
    #socket
    mysylar/address.cpp
    mysylar/socket.cpp
    #bytearray
    mysylar/bytearray.cpp

    #http
    mysylar/http/http.cpp
    mysylar/http/http_parser.cpp
    #thirdparty
    mysylar/http/httpclient_parser.rl.cc
    mysylar/http/http11_parser.rl.cc

    #TCP
    mysylar/tcp_server.cpp
    #stream
    mysylar/stream.cpp
    mysylar/streams/socket_stream.cpp
    #httpsession
    mysylar/http/http_session.cpp
    #http_server
    mysylar/http/http_server.cpp
    # servlet
    mysylar/http/servlet.cpp
    # connection
    mysylar/http/http_connection.cpp
    mysylar/uri.rl.cc
    # mysylar/http/servlets/config_servlet.cpp
    # mysylar/http/servlets/status_servlet.cpp
    # mysylar/daemon.cpp
    )

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()
find_package(jsoncpp REQUIRED)
if(jsoncpp_FOUND)

set(JSONCPP_INCLUDE_DIR /usr/include/jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIR})
endif()

add_library(mysylar ${LIB_SRC})

set(LIBS
    mysylar
    dl
    ${OPENSSL_LIBRARIES}
    yaml-cpp
    pthread

)

add_executable(test test/test.cpp)
add_dependencies(test mysylar)
target_link_libraries(test ${LIBS})

add_executable(test_config test/test_config.cpp mysylar/config.cpp)
add_dependencies(test_config mysylar)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread test/test_thread.cpp)
add_dependencies(test_thread mysylar)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util test/test_util.cpp)
add_dependencies(test_util mysylar)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber test/test_fiber.cpp)
add_dependencies(test_fiber mysylar)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler test/test_scheduler.cpp)
add_dependencies(test_scheduler mysylar)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager test/test_iomanager.cpp)
add_dependencies(test_iomanager mysylar)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook test/test_hook.cpp)
add_dependencies(test_hook mysylar)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address test/test_address.cpp)
add_dependencies(test_address mysylar)
target_link_libraries(test_address ${LIBS})

add_executable(test_socket test/test_socket.cpp)
add_dependencies(test_socket mysylar)
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray test/test_bytearray.cpp)
add_dependencies(test_bytearray mysylar)
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http test/test_http.cpp)
add_dependencies(test_http mysylar)
target_link_libraries(test_http ${LIBS})

add_executable(test_http_parser test/test_http_parser.cpp)
add_dependencies(test_http_parser mysylar)
target_link_libraries(test_http_parser ${LIBS})

add_executable(test_tcp_server test/test_tcp_server.cpp)
add_dependencies(test_tcp_server mysylar)
target_link_libraries(test_tcp_server ${LIBS})

add_executable(echo_server example/echo_server.cpp)
add_dependencies(echo_server mysylar)
target_link_libraries(echo_server ${LIBS})

add_executable(test_http_server test/test_http_server.cpp)
add_dependencies(test_http_server mysylar)
target_link_libraries(test_http_server ${LIBS})

add_executable(test_http_connection test/test_http_connection.cpp)
add_dependencies(test_http_connection mysylar)
target_link_libraries(test_http_connection ${LIBS})

add_executable(test_uri test/test_uri.cpp)
add_dependencies(test_uri mysylar)
target_link_libraries(test_uri ${LIBS})

add_executable(my_http_server samples/my_http_server.cpp)
add_dependencies(my_http_server mysylar)
target_link_libraries(my_http_server ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)