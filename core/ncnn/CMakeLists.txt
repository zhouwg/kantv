# Copyright (c) 2024- KanTV Authors

# Description: build libncnn-jni.so for Project KanTV

cmake_minimum_required(VERSION 3.22.1)
project(ncnn-jni)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(TARGET_XIAOMI14    OFF)  # set it to ON to get much better performance on Xiaomi 14 or Qualcomm Snapdragon 8 Gen 3 SoC based Android phone
#TODO: QNN backend for NCNN not supported currently
set(NCNN_ENABLE_QNN    OFF)  # set it to ON to enable QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK on Qualcomm SoC based Android phone

set(NCNN_DIR                    ${CMAKE_SOURCE_DIR}/ncnn)
set(NCNN_SRC_DIR                ${CMAKE_SOURCE_DIR}/ncnn/ncnn-src)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)
set(PREBUILT_PATH               ${NCNN_DIR}/../../prebuilts/)
set(PREBUILT_LIB_PATH           ${NCNN_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${NCNN_DIR}/../../prebuilts/include/)
set(OpenCV_DIR                  ${PREBUILT_PATH}/opencv-mobile-4.9.0-android/sdk/native/jni)
set(QNN_INC_PATH                ${NCNN_DIR}/../../prebuilts/include/QNN)
set(QNN_LIB_PATH                ${NCNN_DIR}/../../prebuilts/lib/)
set(NCNNJNI_SRC_DIR             ${NCNN_DIR}/jni/)
set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)


message("NCNN_DIR             : ${NCNN_DIR}")
message("NCNN_SRC_DIR         : ${NCNN_SRC_DIR}")
message("OpenCV_DIR           : ${OpenCV_DIR}")
message("PREBUILT_INC_PATH    : ${PREBUILT_INC_PATH}")
message("PREBUILT_LIB_PATH    : ${PREBUILT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")
message("CMAKE_BUILD_TYPE     : ${CMAKE_BUILD_TYPE}")

find_package(OpenCV REQUIRED core imgproc)

include_directories(${OpenCV_DIR}/include/)
include_directories(${NCNN_SRC_DIR}/include/) # prebuild header files of ncnn
include_directories(${PREBUILT_INC_PATH}/)
include_directories(${QNN_INC_PATH}/)
include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)
add_definitions(-O3)
add_compile_options("-Wall" "-Wno-sign-compare")

if (NCNN_ENABLE_QNN)
    add_definitions(-DNCNN_USE_QNN)
else()
    add_definitions(-DNCNN_DISABLE_QNN)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DDEBUG)
    add_definitions(-g)

else()
    add_definitions(-DNDEBUG)
    add_link_options("-s")
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    add_compile_options("-O3"
            "-ffunction-sections"
            "-fdata-sections"
            "-Wno-unused-command-line-argument")
endif()


if (TARGET_XIAOMI14)
    # the below special build optimization ONLY validated ok on Xiaomi 14
    # works very well on Xiaomi 14 and got best ASR performance until now(less then 1 second)
    # manually enable it for Xiaomi 14
    add_definitions(-march=armv8.7-a)
    add_definitions(-mcpu=cortex-x1)
    add_definitions(-mtune=cortex-x1)

else()
    # the below build optimization might be works well on ALL mainstream Android phones
    # but NOT work with realtime subtitle feature(app would crash)
    # so it's default enabled
    add_definitions(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)

endif()


set(SOURCE_FILES
        ${NCNNJNI_SRC_DIR}/ncnn-jni.c
        ${NCNNJNI_SRC_DIR}/ncnn-jni-impl.cpp
        ${NCNNJNI_SRC_DIR}/scrfd.cpp
        ${NCNNJNI_SRC_DIR}/ndkcamera.cpp
        ${NCNNJNI_SRC_DIR}/nanodet.cpp
)

if (NCNN_ENABLE_QNN)
    file(GLOB allPrebuiltQNNLibs "${QNN_LIB_PATH}/libQnn*.so")

    #file(COPY ${allPrebuiltQNNLibs}  DESTINATION ${PREBUILT_LIB_PATH}/ )

endif()

find_library(LOG_LIB log)

add_library(kantvmedia
        SHARED
        IMPORTED)

set_target_properties(kantvmedia
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libkantv-media.so)


add_library(ncnn-jni SHARED ${SOURCE_FILES})

target_link_libraries(ncnn-jni ncnn ${OpenCV_LIBS} camera2ndk mediandk ${LOG_LIB} kantvmedia android)
