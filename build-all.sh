#!/usr/bin/env bash

# Copyright (c) 2021-2023, zhou.weiguo(zhouwg2000@gmail.com)
# Copyright (c) 2021-2023, Project KanTV
# Copyright (c) 2024- KanTV Authors

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


function build_whispercpp_x86
{
    cd ${PROJECT_ROOT_PATH}/external/whispercpp
    show_pwd

    echo ""
    echo ""
    echo -e "------------------------------------------------------------------------------------------\n"

    echo -e "${TEXT_BLUE}build whispercpp regular tool for target x86${TEXT_RESET}"
    make

    if [ -f main ]; then
        /bin/cp -fv main     ${FF_PREFIX}/bin/
    fi

    if [ -f bench ]; then
        /bin/cp -fv bench    ${FF_PREFIX}/bin/
    fi

    if [ -f quantize ]; then
        /bin/cp -fv quantize ${FF_PREFIX}/bin/
    fi

    echo ""
    echo ""
    echo -e "------------------------------------------------------------------------------------------\n"
    echo ""
    echo ""
    ls -l ${FF_PREFIX}/bin/
    echo -e "------------------------------------------------------------------------------------------\n"

    cd -
}


function build_thirdparty()
{
    third_parties=" ffmpeg-android-build "

    cd ${PROJECT_ROOT_PATH}/external/

    for item in ${third_parties};do
        if [ -d ${PROJECT_ROOT_PATH}/external/${item} ]; then
            cd ${PROJECT_ROOT_PATH}/external/${item}/
            echo "build thirdparty ${item} in `pwd` for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST}"
            if [ -f build.sh ]; then
                ./build.sh
            fi
            cd ${PROJECT_ROOT_PATH}/external/
        else
            echo -e "${TEXT_RED}dir not exist:${PROJECT_ROOT_PATH}/external/${item}${TEXT_RESET}\n"
        fi
    done

    cd ${PROJECT_ROOT_PATH}
}


function build_jni()
{
    jni_libs=" whispercpp "

    cd ${PROJECT_ROOT_PATH}/external/
    for item in ${jni_libs};do
        cd ${PROJECT_ROOT_PATH}/external/${item}/
        echo "build Android JNI lib${item}.so in `pwd` for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST}"
        ./build-android-jni-lib.sh
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
}


function build_nativelibs
{
    build_ffmpeg
    build_kantvcore

    build_thirdparty
    build_jni


}


function build_kantv_apk()
{
    echo ""
    echo "------------------------------------------------------------------------------------------"
    echo -e "[*] to continue to build project KanTV Android APK for ${TEXT_RED} target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST} ${TEXT_RESET}, pls\n"
    echo ""
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


function do_buildlinux()
{
    cd ${PROJECT_ROOT_PATH}/external/ffmpeg-deps
    ./clean-all.sh
    ./build-all.sh
    cd ${PROJECT_ROOT_PATH}/external/ffmpeg
    ./clean-all.sh
    ./build-ffmpeg-x86.sh

    cd ${PROJECT_ROOT_PATH}/examples/ff_terminal
    ./clean.sh
    ./build.sh
    cd ${PROJECT_ROOT_PATH}/examples/ff_encode
    make clean
    make
    #make ff-encode happy
    /bin/cp -fv ${FF_PREFIX}/bin/ffplay  ./
    /bin/cp -fv ${FF_PREFIX}/bin/ffprobe ./

    cd ${PROJECT_ROOT_PATH}/
    build_whispercpp_x86


}


function dump_usage()
{
    echo "Usage:"
    echo "  $0 clean"
    echo "  $0 android"
    echo "  $0 linux"
    echo "  $0 ios"
    echo "  $0 wasm"
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
    android)
        do_buildandroid
    ;;
    linux)
        do_buildlinux
    ;;
    ios)
        do_buildios
    ;;
    wasm)
        do_buildwasm
    ;;
    *)
        dump_usage
        exit 1
    ;;
esac
}



if [ $# == 0 ]; then
    #default target is linux
    user_command="linux"
else
    user_command=$1
fi

build_init

#read -p "Press any key to continue..."

unset $user_command
main $user_command
