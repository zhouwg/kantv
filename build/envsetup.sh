#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2021-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: configure project's compilation environment
#
#


export TEXT_BLACK=" \033[30m "
export TEXT_RED="   \033[31m "
export TEXT_GREEN=" \033[32m "
export TEXT_BLUE="  \033[34m "
export TEXT_BLACK=" \033[35m "
export TEXT_WHITE=" \033[37m "
export TEXT_RESET=" \033[0m  "

export BUILD_USER=$(whoami)
export BUILD_TIME=`date +"%Y-%m-%d-%H-%M-%S"`

export BUILD_HOST=Linux

#export BUILD_TARGET=ios
#export BUILD_TARGET=wasm
export BUILD_TARGET=android

export PROJECT_NAME=KanTV
export PROJECT_BUILD_TYPE=debug
export PROJECT_BUILD_TYPE=release
#export BUILD_ARCHS="arm64-v8a armeabi-v7a"
export BUILD_ARCHS="arm64-v8a"

export HOME_PATH=$(env | grep ^HOME= | cut -c 6-)
export PROJECT_HOME_PATH=`pwd`
export PROJECT_BRANCH=`git branch | grep "*" | cut -f 2 -d ' ' `
export PROJECT_ROOT_PATH=${PROJECT_HOME_PATH}
export PROJECT_OUT_PATH=${PROJECT_ROOT_PATH}/out


export LOCAL_WHISPERCPP_PATH=${PROJECT_ROOT_PATH}/external/whispercpp

#modify following lines to adapt to local dev envs
#export KANTV_TOOLCHAIN_PATH=${PROJECT_ROOT_PATH}/toolchain
export KANTV_TOOLCHAIN_PATH=/opt/kantv-toolchain
#API21：Android 5.0 (Android L)Lollipop
#API22：Android 5.1 (Android L)Lollipop
#API23：Android 6.0 (Android M)Marshmallow
#API24: Android 7.0 (Android N)Nougat
#API25: Android 7.1 (Android N)Nougat
#API26：Android 8.0 (Android O)Oreo
#API27：Android 8.1 (Android O)Oreo
#API28：Android 9.0 (Android P)Pie
#API29：Android 10
#API30：Android 11
#API31：Android 12
#API31：Android 12L
#API33：Android 13
#API34：Android 14
export ANDROID_PLATFORM=android-34
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r18b
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r21e
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r24
export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r26c
export LOCAL_BAZEL_PATH=${HOME_PATH}/.cache/bazel/_bazel_${BUILD_USER}/d483cd2a2d9204cb5bb4d870c2729238
export UPSTREAM_WHISPERCPP_PATH=~/cdeos/whisper.cpp




. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)
if [ $? != 0 ]; then
    echo -e "pls running source build/envsetup.sh firstly\n"
    #exit 1
    return 0
fi


setup_env
check_host
check_ubuntu

if [ "${BUILD_TARGET}" == "android" ]; then
    check_ndk
fi

dump_global_envs
