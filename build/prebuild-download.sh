#!/usr/bin/env bash

# Copyright (c) 2021- KanTV Authors

# Description: download Android NDK for build project with command-line mode

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
is_cmdlinetools_exist=1

if [ ! -d ${ANDROID_NDK} ]; then
    echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
    is_android_ndk_exist=0
fi

if [ ! -f ${ANDROID_NDK}/build/cmake/android.toolchain.cmake ]; then
    echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
    is_android_ndk_exist=0
fi

if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools/bin/sdkmanager ]; then
    echo -e "${TEXT_RED}Android SDK cmdline-tools not exist, pls check...${TEXT_RESET}\n"
    is_cmdlinetools_exist=0
fi

if [ ${is_android_ndk_exist} -eq 0 ]; then
    echo -e "begin downloading android ndk \n"

    if [ ! -d ${PROJECT_ROOT_PATH}/prebuilts/toolchain ]; then
        mkdir -p ${PROJECT_ROOT_PATH}/prebuilts/toolchain
    fi

    wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-ndk-r26c-linux.zip  https://dl.google.com/android/repository/android-ndk-r26c-linux.zip

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



if [ ${is_cmdlinetools_exist} -eq 0 ]; then
    echo -e "begin downloading android sdk cmdline-tools \n"

    if [ ! -d ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools/ ]; then
        mkdir -p ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools/
    fi

    wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools/commandlinetools-linux-9862592_latest.zip  https://dl.google.com/android/repository/commandlinetools-linux-9862592_latest.zip

    if [ $? -ne 0 ]; then
        printf "failed to download android sdk cmdline-tools\n"
        exit 1
    fi

    cd ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools
    unzip commandlinetools-linux-9862592_latest.zip
    mv cmdline-tools latest

    cd ${PROJECT_ROOT_PATH}
else
    printf "android sdk cmdline-tools already exist \n\n"
fi


#prepare for cmdline build

export ANDROID_HOME=${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/
export PATH=${ANDROID_HOME}/cmdline-tools/latest/bin:${PATH}

# check sdkmanager works
sdkmanager --version
if [ $? -ne 0 ]; then
    printf "android cmdline-tools could not work properly, pls check development envs\n"
    exit 1
fi

yes | sdkmanager --licenses
yes | sdkmanager --install "platforms;android-34"
yes | sdkmanager --install "build-tools;34.0.0"
yes | sdkmanager --install "cmake;3.22.1"

