#!/usr/bin/env bash

# Copyright (c) 2021- KanTV Authors

# Description: public script functions for entire project
#
#

function check_global_envs()
{
    if [ -z ${PROJECT_ROOT_PATH} ]; then
        echo -e "${TEXT_RED}${ANDROID_NDK} pls running source build/envsetup.sh firstly${TEXT_RESET}\n"
        #exit 1
    fi
}


function show_pwd()
{
    echo -e "current working path:$(pwd)\n"
}


function setup_env
{
if [ "${BUILD_TARGET}" == "android" ]; then
    export PROJECT_NAME=KanTV-android
    #export BUILD_ARCHS="arm64-v8a armeabi-v7a"
    export BUILD_ARCHS="arm64-v8a"
    export FF_PREFIX=${PROJECT_OUT_PATH}/${BUILD_TARGET}/
    export PROJECT_BUILD_COMMAND="./build-all.sh android"
    export ANDROID_APK_VERSION=$(awk -F"'" '/releaseVersion[ ]*=/ {print $2}' "${PROJECT_ROOT_PATH}/android/constants.gradle")
fi

if [ "${BUILD_TARGET}" == "linux" ]; then
    export PROJECT_NAME=KanTV-linux
    export BUILD_ARCHS="x86"
    export FF_PREFIX=${PROJECT_OUT_PATH}/${BUILD_TARGET}/
    export PROJECT_BUILD_COMMAND="./build-all.sh linux"
fi

if [ "${BUILD_TARGET}" == "wasm" ]; then
    export PROJECT_NAME=KanTV-wasm
    export BUILD_ARCHS="wasm"
    export FF_PREFIX=${PROJECT_OUT_PATH}/${BUILD_TARGET}/
    export PROJECT_BUILD_COMMAND="./build-all.sh wasm"

    export EMSDK=${KANTV_TOOLCHAIN_PATH}/emsdk
    export NODEJS_ROOT_PATH=${KANTV_TOOLCHAIN_PATH}/nodejs
    export EM_CONFIG=${EMSDK}/.emscripten
    export EMSDK_CMAKE_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
    if [ "${PROJECT_CACHE_PATH}" = "" ]; then
        export PROJECT_CACHE_PATH=${EMSDK}:${EMSDK}/upstream/emscripten:${EMSDK}/node/14.15.5_64bit/bin:${EMSDK}/upstream/bin:${NODEJS_ROOT_PATH}/bin:$PATH
    fi
    export PATH=${PROJECT_CACHE_PATH}
    mkdir -p ${PROJECT_OUT_PATH}/wasm
fi


if [ "${BUILD_TARGET}" == "ios" ]; then
    export PROJECT_NAME=KanTV-ios
    export FF_PREFIX=${PROJECT_OUT_PATH}/${BUILD_TARGET}/
    export PROJECT_BUILD_COMMAND="./build-all.sh ios"

    export XCODE_SELECT=/usr/bin/xcode-select
    #export BUILD_ARCHS="armv7 arm64 i386 x86_64"
    export BUILD_ARCHS="arm64"
fi
}


function check_host()
{
    uname_result=`uname -s`
    case ${uname_result} in
        Linux*)  machine="Linux";;
        Darwin*) machine="Mac";;
        *)       machine="Unknown:${uname_result}";;
    esac

    if [ $machine = "Linux" ]; then
        if [ "x${BUILD_TARGET}" == "x" ]; then
            #echo -e "${TEXT_BLUE}host os is ${machine}, reset BUILD_TARGET to android${TEXT_RESET}"
            export BUILD_TARGET=android
        else
            echo
            #echo -e "${TEXT_BLUE}BUILD_TARGET already setting to ${BUILD_TARGET}"
        fi
        export BUILD_HOST=Linux
        export HOST_CPU_COUNTS=`cat /proc/cpuinfo | grep "processor" | wc | awk '{print int($1)}'`
    elif [ $machine = "Mac" ]; then
        if [ "x${BUILD_TARGET}" == "x" ]; then
            #echo -e "${TEXT_BLUE}host os is ${machine}, reset BUILD_TARGET to ios${TEXT_RESET}"
            export BUILD_TARGET=ios
        else
            echo
            #echo -e "${TEXT_BLUE}BUILD_TARGET already setting to ${BUILD_TARGET}"
        fi
        export BUILD_HOST=Mac
        export XCODE_PATH=`xcode-select -p`
        export HOST_CPU_COUNTS=4
    else
        echo -e "${TEXT_RED}host os is ${machine}, not supported at the moment...${TEXT_RESET}\n"
        return 1
    fi
}


