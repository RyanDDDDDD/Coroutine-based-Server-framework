cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

find_package(yaml-cpp)
include_directories(${YAML_CPP_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC 
    source/log.cpp
    source/util.cpp
    source/config.cpp
    )

add_library(lib SHARED ${LIB_SRC})

# Logger test module
add_executable(test tests/test.cpp)
add_dependencies(test lib)
target_link_libraries(test lib yaml-cpp::yaml-cpp)

# Configuration test module
add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config lib)
target_link_libraries(test_config lib yaml-cpp::yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)