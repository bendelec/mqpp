cmake_minimum_required(VERSION 3.0)
project(example_project)


file(GLOB_RECURSE sources       src/*.cpp)

add_library(mqpp SHARED ${sources}) 
target_include_directories(mqpp PRIVATE include interface)
target_compile_options(mqpp PRIVATE -Wall -Wextra -std=c++11 -fno-rtti -fomit-frame-pointer -O2)

add_executable(mqttest test/test_main.cpp)
target_include_directories(mqttest PRIVATE interface)
target_link_libraries(mqttest mqpp)
