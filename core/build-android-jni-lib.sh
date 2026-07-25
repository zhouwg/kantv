#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: build jni for target Android
#

set -e

TARGET=kantvai-jni
BUILD_TYPE=Release

# Hexagon SDK is provided via prebuilts/Hexagon_SDK symlink (from ggml-hexagon project)
# QNN SDK is no longer needed — JZ's ggml-hexagon uses Hexagon SDK FastRPC directly
HEXAGON_SDK_PATH=${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.6.0.0
HEXAGON_TOOLS_PATH=${HEXAGON_SDK_PATH}/tools/HEXAGON_Tools/19.0.07
#override if your Hexagon Tools installed elsewhere:
#HEXAGON_TOOLS_PATH=/opt/qcom/Hexagon_SDK/6.6.0.0/tools/HEXAGON_Tools/19.0.07
#available htp arch version:
#v73 --- Snapdragon 8 Gen2
#v75 --- Snapdragon 8 Gen3
#v79 --- Snapdragon 8 Elite
#v81 --- Snapdragon 8 Elite Gen5 (JZ only)
HTP_ARCH_VERSION=v79

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd

if [ "${PROJECT_BUILD_TYPE}" == "release" ]; then
    BUILD_TYPE=Release
fi

if [ "${PROJECT_BUILD_TYPE}" == "debug" ]; then
    BUILD_TYPE=Debug
fi


echo -e  "build               type: ${BUILD_TYPE}"

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi


function build_arm64
{
cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH} -DGGML_OPENMP=OFF -DCMAKE_C_FLAGS=-march=armv8.7-a -DGGML_HEXAGON=ON -DGGML_HEXAGON_JZ=ON -DLLAMA_CURL=OFF -DHEXAGON_SDK_PATH=${HEXAGON_SDK_PATH} -DHTP_ARCH_VERSION=${HTP_ARCH_VERSION} -DHEXAGON_TOOLS_PATH=${HEXAGON_TOOLS_PATH}
cd ./out/arm64-v8a
make -j${HOST_CPU_COUNTS}

cd -
}


function build_armv7a
{
cmake -H. -B./out/armeabi-v7a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/armeabi-v7a
make -j${HOST_CPU_COUNTS}

cd -
}

function build_arm64_non_qcom
{
cmake -H. -B./out/arm64-v8a -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DLOCAL_WHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH} -DGGML_OPENMP=OFF -DCMAKE_C_FLAGS=-march=armv8.7-a -DGGML_HEXAGON=OFF -DLLAMA_CURL=OFF -DHEXAGON_SDK_PATH=${HEXAGON_SDK_PATH} -DHTP_ARCH_VERSION=${HTP_ARCH_VERSION}
cd ./out/arm64-v8a
make -j${HOST_CPU_COUNTS}

cd -
}


function show_usage()
{
    echo "Usage:"
    echo "  $0 qcom"
    echo "  $0 non_qcom"
    echo "  $0 help"
    echo -e "\n\n\n"
}

if [ $# == 0 ]; then
    #default is build for qcom
    build_arm64
elif [ $# == 1 ]; then
    if [ "$1" == "help" ]; then
        show_usage
        exit 1
    elif [ "$1" == "qcom" ]; then
        echo "build arm64_qcom"
        build_arm64
    else
        echo "build arm64_non_qcom"
        build_arm64_non_qcom
    fi
else
    show_usage
    exit 1
fi
