cmake_minimum_required(VERSION 3.5)
project(ZIP301)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(Decompress
        decompression/Decompress.cpp
)

add_executable(Compress
        compression/Compress.cpp)
