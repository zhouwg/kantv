# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: build libwhispercpp.so for target Android


cmake_minimum_required(VERSION 3.10)

project(whispercpp)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 11)

set(WHISPERCPP_SRC_DIR ${CMAKE_SOURCE_DIR}/)
set(PREBUIT_LIB_PATH ${CMAKE_SOURCE_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUIT_INC_PATH ${CMAKE_SOURCE_DIR}/../../prebuilts/include/)

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
message("PREBUIT_INC_PATH     : ${PREBUIT_INC_PATH}")
message("PREBUIT_LIB_PATH     : ${PREBUIT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("LOCAL_WHISPERCPP_PATH: ${LOCAL_WHISPERCPP_PATH}")

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${WHISPERCPP_SRC_DIR}/jni)
include_directories(${PREBUIT_INC_PATH}/)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)
#add_definitions(-DGGML_USE_ACCELERATE)
#add_definitions(-DGGML_USE_CLBLAST)

#weiguo,2024-03-11,morning,     mightbe slower when armv8/armv9 + simd is enabled
#weiguo,2024-03-11,17:41        confirmed that slower about 2x with optimized version
#add_definitions(-DGGML_USE_OPENBLAS)

add_definitions(-DNDEBUG)
add_definitions(-O3)

#weiguo:2024-03-10
#add_definitions(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)


#weiguo:2024-03-11
#only validated ok on Xiaomi 14
#work
#add_definitions(-march=armv8-a+simd+fp)
#work
#add_definitions(-march=armv8.5-a)
#work
#add_definitions(-march=armv8.6-a)

#works very well on Xiaomi 14 and got best ASR performance until now(less then 1 second)
#add_definitions(-march=armv8.7-a)
#add_definitions(-mcpu=cortex-x1)
#add_definitions(-mtune=cortex-x1)

#2024-3-12,14:45
#ASR framework works well as expected validate on vivo phone(Android 10 + low-end qcom SoC)
#might be works well on all mainstream Android phones
add_definitions(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)

#instruction exception
#add_definitions(-march=armv8.8-a)
#add_definitions(-mcpu=cortex-x2)
#add_definitions(-mtune=cortex-x2)

#instruction exception
#add_definitions(-march=armv9+simd)
#add_definitions(-mcpu=cortex-x3)

#not supported with latest android-ndk-r26c
#add_definitions(-march=armv9.2+simd)
#add_definitions(-mcpu=cortex-x4)



add_definitions(-mfloat-abi=hard -mfpu=neon-fp-armv8)
#add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden)
#brings much side effect
#add_definitions(-funsafe-math-optimizations)

find_library(LOG_LIB log)

add_library(kantvcore
        SHARED
        IMPORTED)

set_target_properties(kantvcore
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-core.so)

add_library(kantvplay
        SHARED
        IMPORTED)

set_target_properties(kantvplay
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-play.so)

add_library(openblas
        SHARED
        IMPORTED)

set_target_properties(openblas
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libopenblas.so)

function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    target_link_libraries(${target_name} ${LOG_LIB} kantvcore kantvplay openblas android)

endfunction()

build_library("whispercpp")
