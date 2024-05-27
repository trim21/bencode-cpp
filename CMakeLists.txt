cmake_minimum_required(VERSION 3.28)
project(bencode_cpp)

set(CMAKE_CXX_STANDARD 23)

find_package(Python3 COMPONENTS Development)

# add python includes to find <Python.h>
include_directories(${Python3_INCLUDE_DIRS})

include_directories(src/bencode_cpp)

include_directories(vendor/klib)
include_directories(./.venv/Lib/site-packages/pybind11/include)

add_executable(
        bencode_cpp
        src/bencode_cpp/overflow.h
        src/bencode_cpp/common.h
        src/bencode_cpp/bencode.cpp
        src/bencode_cpp/encode.cpp
        src/bencode_cpp/str.h
        src/bencode_cpp/ctx.h
        src/bencode_cpp/decode.cpp
)
