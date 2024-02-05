cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG master # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_GetProperties(yaml-cpp)

if(NOT yaml-cpp_POPULATED)
  message(STATUS "Fetching yaml-cpp...")
  FetchContent_Populate(yaml-cpp)
  add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
  include_directories(${YAML_CPP_INCLUDE_DIRS})
#   include_directories(${YAML_CPP_INCLUDE_DIRS}) # should use target_include_directories
endif()

# target_link_libraries(YOUR_LIBRARY PUBLIC yaml-cpp::yaml-cpp) # The library or executable that require yaml-cpp library

# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CXX_STANDARD C++20)
set(CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC 
    source/log.cpp
    source/util.cpp
    source/config.cpp
    )

add_library(lib SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test lib)
target_link_libraries(test PUBLIC yaml-cpp::yaml-cpp lib)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config lib)
target_link_libraries(test_config PUBLIC yaml-cpp::yaml-cpp lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)