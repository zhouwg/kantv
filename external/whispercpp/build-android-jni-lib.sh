#!/usr/bin/env bash

# Copyright (c) 2024, zhou.weiguo(zhouwg2000@gmail.com)

# Copyright (c) 2024- KanTV Authors

# Description: build libwhispercpp.so for target Android
#

set -e

TARGET=whispercpp
BUILD_TYPE=Release

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd
check_upstream_whispercpp
check_local_whispercpp

if [ "${PROJECT_BUILD_TYPE}" == "release" ]; then
    BUILD_TYPE=Release
fi

if [ "${PROJECT_BUILD_TYPE}" == "debug" ]; then
    BUILD_TYPE=Debug
fi

echo -e  "upstream whispercpp path: ${UPSTREAM_WHISPERCPP_PATH}\n"
echo -e  "local    whispercpp path: ${LOCAL_WHISPERCPP_PATH}\n"
echo -e  "build               type: ${BUILD_TYPE}"

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi


function build_arm64
{
cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/arm64-v8a
make

ls -l lib${TARGET}.so
ls -lah lib${TARGET}.so
#don't do this because IDE is preferred to this open source project for purpose of more easily/friendly
#/bin/cp -fv lib${TARGET}.so ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/src/main/jniLibs/arm64-v8a/
cd -
}


function build_armv7a
{
cmake -H. -B./out/armeabi-v7a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/armeabi-v7a
make

ls -l lib${TARGET}.so
ls -lah lib${TARGET}.so
#don't do this because IDE is preferred to this open source project for purpose of more easily/friendly
#/bin/cp -fv lib${TARGET}.so ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/src/main/jniLibs/armeabi-v7a/
cd -
}


build_arm64
#build_armv7a
