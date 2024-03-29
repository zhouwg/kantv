# Copyright (c) 2024, zhou.weiguo(zhouwg2000@gmail.com)

# Copyright (c) 2024- KanTV Authors

# Description: build libggml-jni.so for target Android


cmake_minimum_required(VERSION 3.22.1) # make llamacpp happy

project(ggml-jni)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 11)

set(WHISPERCPP_SRC_DIR ${CMAKE_SOURCE_DIR}/)
set(PREBUIT_LIB_PATH ${CMAKE_SOURCE_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUIT_INC_PATH ${CMAKE_SOURCE_DIR}/../../prebuilts/include/)
set(LLAMACPP_SRC_DIR ${CMAKE_SOURCE_DIR}/llamacpp/)

#03-29-2024, PoC#121:Add Qualcomm mobile SoC native backend for GGML
set(QNN_INC_PATH  ${CMAKE_SOURCE_DIR}/../../prebuilts/include/QNN)
set(QNN_LIB_PATH  ${CMAKE_SOURCE_DIR}/../../prebuilts/lib/)
set(QNN_SRC_DIR   ${CMAKE_SOURCE_DIR}/qnnsample/)
include_directories(${QNN_INC_PATH}/)
include_directories(${QNN_SRC_DIR}/)
include_directories(${QNN_SRC_DIR}/Utils)
include_directories(${QNN_SRC_DIR}/Log)
include_directories(${QNN_SRC_DIR}/WrapperUtils)
include_directories(${QNN_SRC_DIR}/PAL/include)

set(SOURCE_FILES
        ${WHISPERCPP_SRC_DIR}/ggml.c
        ${WHISPERCPP_SRC_DIR}/ggml-alloc.c
        ${WHISPERCPP_SRC_DIR}/ggml-backend.c
        ${WHISPERCPP_SRC_DIR}/ggml-quants.c

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${LLAMACPP_SRC_DIR}/llama.cpp
        ${LLAMACPP_SRC_DIR}/unicode.cpp
        ${LLAMACPP_SRC_DIR}/unicode-data.cpp

        ${WHISPERCPP_SRC_DIR}/jni/ggml-jni.c
        ${WHISPERCPP_SRC_DIR}/jni/ggml-jni-impl.cpp

        ${WHISPERCPP_SRC_DIR}/jni/tinywav.c
        ${WHISPERCPP_SRC_DIR}/jni/sampling.cpp
        ${WHISPERCPP_SRC_DIR}/jni/common.cpp
        ${WHISPERCPP_SRC_DIR}/jni/grammar-parser.cpp
        ${WHISPERCPP_SRC_DIR}/jni/llama-bench.cpp

        ${QNN_SRC_DIR}/QnnSampleMain.cpp
        ${QNN_SRC_DIR}/QnnSampleApp.cpp
        ${QNN_SRC_DIR}/Log/Logger.cpp
        ${QNN_SRC_DIR}/Log/LogUtils.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/Directory.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/DynamicLoading.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/FileOp.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/Path.cpp
        ${QNN_SRC_DIR}/PAL/src/common/GetOpt.cpp
        ${QNN_SRC_DIR}/PAL/src/common/StringOp.cpp
        ${QNN_SRC_DIR}/Utils/DataUtil.cpp
        ${QNN_SRC_DIR}/Utils/DynamicLoadUtil.cpp
        ${QNN_SRC_DIR}/Utils/IOTensor.cpp
        ${QNN_SRC_DIR}/Utils/QnnSampleAppUtils.cpp
        ${QNN_SRC_DIR}/WrapperUtils/QnnWrapperUtils.cpp
)


message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("PREBUIT_INC_PATH     : ${PREBUIT_INC_PATH}")
message("PREBUIT_LIB_PATH     : ${PREBUIT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("LOCAL_WHISPERCPP_PATH: ${LOCAL_WHISPERCPP_PATH}")
message("LLAMACPP_SRC_DIR     : ${LLAMACPP_SRC_DIR}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${WHISPERCPP_SRC_DIR}/jni)
include_directories(${PREBUIT_INC_PATH}/)

#re-use the ggml.h in subdirectory llamacpp/ggml.h to avoid NDK complain "error: redefinition of 'ggml_status'"
include_directories(${LLAMACPP_SRC_DIR}/)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)

add_definitions(-DNDEBUG)
add_definitions(-O3)

#add_subdirectory(${LLAMACPP_SRC_DIR})


#weiguo:2024-03-10
# the below build optimization might be works well on ALL mainstream Android phones
# but NOT work with realtime subtitle feature(app would crash)
# so it's default enabled
#add_definitions(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)

#weiguo:2024-03-11
# the below special build optimization ONLY validated ok on Xiaomi 14
# works very well on Xiaomi 14 and got best ASR performance until now(less then 1 second)
# manully enable it for Xiaomi 14
add_definitions(-march=armv8.7-a)
add_definitions(-mcpu=cortex-x1)
add_definitions(-mtune=cortex-x1)

find_library(LOG_LIB log)

add_library(kantvcore
        SHARED
        IMPORTED)

set_target_properties(kantvcore
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-core.so)

add_library(kantvffmpeg
        SHARED
        IMPORTED)

set_target_properties(kantvffmpeg
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUIT_LIB_PATH}/libkantv-ffmpeg.so)


function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    target_link_libraries(${target_name} ${LOG_LIB} kantvcore kantvffmpeg android)

endfunction()

build_library("ggml-jni")
