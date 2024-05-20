project(ncnn-jni)

cmake_minimum_required(VERSION 3.22.1)

set(NCNN_SRC_DIR                ${CMAKE_SOURCE_DIR}/ncnn)
set(OpenCV_DIR ${NCNN_SRC_DIR}/opencv-mobile-4.9.0-android/sdk/native/jni)
set(ncnn_DIR ${NCNN_SRC_DIR}/ncnn-20240410-android-vulkan/${ANDROID_ABI}/lib/cmake/ncnn)

message("NCNN_SRC_DIR         : ${NCNN_SRC_DIR}")
message("OpenCV_DIR           : ${OpenCV_DIR}")
message("ncnn_DIR             : ${ncnn_DIR}")

find_package(OpenCV REQUIRED core imgproc)

find_package(ncnn REQUIRED)

include_directories(${OpenCV_DIR}/include/)
include_directories(${NCNN_SRC_DIR}/ncnn-20240410-android-vulkan/${ANDROID_ABI}/include/)

add_library(ncnn-jni SHARED jni/scrfdncnn.cpp jni/scrfd.cpp jni/ndkcamera.cpp)

target_link_libraries(ncnn-jni ncnn ${OpenCV_LIBS} camera2ndk mediandk)
