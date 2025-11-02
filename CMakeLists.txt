cmake_minimum_required(VERSION 3.15)
project(gitlite CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

list(APPEND SOURCES main.cpp)

add_executable(gitlite ${SOURCES})

