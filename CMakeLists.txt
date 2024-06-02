cmake_minimum_required(VERSION 3.28)
project(bencode_cpp)

set(CMAKE_CXX_STANDARD 17)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake CACHE FILEPATH "Path to toolchain")

# add python includes to find <Python.h>
find_package(Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIRS})

add_executable(
        bencode_cpp
        src/bencode_cpp/bencode.cpp
        src/bencode_cpp/encode.cpp
        src/bencode_cpp/decode.cpp
        src/bencode_cpp/overflow.h
        src/bencode_cpp/common.h
        src/bencode_cpp/ctx.h
)

include_directories(./src/bencode_cpp/)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(bencode_cpp PRIVATE fmt::fmt-header-only)

include_directories(./.venv/Lib/site-packages/pybind11/include/)