function check_ndk()
{
    if [ -z ${ANDROID_NDK} ]; then
        echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        #exit 1
        return 1;
    fi

    if [ ! -d ${ANDROID_NDK} ]; then
        echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        #exit 1
        return 1;
    fi

    if [ ! -f ${ANDROID_NDK}/build/cmake/android.toolchain.cmake ]; then
        echo -e "${TEXT_RED}NDK ${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        return 1;
    fi
}


function check_ubuntu()
{
    if [ ${BUILD_HOST} != "Linux" ]; then
        echo -e "${TEXT_RED}host os is ${BUILD_HOST}${TEXT_RESET}\n"
        return 1;
    fi

    if [ -f /etc/issue ]; then
        host_os=`cat /etc/issue | awk '{print $1 $2}'`
        if [ ${host_os} == "Ubuntu20.04.2" ]; then
            echo -e "${TEXT_BLUE}host os is Ubuntu 20.04.2${TEXT_RESET}\n"
        else
            echo -e "${TEXT_RED}host os is ${host_os}${TEXT_RESET}\n"
            echo -e "${TEXT_RED}this script validated ok on host Ubuntu 20.04 x86_64 LTS at the moment${TEXT_RESET}\n"
            #exit 1
            return 1
        fi
    else
        echo -e "${TEXT_RED}/etc/issue not exist, pls check...${TEXT_RESET}\n"
    fi
    return 0;
}


function check_emsdk()
{
    if [ $BUILD_TARGET == "wasm" ]; then
        if [ ! -d ${EMSDK} ]; then
            echo -e "${TEXT_RED}${EMSDK} not exist, pls check...${TEXT_RESET}"
            #exit 1
        else
            echo -e "${TEXT_RED}${EMSDK} exist${TEXT_RESET}"
        fi
    fi
}


function check_xcode()
{
    if [ ${BUILD_TARGET} = "ios" ]; then
        echo -e "${TEXT_BLUE}check xcode for target ios...${TEXT_RESET}\n"
        if [  -z ${XCODE_SELECT} ]; then
            echo -e "${TEXT_RED}${XCODE_SELECT} not exist, pls check...${TEXT_RESET}\n"
            #exit 1
        fi

        if [ ! -f ${XCODE_SELECT} ]; then
            echo -e "${TEXT_RED}${XCODE_SELECT} not exist, pls check...${TEXT_RESET}\n"
            #exit 1
        fi
    fi
}


