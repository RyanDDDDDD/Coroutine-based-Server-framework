cmake_minimum_required(VERSION 3.22.1)

project(ServerFramework)

include(cmake/utils.cmake) 

set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

find_package(yaml-cpp)
include_directories(${YAML_CPP_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC 
    source/log.cpp
    source/util.cpp
    source/config.cpp
    source/thread.cpp
    )

add_library(lib SHARED ${LIB_SRC})

force_redefine_file_macro_for_sources(lib)          # redefine __FILE__

set(LIBS 
    lib
    pthread
    yaml-cpp::yaml-cpp
    )

# Logger test module
add_executable(test tests/test.cpp)
add_dependencies(test lib)
force_redefine_file_macro_for_sources(test)         # redefine __FILE__
target_link_libraries(test ${LIBS})

# Configuration test module
add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config lib)
force_redefine_file_macro_for_sources(test_config)  # redefine __FILE__
target_link_libraries(test_config ${LIBS})

# Thread test module
add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread lib)
force_redefine_file_macro_for_sources(test_thread)  # redefine __FILE__
target_link_libraries(test_thread ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)