#!/usr/bin/env bash

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

#Description: public script functions for entire project
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
            echo -e "${TEXT_RED}host os is ${host_os}, not Ubuntu 20.04.2${TEXT_RESET}\n"
            echo -e "${TEXT_RED}this script only validated ok on host Ubuntu 20.04 x86_64 LTS at the moment${TEXT_RESET}\n"
            #exit 1
            return 1
        fi
    else
        echo -e "${TEXT_RED}/etc/issue not exist, pls check...${TEXT_RESET}\n"
    fi
    return 0;
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

    echo -e "host cpu counts:      ${HOST_CPU_COUNTS}"

    if [ "${BUILD_TARGET}" = "android" ]; then
    echo -e "\n"
    echo -e "ANDROID_NDK:          ${ANDROID_NDK}"
    echo -e "TEE_TDK_DIR:          ${TDK_DIR}"
    echo -e "BOARD_INFO:           ${BOARD_INFO}"
    echo -e "\n"
    fi

    echo "PATH=${PATH}"
    echo -e "\n"
}
