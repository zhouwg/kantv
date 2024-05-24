# Copyright (c) 2024- KanTV Authors

cmake_minimum_required(VERSION 3.22.1)

project(kantv-core)

set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(ggml)

add_subdirectory(ncnn/ncnn-src)
add_subdirectory(ncnn)

