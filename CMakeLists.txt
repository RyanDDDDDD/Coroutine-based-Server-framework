cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON) 

project("ServerFramework")

add_subdirectory("source")

add_executable(${PROJECT_NAME} main.cpp)