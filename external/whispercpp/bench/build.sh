#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: build whispercpp's bench tool for target Android(this project only focus on Android-based device)
#

set -e

TARGET=bench
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
cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DWHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/arm64-v8a
make

ls -l ${TARGET}
ls -lah ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}_arm64
cd -
echo -e `pwd`
ls -l ${TARGET}_arm64
}


function build_armv7a
{
cmake -H. -B./out/armeabi-v7a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-21 -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DWHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/armeabi-v7a
make

ls -l ${TARGET}
ls -lah ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}_armv7a
cd -
ls -l ${TARGET}_armv7a
}


function build_x86
{
cmake -H. -B./out/x86 -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="x86" -DWHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/x86
make

ls -l ${TARGET}
ls -lah ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}
cd -
}

build_arm64
#build_armv7a
#build_x86
