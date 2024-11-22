cmake_minimum_required(VERSION 2.8)
project(mysylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    mysylar/log.cpp
    mysylar/util/util.cpp
    mysylar/config.cpp
    )

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIRS})
# link_directories(${YAML_CPP_LIBRARY_DIRS})
# add_executable(${PROJECT_NAME} ${SOURCE_FILES})
# target_link_libraries(${PROJECT_NAME} ${YAML_CPP_LIBRARIES})

# include_directories(mysylar)

add_library(mysylar ${LIB_SRC})
# add_library(mysylar_static STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES(mysylar_static PROPERTIES OUTPUT_NAME "mysylar")

add_executable(test test/test.cpp)
add_dependencies(test mysylar)
target_link_libraries(test mysylar yaml-cpp)


add_executable(test_config test/test_config.cpp mysylar/config.cpp)
add_dependencies(test_config mysylar)
target_link_libraries(test_config mysylar yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
