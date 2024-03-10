#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2021-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: top level build script for entire project

# validated ok or works well on host Ubuntu 20.04


function build_init()
{
    if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
        echo "pwd is `pwd`"
        echo "pls run . build/envsetup in project's toplevel directory firstly"
        exit 1
    fi

    . ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

    echo -e "build project ${PROJECT_NAME} on ${BUILD_TIME} by ${BUILD_USER} for ${TEXT_RED} target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST} ${TEXT_RESET}\n"

    show_pwd
}


function build_ffmpeg
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build ffmpeg for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_kantvcore
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build kantvcore for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_jni
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build jni for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
}


function build_OpenBLAS
{
    cd ${PROJECT_ROOT_PATH}/external/OpenBLAS
    show_pwd
    echo "build OpenBLAS for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
    ./build.sh

    cd -
}


function build_whispercpp
{
    cd ${PROJECT_ROOT_PATH}/external/whispercpp
    show_pwd
    echo "build whispercpp for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"
    ./build.sh

    cd -
}


function build_thirdparty()
{
    third_parties=" OpenBLAS whispercpp "

    cd ${PROJECT_ROOT_PATH}/external/
    for item in ${third_parties};do
        cd ${PROJECT_ROOT_PATH}/external/${item}/
        echo "build ${item} in `pwd` for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST}"
        ./build.sh
        cd ${PROJECT_ROOT_PATH}/external/
    done

    cd ${PROJECT_ROOT_PATH}
}


#for sanity check of build system
function build_nativelibs_1
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build nativelibs for target ${BUILD_TAREGT} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode"

    build_ffmpeg
    build_kantvcore
    build_jni

    build_OpenBLAS
    build_whispercpp
}


function build_nativelibs
{
    build_ffmpeg
    build_kantvcore
    build_jni

    build_thirdparty
}


function build_kantv_apk()
{
    echo ""
    echo "------------------------------------------------------------------------------------------"
    echo -e "[*] to continue to build project KanTV for ${TEXT_RED} target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST} ${TEXT_RESET}, pls\n"
    echo ""
    echo -e "${PROJECT_BUILD_SCRIPT}"
    echo -e "${TEXT_BLUE}build KanTV apk by latest Android Studio IDE ${TEXT_RESET}\n"
    echo ""
    echo ""
    echo "------------------------------------------------------------------------------------------"
    echo ""
}


function build_check()
{

if [ "${BUILD_TARGET}" == "android" ]; then
    cd ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/src/main/jniLibs/arm64-v8a/
    show_pwd

    echo -e "begin check...\n"

    ls -l   ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/src/main/jniLibs/arm64-v8a/
    ls -lah ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/src/main/jniLibs/arm64-v8a/

    echo -e "\nend check...\n"

    cd ${PROJECT_ROOT_PATH}
fi

}


function do_buildandroid()
{
    build_init

    build_nativelibs
    build_jni

    build_check

    build_kantv_apk
}


function dump_usage()
{
    echo "Usage:"
    echo "  $0 clean"
    echo "  $0 buildandroid"
    echo "  $0 buildios"
    echo "  $0 buildwasm"
    echo -e "\n\n\n"
}


function do_buildios()
{
    setup_env
    check_xcode
    dump_global_envs

    echo -e "${TEXT_BLUE} target ${BUILD_TARGET} not support currently ${TEXT_RESET}\n"
    dump_usage
}


function do_buildwasm()
{
    setup_env
    check_emsdk
    dump_global_envs

    echo -e "${TEXT_BLUE} target ${BUILD_TARGET} not support currently ${TEXT_RESET}\n\n"
    dump_usage
}


function do_clean()
{
    echo -e "${TEXT_BLUE} nothing to do currently ${TEXT_RESET}\n\n"
    dump_usage
}


function main()
{
case "$user_command" in
    clean)
        do_clean
    ;;
    buildandroid)
        export BUILD_TARGET=android
        do_buildandroid
    ;;
    buildios)
        export BUILD_TARGET=ios
        do_buildios
    ;;
    buildwasm)
        export BUILD_TARGET=wasm
        do_buildwasm
    ;;
    *)
        dump_usage
        exit 1
    ;;
esac
}


if [ $# == 0 ]; then
    user_command="buildandroid"
else
    user_command=$1
fi

build_init

#read -p "Press any key to continue..."

main $user_command
