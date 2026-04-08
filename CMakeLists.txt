cmake_minimum_required(VERSION 4.1)
project(ZIP301)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Decompress
        decompression/Decompress.cpp
        decompression/Decompress.h)

add_executable(Compress
        compression/Compress.cpp
        compression/Compress.h
        compression/KanaanCompress.cpp)
