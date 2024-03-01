#!/usr/bin/env bash

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.


function build_init()
{
    if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
        echo "pwd is `pwd`"
        echo "pls run . build/envsetup in project's toplevel directory firstly"
        exit 1
    fi

    . ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

    echo "build project ${PROJECT_NAME} on ${BUILD_TIME} by ${BUILD_USER}"

    show_pwd
}


function build_ffmpeg
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build ffmpeg for target ${BUILD_TAREGT} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_nativelibs
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build nativelibs for target ${BUILD_TAREGT} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_jni
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build jni for target ${BUILD_TAREGT} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_kantv_apk()
{
    echo "pls build kantv apk by latest Android Studio IDE"
}


function build_check()
{
    cd ${PROJECT_ROOT_PATH}
    echo -e "begin check...\n"

    echo -e "end check...\n"
}



build_init
build_ffmpeg
build_nativelibs
build_jni
build_kantv_apk
build_check
