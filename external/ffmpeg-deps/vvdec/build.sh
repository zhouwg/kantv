#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2015-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.



set -e

TARGET=libvvdec.a
LIB_TYPE=STATIC
LIB_BUILD_TYPE=Release

if [ "${PROJECT_BUILD_TYPE}" == "release" ]; then
    LIB_BUILD_TYPE=Release
fi

if [ "${PROJECT_BUILD_TYPE}" == "debug" ]; then
    LIB_BUILD_TYPE=Debug
fi

#workaroud for vvdec
LIB_BUILD_TYPE=Debug

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi


if [ "x${FF_PREFIX}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "FF_PREFIX is empty, pls check"
    exit 1
fi


if [ "${BUILD_TARGET}" == "android" ]; then
    echo "build ${TARGET} for target ${BUILD_TARGET} with build type ${LIB_BUILD_TYPE}"

    if [ "x${ANDROID_NDK}" == "x" ]; then
        echo "pwd is `pwd`"
        echo "pls run . build/envsetup in project's toplevel directory firstly"
        exit 1
    fi
fi

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi

function build_arm64
{
cmake -H. -B./out/arm64-v8a -DPRJ_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${LIB_BUILD_TYPE} -DBUILD_TARGET=${BUILD_TARGET} -DBUILD_LIB_TYPE=${LIB_TYPE} -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=${ANDROID_PLATFORM} -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake -DPREFIX=${FF_PREFIX} -DCMAKE_INSTALL_PREFIX=${FF_PREFIX} -DCMAKE_VERBOSE_MAKEFILE=ON
cd ./out/arm64-v8a
make -j${HOST_CPU_COUNTS}
make install

cd -
}


function build_x86
{
cmake -H. -B./out/x86 -DPRJ_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${LIB_BUILD_TYPE} -DBULD_LIB_TYPE=${LIB_TYPE} -DPREFIX=${FF_PREFIX} -DCMAKE_INSTALL_PREFIX=${FF_PREFIX} -DCMAKE_VERBOSE_MAKEFILE=ON -DVVDEC_INSTALL_VVDECAPP=ON
cd ./out/x86
make -j${HOST_CPU_COUNTS}
make install
cd -
}


function build_android
{
    if [ $# == 0 ]; then
        build_x86
    else
        arch=$1
        echo "ARCH : $arch"
        if [ "${arch}"   == "arm64" ]; then
            build_arm64
        elif [ "${arch}" == "x86" ]; then
            build_x86
        elif [ "${arch}" == "clean" ]; then
            if [ -d out ]; then
                echo "remove out directory in `pwd`"
                rm -rf out
            fi
        else
            echo -e "${TEXT_RED}${arch} unknown, only support arm64 x86, pls check...${TEXT_RESET}"
        fi
    fi
}


function build_ios
{
    echo "ios not support currently"
}


function build_wasm
{
    echo "wasm not support currently"
}



if [ "x${BUILD_TARGET}" == "xios" ]; then
    build_ios $1
elif [ "x${BUILD_TARGET}" == "xwasm" ]; then
    build_wasm $1
else
    build_android $1
fi
