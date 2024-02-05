cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -std=c++20 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC 
    source/log.cc
    source/util.cc
    )

add_library(logger SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test logger)
target_link_libraries(test logger)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)