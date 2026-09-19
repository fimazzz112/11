cmake_minimum_required(VERSION 3.20)

project(RustMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RustMonitor
    src/main.cpp
)

if (MSVC)
    target_compile_options(RustMonitor PRIVATE /W4)
endif()
