#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: download Android NDK for build project

set -e

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd

echo "ANDROID_NDK: ${ANDROID_NDK}"

is_android_ndk_exist=1

if [ ! -d ${ANDROID_NDK} ]; then
    echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
    is_android_ndk_exist=0
fi

if [ ! -f ${ANDROID_NDK}/build/cmake/android.toolchain.cmake ]; then
    echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
    is_android_ndk_exist=0
fi

if [ ${is_android_ndk_exist} -eq 0 ]; then
    echo -e "begin downloading android ndk \n"
    wget --no-config --quiet --show-progress -O ${ANDROID_NDK}-linux.zip  https://dl.google.com/android/repository/android-ndk-r26c-linux.zip
    cd ${PROJECT_ROOT_PATH}/prebuilts/toolchain
    unzip android-ndk-r26c-linux.zip
    cd ${PROJECT_ROOT_PATH}

    if [ $? -ne 0 ]; then
        printf "failed to download android ndk to %s \n" "${ANDROID_NDK}"
        exit 1
    fi

    printf "android ndk saved to ${ANDROID_NDK} \n\n"
else
    printf "android ndk already exist:${ANDROID_NDK} \n\n"
fi