function echo_left_align()
{
    if [ ! $# -eq 2 ]; then
        echo "params counts should be 2"
        exit 1
    fi

    index=0
    max_length=40
    l_string=$1
    r_string=$2

    length=${#l_string}
    echo -e "${TEXT_BLUE}$l_string${TEXT_RESET}\c"
    if [ $length -lt $max_length ]; then
        padding_length=`expr $max_length - $length`
        for (( tmp=0; tmp < $padding_length; tmp++ ))
            do
               echo -e " \c"
            done
    fi

    echo $r_string
}


function check_upstream_whispercpp()
{
if [  -z ${UPSTREAM_WHISPERCPP_PATH} ]; then
    echo "pls set UPSTREAM_WHISPERCPP_PATH properly"
    echo "or run . build/envsetup.sh firstly"
    exit 1
fi


if [ ! -d ${UPSTREAM_WHISPERCPP_PATH} ]; then
    echo "${UPSTREAM_WHISPERCPP_PATH} not exist, pls check"
    exit 1
fi
}


function check_local_whispercpp()
{
if [ "x${LOCAL_WHISPERCPP_PATH}" == "x" ]; then
    echo "pls check LOCAL_WHISPERCPP_PATH"
    exit 1
fi


if [ ! -d ${LOCAL_WHISPERCPP_PATH} ]; then
    echo "${LOCAL_WHISPERCPP_PATH} not exist, pls check"
    exit 1
fi
}


function check_upstream_llamacpp()
{
if [  -z ${UPSTREAM_LLAMACPP_PATH} ]; then
    echo "pls set UPSTREAM_LLAMACPP_PATH properly"
    echo "or run . build/envsetup.sh firstly"
    exit 1
fi


if [ ! -d ${UPSTREAM_LLAMACPP_PATH} ]; then
    echo "${UPSTREAM_LLAMACPP_PATH} not exist, pls check"
    exit 1
fi
}


function check_local_llamacpp()
{
if [ "x${LOCAL_LLAMACPP_PATH}" == "x" ]; then
    echo "pls check LOCAL_LLAMACPP_PATH"
    exit 1
fi


if [ ! -d ${LOCAL_LLAMACPP_PATH} ]; then
    echo "${LOCAL_LLAMACPP_PATH} not exist, pls check"
    exit 1
fi
}


function dump_global_envs()
{
    echo -e "\n"
    echo -e "BUILD_USER:           $BUILD_USER"
    echo -e "BUILD_TIME:           $BUILD_TIME"
    echo -e "PROJECT_BUILD_TYPE:   $PROJECT_BUILD_TYPE"
    echo -e "BUILD_HOST:           $BUILD_HOST"
    echo -e "BUILD_TARGET:         $BUILD_TARGET"
    echo -e "BUILD_ARCHS:"
    for arch in ${BUILD_ARCHS};do
    echo -e "                      $arch"
    done
    echo -e "\n"

    echo -e "HOME_PATH:            ${HOME_PATH}"
    echo -e "PROJECT_NAME:         ${PROJECT_NAME}"
    echo -e "PROJECT_BRANCH:       ${PROJECT_BRANCH}"
    echo -e "PROJECT_HOME_PATH:    ${PROJECT_HOME_PATH}"
    echo -e "PROJECT_ROOT_PATH:    ${PROJECT_ROOT_PATH}"
    echo -e "FF_PREFIX:            ${FF_PREFIX}"
    echo -e "KANTV_TOOLCHAIN_PATH: ${KANTV_TOOLCHAIN_PATH}"
    echo -e "host cpu counts:      ${HOST_CPU_COUNTS}"

    if [ "${BUILD_TARGET}" = "android" ]; then
    echo -e "\n"
    echo -e "${TEXT_RED}ANDROID_PLATFORM:         ${ANDROID_PLATFORM}${TEXT_RESET}"
    echo -e "${TEXT_RED}ANDROID_NDK:              ${ANDROID_NDK}${TEXT_RESET}"
    echo -e "LOCAL_WHISPERCPP_PATH:        ${LOCAL_WHISPERCPP_PATH}"
    echo -e "UPSTREAM_WHISPERCPP_PATH:     ${UPSTREAM_WHISPERCPP_PATH}"
    echo -e "LOCAL_LLAMACPP_PATH:          ${LOCAL_LLAMACPP_PATH}"
    echo -e "UPSTREAM_LLAMACPP_PATH:       ${UPSTREAM_LLAMACPP_PATH}"
    echo -e "\n"
    fi

    if [ $BUILD_TARGET == "ios" ]; then
    echo -e "XCODE_PATH:           ${XCODE_PATH}"
    echo -e "\n"
    fi

    if [ "${BUILD_TARGET}" = "wasm" ]; then
    echo -e "\n"
    echo -e "EMSDK:                ${EMSDK}"
    echo -e "EM_CONFIG:            ${EM_CONFIG}"
    echo -e "EMSDK_CMAKE_FILE:     ${EMSDK_CMAKE_FILE}"
    echo -e "NODEJS_ROOT_PATH:     ${NODEJS_ROOT_PATH}"
    echo -e "\n"
    fi

    echo -e "PATH=${PATH}"
    echo -e "\n"
}
