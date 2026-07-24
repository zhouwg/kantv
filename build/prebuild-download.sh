#!/usr/bin/env bash

# Description: download Android SDK and Android NDK and a fully official QNN SDK and
# a customized Hexagon LLVM toolchain for build the project KanTV in command-line mode.
# a fully official Hexagon SDK must be obtained with a Qualcomm Developer Account and
# cannot be downloaded automatically in this script.

# verified on Ubuntu 20.04, Ubuntu 24.04

set -e

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd

echo "ANDROID_NDK: ${ANDROID_NDK}"

function check_and_download_androidndk()
{
    is_android_ndk_exist=1
    if [ ! -d ${ANDROID_NDK} ]; then
        echo -e "${TEXT_RED}Android NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        is_android_ndk_exist=0
    fi

    if [ ! -f ${ANDROID_NDK}/build/cmake/android.toolchain.cmake ]; then
        echo -e "${TEXT_RED}Android NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        is_android_ndk_exist=0
    fi

    if [ ${is_android_ndk_exist} -eq 0 ]; then
        echo -e "begin downloading android ndk \n"

        if [ ! -d ${PROJECT_ROOT_PATH}/prebuilts/toolchain ]; then
            mkdir -p ${PROJECT_ROOT_PATH}/prebuilts/toolchain
        fi

        wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-ndk-r28-linux.zip  https://dl.google.com/android/repository/android-ndk-r28-linux.zip
        if [ $? -ne 0 ]; then
            printf "failed to download android ndk to %s \n" "${ANDROID_NDK}"
            exit 1
        fi

        cd ${PROJECT_ROOT_PATH}/prebuilts/toolchain

        unzip android-ndk-r28-linux.zip

        cd ${PROJECT_ROOT_PATH}

        printf "android ndk saved to ${ANDROID_NDK} \n\n"
else
        printf "android ndk already exist:${ANDROID_NDK} \n\n"
fi
}


function check_and_download_androidsdk()
{
    is_android_sdk_exist=1

    if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/cmdline-tools/latest/bin/sdkmanager ]; then
        echo -e "${TEXT_RED}Android SDK not exist, pls check...${TEXT_RESET}\n"
        is_android_sdk_exist=0
    fi

    if [ ${is_android_sdk_exist} -eq 0 ]; then
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

    if [ ${is_android_sdk_exist} -eq 0 ]; then
        #prepare for cmdline build
        export ANDROID_HOME=${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk/
        export PATH=${ANDROID_HOME}/cmdline-tools/latest/bin:${PATH}

        # check Java works
        java --version
        if [ $? -ne 0 ]; then
            echo -e "${TEXT_RED}Java not exist, pls install it via ${PROJECT_ROOT_PATH}/build/prebuild.sh${TEXT_RESET}\n"
            exit 1
        fi

        # check sdkmanager works
        sdkmanager --version
        if [ $? -ne 0 ]; then
            printf "android cmdline-tools could not work properly, pls check development envs\n"
            exit 1
        fi
        echo -e "begin downloading android sdk components \n"
        yes | sdkmanager --licenses
        yes | sdkmanager --install "platforms;android-34"
        yes | sdkmanager --install "build-tools;34.0.0"
        yes | sdkmanager --install "cmake;3.22.1"
    else
        printf "android sdk already exist \n\n"
    fi
}


function check_and_download_hexagon_sdk()
{
    set -e
    is_hexagon_llvm_exist=1
    if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/8.8.06/NOTICE.txt ]; then
        echo -e "${TEXT_RED}minimal-hexagon-sdk not exist...${TEXT_RESET}\n"
        is_hexagon_llvm_exist=0
    fi

    if [ ${is_hexagon_llvm_exist} -eq 0 ]; then
        if [ -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/minimal-hexagon-sdk-6.2.0.1.xz ]; then
            echo -e "minimal-hexagon-sdk-6.2.0.1.xz already exist\n"
        else
            echo -e "begin downloading minimal-hexagon-sdk-6.2.0.1.xz \n"
            wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/minimal-hexagon-sdk-6.2.0.1.xz https://github.com/zhouwg/toolchain/raw/refs/heads/master/minimal-hexagon-sdk-6.2.0.1.xz
            if [ $? -ne 0 ]; then
                printf "failed to download minimal-hexagon-sdk-6.2.0.1.xz\n"
                exit 1
            fi
        fi

        echo -e "begin decompressing minimal-hexagon-sdk-6.2.0.1.xz \n"
        xzcat ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/minimal-hexagon-sdk-6.2.0.1.xz | tar -C ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/ -xf -
        if [ $? -ne 0 ]; then
            printf "failed to decompress minimal-hexagon-sdk-6.2.0.1.xz\n"
            exit 1
        fi
        printf "install minimal-hexagon-sdk successfully\n\n"
    else
        printf "Qualcomm Hexagon SDK already exist\n\n"
    fi
}



check_and_download_androidndk
check_and_download_androidsdk
check_and_download_hexagon_sdk
