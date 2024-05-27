cmake_minimum_required(VERSION 3.28)
project(bencode_cpp)

set(CMAKE_CXX_STANDARD 17)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

find_package(Python3 COMPONENTS Interpreter Development)

# add python includes to find <Python.h>
include_directories(${Python3_INCLUDE_DIRS})

include_directories(./src/bencode_cpp/)

include_directories(./vendor/fmt/include/)
include_directories(./.venv/Lib/site-packages/pybind11/include/)

add_executable(
        bencode_cpp
        src/bencode_cpp/bencode.cpp
        src/bencode_cpp/encode.cpp
        src/bencode_cpp/decode.cpp
        src/bencode_cpp/overflow.h
        src/bencode_cpp/common.h
        src/bencode_cpp/ctx.h
)
