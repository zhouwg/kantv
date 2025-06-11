#!/bin/bash
# check dev envs for Project KanTV.
#
# setup dev envs for Project KanTV can be found at ${PROJECT_ROOT_PATH}/docs/how-to-build.md
# all dependent SDKs/Toolchains can be downloaded automatically via ${PROJECT_ROOT_PATH}/build/prebuild-download.sh

set -e

PWD=`pwd`

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

#running path on Android phone
REMOTE_PATH=/data/local/tmp/

#Android NDK can be found at:
#https://developer.android.com/ndk/downloads
ANDROID_PLATFORM=android-34
ANDROID_NDK_VERSION=r28
ANDROID_NDK_NAME=android-ndk-${ANDROID_NDK_VERSION}
ANDROID_NDK_FULLNAME=${ANDROID_NDK_NAME}-linux.zip
ANDROID_NDK=${PROJECT_ROOT_PATH}/prebuilts/toolchain/${ANDROID_NDK_NAME}

#QNN SDK can be found at:
#https://www.qualcomm.com/developer/software/qualcomm-ai-engine-direct-sdk
QNN_SDK_URL=https://www.qualcomm.com/developer/software/qualcomm-ai-engine-direct-sdk
QNN_SDK_INSTALL_PATH=/opt/qcom/aistack/qairt
QNN_SDK_VERSION=2.32.0.250228
QNN_SDK_VERSION=2.33.0.250327
QNN_SDK_VERSION=2.34.0.250424
QNN_SDK_PATH=${QNN_SDK_INSTALL_PATH}/${QNN_SDK_VERSION}

#Hexagon SDK can be found at:
#https://developer.qualcomm.com/software/hexagon-dsp-sdk/tools
#Hexagon SDK must be obtained with a Qualcomm Developer Account and cannot be downloaded automatically in this script.
HEXAGON_SDK_PATH=/opt/qcom/Hexagon_SDK/6.2.0.1

HEXAGON_LLVM_TOOLCHAIN_PATH=${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/8.8.06

#available htp arch version:
#v68 --- Snapdragon 888
#v69 --- Snapdragon 8 Gen1
#v73 --- Snapdragon 8 Gen2
#v75 --- Snapdragon 8 Gen3
#v79 --- Snapdragon 8 Elite(aka Gen4)
#8Gen3
HTP_ARCH_VERSION=v75
HTP_ARCH_VERSION_a=V75
#8Elite
HTP_ARCH_VERSION=v79
HTP_ARCH_VERSION_a=V79

#running_params=" -mg 2 -ngl 99 -t 8 -fa 1 "
running_params=" -mg 2 -ngl 99 -t 8 "

function dump_vars()
{
    echo -e "ANDROID_NDK:                   ${ANDROID_NDK}"
    echo -e "QNN_SDK_PATH:                  ${QNN_SDK_PATH}"
    echo -e "HEXAGON_SDK_PATH:              ${HEXAGON_SDK_PATH}"
    echo -e "HEXAGON_LLVM_TOOLCHAIN_PATH:   ${HEXAGON_LLVM_TOOLCHAIN_PATH}"
}


function show_pwd()
{
    echo -e "current working path:$(pwd)\n"
}


function check_hexagon_sdk()
{
    echo "check Hexagon SDK"
    if [ ! -d ${HEXAGON_SDK_PATH} ]; then
        echo -e "HEXAGON_SDK_PATH ${HEXAGON_SDK_PATH} not exist\n"
    else
        printf "Qualcomm Hexagon SDK already exist:${HEXAGON_SDK_PATH} \n\n"
    fi
}


function check_qnn_sdk()
{
    echo "check QNN SDK"
    if [ ! -d ${QNN_SDK_PATH} ]; then
        echo -e "QNN_SDK_PATH ${QNN_SDK_PATH} not exist\n"
    else
        echo -e "QNN_SDK_PATH ${QNN_SDK_PATH} already exist\n"
    fi
}


function check_hexagon_llvm_toolchain()
{
    echo "check hexagon LLVM toolchain"
    if [ ! -f ${PROJECT_ROOT_PATH}/prebuilts/Hexagon_SDK/6.2.0.1/tools/HEXAGON_Tools/8.8.06/NOTICE.txt ]; then
        echo -e "${TEXT_RED}hexagon LLVM toolchain not exist, pls download it via ${PROJECT_ROOT_PATH}/build/prebuild-download.sh${TEXT_RESET}\n"
    else
        echo -e "hexagon LLVM toolchain already exist\n"
    fi
}


function update_qnn_libs()
{
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnSystem.so              ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnCpu.so                 ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnGpu.so                 ${REMOTE_PATH}/

    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnHtp.so                 ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnHtpNetRunExtensions.so ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnHtpPrepare.so          ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/aarch64-android/libQnnHtp${HTP_ARCH_VERSION_a}Stub.so          ${REMOTE_PATH}/
    adb push ${QNN_SDK_PATH}/lib/hexagon-${HTP_ARCH_VERSION}/unsigned/libQnnHtp${HTP_ARCH_VERSION_a}Skel.so     ${REMOTE_PATH}/
}


function show_usage()
{
    echo "Usage:"
    echo "  $0 help"
    echo "  $0 updateqnnlib"

    echo -e "\n\n\n"
}



show_pwd
check_qnn_sdk
check_hexagon_sdk
check_hexagon_llvm_toolchain
dump_vars

if [ $# == 0 ]; then
    show_usage
    exit 1
elif [ $# == 1 ]; then
    if [ "$1" == "-h" ]; then
        show_usage
        exit 1
    elif [ "$1" == "help" ]; then
        show_usage
        exit 1
    elif [ "$1" == "updateqnnlib" ]; then
        update_qnn_libs
        exit 0
    else
        show_usage
        exit 1
    fi
else
    show_usage
    exit 1
fi
