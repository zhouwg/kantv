#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: build libncnn.a for project KanTV
#
# TIP:RTTI and EXCEPTION should be enabled in CMakeLists.txt of ncnn source codes
#     otherwise some NCNN inference cases would be failure or even crash
#    option(NCNN_DISABLE_RTTI "disable rtti" OFF)
#    option(NCNN_DISABLE_EXCEPTION "disable exception" OFF)

set -e

TARGET=ncnn
BUILD_TYPE=Debug
#default is release build
BUILD_TYPE=Release

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd


echo -e  "build               type: ${BUILD_TYPE}"

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi


function build_arm64
{
#enable NCNN_VULKAN
#cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH} -DNCNN_VULKAN=ON

#disable NCNN_VULKAN
cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/arm64-v8a
make -j${HOST_CPU_COUNTS}
make install

show_pwd
ls -lah src/libncnn.a

cd -
show_pwd
#generate prebuild header files of ncnn for ncnn-jni codes because some special methods was used in the implementation of internal ncnn
/bin/cp -rf ./out/arm64-v8a/install/include/ncnn/* ./include/
}


function build_armv7a
{
cmake -H. -B./out/armeabi-v7a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/armeabi-v7a
make -j${HOST_CPU_COUNTS}

cd -
}


build_arm64
#build_armv7a
