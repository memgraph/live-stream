cmake_minimum_required(VERSION 3.8)

project(memgraph-pagerank)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_library(example SHARED example.cpp)
target_include_directories(example PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_library(pagerank SHARED pagerank.cpp)
target_include_directories(pagerank PRIVATE ${CMAKE_SOURCE_DIR}/include)
