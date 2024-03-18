# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: build whispercpp's bench tool for target x86-linux and target Android


cmake_minimum_required(VERSION 3.10)

project(whisperbench)
set(CMAKE_VERBOSE_MAKEFILE on  )

set(TARGET             ${TARGET_NAME})
set(PREBUIT_INC_PATH   ${CMAKE_SOURCE_DIR}/../../../prebuilts/include/)
set(PREBUIT_LIB_PATH   ${CMAKE_SOURCE_DIR}/../../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)

message("project root path: " ${PROJECT_ROOT_PATH})
message("whispercpp path  : " ${WHISPERCPP_PATH})
message("PREBUIT_INC_PATH : ${PREBUIT_INC_PATH}")

include_directories(${PREBUIT_INC_PATH}/)

add_library(kantvcore
        SHARED
        IMPORTED)

set_target_properties(kantvcore
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-core.so)

IF (${BUILD_TARGET} MATCHES "android")
    add_definitions(-D__ARM_NEON)
    link_libraries(log kantvcore android)
ENDIF()


IF (${BUILD_TARGET} MATCHES "x86")
    add_definitions(-D__gnu_linux__)
    add_definitions(-D_GNU_SOURCE)
    #TODO: failed to build bench tool when SSE3 or AVX enabled
    #add_definitions(-D__SSE3__)
    #add_definitions(-D__AVX__)
    link_libraries(pthread m)
ENDIF()

include_directories(${WHISPERCPP_PATH}/)

add_executable(${TARGET} bench.cpp
    ${WHISPERCPP_PATH}/cde_log.c
    ${WHISPERCPP_PATH}/ggml.c
    ${WHISPERCPP_PATH}/ggml-alloc.c
    ${WHISPERCPP_PATH}/ggml-backend.c
    ${WHISPERCPP_PATH}/ggml-quants.c
    ${WHISPERCPP_PATH}/whisper.cpp
)
