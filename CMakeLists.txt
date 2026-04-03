cmake_minimum_required(VERSION 4.1)
project(ZIP301)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(ZIP301
        Decompress.cpp
        Decompress.h
        Compress.cpp
        Compress.h)
