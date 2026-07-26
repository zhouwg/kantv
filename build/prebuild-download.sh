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
echo "HEXAGON_SDK_PATH: ${HEXAGON_SDK_PATH}"

# Hexagon SDK version must align with build/envsetup.sh
HEXAGON_SDK_VERSION=6.6.0.0
HEXAGON_TOOLS_VERSION=19.0.07

function check_command_in_host()
{
    set +e
    cmd=$1
    if command -v ${cmd} > /dev/null 2>&1; then
        printf "${cmd} is available on host machine\n"
    else
        printf "${TEXT_RED}${cmd} not exist on host machine, pls install command line utility ${cmd} firstly${TEXT_RESET}\n"
        exit 1
    fi
    set -e
}


function check_commands_in_host()
{
    check_command_in_host wget
    check_command_in_host xzcat
    check_command_in_host unzip
    check_command_in_host cmake
    check_command_in_host ninja
}


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


# Download and install the official Qualcomm Hexagon SDK from snapdragon-toolchain GitHub releases.
# This is a full SDK (not a minimal subset), so it includes the Hexagon LLVM toolchain,
# FastRPC stubs, and all headers needed to build ggml-hexagon.
function check_and_download_hexagon_sdk()
{
    set -e
    is_hexagon_llvm_exist=1
    if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/${HEXAGON_SDK_VERSION}/tools/HEXAGON_Tools/${HEXAGON_TOOLS_VERSION}/NOTICE.txt ]; then
        echo -e "${TEXT_RED}Hexagon SDK not exist...${TEXT_RESET}\n"
        is_hexagon_llvm_exist=0
    fi

    if [ ${is_hexagon_llvm_exist} -eq 0 ]; then
        mkdir -p ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/

        local sdk_tarball=hexagon-sdk-v${HEXAGON_SDK_VERSION}-amd64-lnx.tar.xz
        local sdk_url=https://github.com/snapdragon-toolchain/hexagon-sdk/releases/download/v${HEXAGON_SDK_VERSION}/${sdk_tarball}

        if [ -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/${sdk_tarball} ]; then
            echo -e "${sdk_tarball} already exist\n"
        else
            echo -e "begin downloading ${sdk_tarball} \n"
            wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/${sdk_tarball} ${sdk_url}
            if [ $? -ne 0 ]; then
                printf "failed to download ${sdk_tarball}\n"
                exit 1
            fi
        fi

        echo -e "begin decompressing ${sdk_tarball} \n"
        xzcat ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/${sdk_tarball} | tar -C ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/ -xf -
        if [ $? -ne 0 ]; then
            printf "failed to decompress ${sdk_tarball}\n"
            exit 1
        fi
        printf "install Hexagon SDK successfully\n\n"
    else
        printf "Qualcomm Hexagon SDK already exist\n\n"
    fi

    if [ ! -d ${HEXAGON_SDK_PATH} ]; then
        echo -e "${TEXT_RED}HEXAGON_SDK_PATH ${HEXAGON_SDK_PATH} not exist after installation, pls check...${TEXT_RESET}\n"
        exit 1
    fi
}


check_commands_in_host
check_and_download_androidndk
check_and_download_androidsdk
# Set SKIP_HEXAGON_SDK=1 (e.g. for non-qcom builds) to skip downloading them.
if [ "${SKIP_HEXAGON_SDK}" != "1" ]; then
    check_and_download_hexagon_sdk
fi
