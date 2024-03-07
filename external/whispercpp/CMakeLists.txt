# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: build libwhispercpp.so for target Android


cmake_minimum_required(VERSION 3.10)

project(whispercpp)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 11)

set(WHISPERCPP_SRC_DIR         ${CMAKE_SOURCE_DIR}/)
set(PREBUIT_LIB_PATH           ${PROJECT_SOURCE_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)

set(SOURCE_FILES
        ${WHISPERCPP_SRC_DIR}/ggml.c
        ${WHISPERCPP_SRC_DIR}/ggml-alloc.c
        ${WHISPERCPP_SRC_DIR}/ggml-backend.c
        ${WHISPERCPP_SRC_DIR}/ggml-quants.c
        ${WHISPERCPP_SRC_DIR}/whisper.cpp
        ${WHISPERCPP_SRC_DIR}/cde_log.c
        ${WHISPERCPP_SRC_DIR}/jni/whispercpp-jni.c
)

message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("PREBUIT_LIB_PATH     : ${PREBUIT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("LOCAL_WHISPERCPP_PATH: ${LOCAL_WHISPERCPP_PATH}")

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${WHISPERCPP_SRC_DIR}/jni)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)


find_library(LOG_LIB log)

add_library(kantvcore
        SHARED
        IMPORTED)

set_target_properties(kantvcore
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-core.so)

function(build_library target_name)
    add_library(
        ${target_name}
        SHARED
        ${SOURCE_FILES}
    )

    target_link_libraries(${target_name} ${LOG_LIB}  kantvcore android)

    if (${target_name} STREQUAL "whisper_v8fp16_va")
        target_compile_options(${target_name} PRIVATE -march=armv8.2-a+fp16)
    elseif (${target_name} STREQUAL "whisper_vfpv4")
        target_compile_options(${target_name} PRIVATE -mfpu=neon-vfpv4)
    endif ()

    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        target_compile_options(${target_name} PRIVATE -O3)
        target_compile_options(${target_name} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
        target_compile_options(${target_name} PRIVATE -ffunction-sections -fdata-sections)

        #target_link_options(${target_name} PRIVATE -Wl,--gc-sections)
        #target_link_options(${target_name} PRIVATE -Wl,--exclude-libs,ALL)
        #target_link_options(${target_name} PRIVATE -flto)
    endif ()
endfunction()

build_library("whispercpp")
