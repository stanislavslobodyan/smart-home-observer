cmake_minimum_required(VERSION 3.16)

project(smart_home_observer LANGUAGES CXX)

add_executable(smart_home main.cpp)
target_compile_features(smart_home PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(smart_home PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(smart_home PRIVATE -Wall -Wextra -Wpedantic)
endif()
