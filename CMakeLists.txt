cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost)

include_directories(${Boost_INCLUDE_DIRS})

# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -std=c++20 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC 
    source/log.cpp
    source/util.cpp
    source/config.cpp
    )

add_library(lib SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test lib)
target_link_libraries(test lib)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config lib)
target_link_libraries(test_config lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)