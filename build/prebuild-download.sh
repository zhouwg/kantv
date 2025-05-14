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

#QNN SDK can be found at:
#https://www.qualcomm.com/developer/software/qualcomm-ai-engine-direct-sdk
QNN_SDK_URL=https://www.qualcomm.com/developer/software/qualcomm-ai-engine-direct-sdk
QNN_SDK_INSTALL_PATH=/opt/qcom/aistack/qairt
QNN_SDK_VERSION=2.32.0.250228
QNN_SDK_VERSION=2.33.0.250327
QNN_SDK_VERSION=2.34.0.250424
QNN_SDK_PATH=${QNN_SDK_INSTALL_PATH}/${QNN_SDK_VERSION}


function check_and_download_qnn_sdk()
{
    is_qnn_sdk_exist=1

    if [ ! -d ${QNN_SDK_PATH} ]; then
        echo -e "QNN_SDK_PATH ${QNN_SDK_PATH} not exist, download it from ${QNN_SDK_URL}...\n"
        is_qnn_sdk_exist=0
    fi

    if [ ! -f ${QNN_SDK_PATH}/sdk.yaml ]; then
        is_qnn_sdk_exist=0
    fi

    if [ ${is_qnn_sdk_exist} -eq 0 ]; then
        echo "sudo mkdir -p ${QNN_SDK_INSTALL_PATH}"
        sudo mkdir -p ${QNN_SDK_INSTALL_PATH}
        if [ ! -f v${QNN_SDK_VERSION}.zip ]; then
            wget --no-config --quiet --show-progress -O v${QNN_SDK_VERSION}.zip https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/${QNN_SDK_VERSION}/v${QNN_SDK_VERSION}.zip
        fi
        unzip v${QNN_SDK_VERSION}.zip
        if [ $? -ne 0 ]; then
            printf "failed to download Qualcomm QNN SDK to %s \n" "${QNN_SDK_PATH}"
            exit 1
        fi
        sudo mv qairt/${QNN_SDK_VERSION} ${QNN_SDK_INSTALL_PATH}/
        printf "Qualcomm QNN SDK saved to ${QNN_SDK_PATH} \n\n"
        sudo rm -rf qairt
        sudo mv v${QNN_SDK_VERSION}.zip /tmp/
    else
        printf "Qualcomm QNN SDK already exist:${QNN_SDK_PATH} \n\n"
    fi
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

        wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-ndk-r26c-linux.zip  https://dl.google.com/android/repository/android-ndk-r26c-linux.zip
        if [ $? -ne 0 ]; then
            printf "failed to download android ndk to %s \n" "${ANDROID_NDK}"
            exit 1
        fi

        cd ${PROJECT_ROOT_PATH}/prebuilts/toolchain

        unzip android-ndk-r26c-linux.zip

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


function check_and_download_hexagon_llvm_toolchain()
{
    is_hexagon_llvm_exist=1
    if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/8.8.06/NOTICE.txt ]; then
        echo -e "${TEXT_RED}hexagon LLVM toolchain not exist, pls check...${TEXT_RESET}\n"
        is_hexagon_llvm_exist=0
    else
        printf "hexagon LLVM toolchain already exist\n\n"
    fi

    #download customized LLVM toolchain HEXAGON_TOOLs_8.8.06.tar.gz
    if [ ${is_hexagon_llvm_exist} -eq 0 ]; then
        echo -e "begin downloading hexagon LLVM toolchain \n"
        wget --no-config --quiet --show-progress -O ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/HEXAGON_TOOLs_8.8.06.tar.gz https://github.com/kantv-ai/toolchain/raw/refs/heads/main/HEXAGON_TOOLs_8.8.06.tar.gz
        if [ $? -ne 0 ]; then
            printf "failed to download hexagon LLVM toolchain\n"
            exit 1
        else
            zcat ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/HEXAGON_TOOLs_8.8.06.tar.gz | tar -C ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools -xvf -
            printf "install hexagon LLVM toolchain successfully\n\n"
        fi
    fi
}



check_and_download_qnn_sdk
check_and_download_androidndk
check_and_download_androidsdk
check_and_download_hexagon_llvm_toolchain
