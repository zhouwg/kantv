#!/usr/bin/env bash

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

#Description: configure project's compilation environment
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
export BUILD_TARGET=android

export PROJECT_NAME=KanTV
export PROJECT_BUILD_TYPE=debug
export PROJECT_BUILD_TYPE=release
export BUILD_ARCHS="arm64-v8a armeabi-v7a"
export BUILD_ARCHS="arm64-v8a"

export HOME_PATH=$(env | grep ^HOME= | cut -c 6-)
export PROJECT_HOME_PATH=`pwd`
export PROJECT_BRANCH=`git branch | grep "*" | cut -f 2 -d ' ' `
export PROJECT_ROOT_PATH=${PROJECT_HOME_PATH}


export LOCAL_WHISPERCPP_PATH=${PROJECT_ROOT_PATH}/external/whispercpp

#modify following lines to adapt to local dev envs
export ANDROID_NDK=/opt/kantv-toolchains/android-ndk-r21e
export LOCAL_BAZEL_PATH=${HOME_PATH}/.cache/bazel/_bazel_${BUILD_USER}/d483cd2a2d9204cb5bb4d870c2729238
export UPSTREAM_WHISPERCPP_PATH=~/cdeos/whisper.cpp




. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)
if [ $? != 0 ]; then
    echo -e "pls running source build/envsetup.sh firstly\n"
    #exit 1
    return 0
fi


check_host
check_ubuntu
check_ndk
dump_global_envs
