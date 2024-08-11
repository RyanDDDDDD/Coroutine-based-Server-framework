cmake_minimum_required(VERSION 3.22.1)

project(ServerFramework)

include(cmake/utils.cmake) 

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_PREFIX_PATH 
    "/home/linuxbrew/.linuxbrew/opt/yaml-cpp" 
    "/home/linuxbrew/.linuxbrew/opt/boost")

find_package(Boost)
include_directories(${Boost_INCLUDE_DIR})

find_package(yaml-cpp)
include_directories(${YAML_CPP_INCLUDE_DIR})

include_directories(".")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -g -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC 
    source/log.cpp
    source/util.cpp
    source/config.cpp
    source/thread.cpp
    source/fiber.cpp
    source/mutex.cpp
    source/scheduler.cpp
    source/iomanager.cpp
    )

add_library(lib SHARED ${LIB_SRC})

force_redefine_file_macro_for_sources(lib)          # redefine __FILE__

set(LIBS 
    lib
    pthread
    ${Boost_LIBRARIES}
    yaml-cpp::yaml-cpp
    )

# Logger test module
add_executable(test_logger tests/test_logger.cpp)
add_dependencies(test_logger lib)
force_redefine_file_macro_for_sources(test_logger)         # redefine __FILE__
target_link_libraries(test_logger ${LIBS})

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

# Util test module
add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util lib)
force_redefine_file_macro_for_sources(test_util)    # redefine __FILE__
target_link_libraries(test_util ${LIBS})

# Fiber test module
add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber lib)
force_redefine_file_macro_for_sources(test_fiber)    # redefine __FILE__
target_link_libraries(test_fiber ${LIBS})

# Scheduler test module
add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler lib)
force_redefine_file_macro_for_sources(test_scheduler)    # redefine __FILE__
target_link_libraries(test_scheduler ${LIBS})

# IOmanager test module
add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager lib)
force_redefine_file_macro_for_sources(test_iomanager)    # redefine __FILE__
target_link_libraries(test_iomanager ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)