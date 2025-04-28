#!/usr/bin/env bash

# Copyright (c) 2021- KanTV Authors

# Description: configure project's compilation environment
#
#

export TEXT_BLACK=" \033[30m "
export TEXT_RED="   \033[31m "
export TEXT_GREEN=" \033[32m "
export TEXT_BLUE="  \033[34m "
export TEXT_BLACK=" \033[35m "
export TEXT_WHITE=" \033[37m "
export TEXT_RESET=" \033[0m  "

export BUILD_USER=$(whoami)
export BUILD_TIME=`date +"%Y-%m-%d-%H-%M-%S"`

#target iOS must be built on MacOS
#export BUILD_HOST=Mac
#export BUILD_TARGET=ios
#export PROJECT_NAME=KanTV-ios

export BUILD_HOST=Linux

export BUILD_TARGET=android
export PROJECT_NAME=KanTV-android
#default is release for target android
export PROJECT_BUILD_TYPE=release

#default target is linux
#export BUILD_TARGET=linux
#export PROJECT_NAME=KanTV-linux
#default is debug for target linux to purpose of troubleshooting
#export PROJECT_BUILD_TYPE=debug


if [ "${BUILD_TARGET}" == "android" ]; then
    #export BUILD_ARCHS="arm64-v8a armeabi-v7a"
    export BUILD_ARCHS="arm64-v8a"
fi


export HOME_PATH=$(env | grep ^HOME= | cut -c 6-)
export PROJECT_HOME_PATH=`pwd`
export PROJECT_BRANCH=`git branch | grep "*" | cut -f 2 -d ' ' `
export PROJECT_ROOT_PATH=${PROJECT_HOME_PATH}
export PROJECT_OUT_PATH=${PROJECT_ROOT_PATH}/out
export FF_PREFIX=${PROJECT_OUT_PATH}/${BUILD_TARGET}/
#export KANTV_TOOLCHAIN_PATH=/opt/kantv-toolchain
export KANTV_TOOLCHAIN_PATH=${PROJECT_ROOT_PATH}/prebuilts/toolchain
export LOCAL_WHISPERCPP_PATH=${PROJECT_ROOT_PATH}/core/ggml/whispercpp
export LOCAL_LLAMACPP_PATH=${PROJECT_ROOT_PATH}/core/ggml/llamacpp


export KANTV_PROJECTS="kantv-android kantv-linux kantv-ios"
export KANTV_PROJECTS_DEBUG="kantv-android-debug"


#API21：Android 5.0 (Android L)Lollipop
#API22：Android 5.1 (Android L)Lollipop
#API23：Android 6.0 (Android M)Marshmallow
#API24: Android 7.0 (Android N)Nougat
#API25: Android 7.1 (Android N)Nougat
#API26：Android 8.0 (Android O)Oreo
#API27：Android 8.1 (Android O)Oreo
#API28：Android 9.0 (Android P)Pie
#API29：Android 10
#API30：Android 11
#API31：Android 12
#API31：Android 12L
#API33：Android 13
#API34：Android 14
export ANDROID_PLATFORM=android-34
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r18b
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r21e
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r24
#export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r25c
export ANDROID_NDK=${KANTV_TOOLCHAIN_PATH}/android-ndk-r26c
export ANDROID_NDK_ROOT=${ANDROID_NDK}  # make some open source project happy
export NDK_ROOT=${ANDROID_NDK}          # make some open source project happy
export PATH=${ANDROID_NDK_ROOT}:${PATH}

export ANDROID_HOME=${PROJECT_ROOT_PATH}/prebuilts/toolchain/android-sdk
export ANDROID_SDK_ROOT=${ANDROID_HOME} # build with github actions, this is required
export PATH=${ANDROID_HOME}/cmdline-tools/latest/bin:${PATH}
export PATH=${ANDROID_HOME}/cmake/3.22.1/bin:${PATH}


#the following is required for project's maintainers
export UPSTREAM_WHISPERCPP_PATH=~/github/whisper.cpp
export UPSTREAM_LLAMACPP_PATH=~/github/llama.cpp

#the following is required for project's maintainers
export QNN_SDK_ROOT=/opt/qcom/aistack/qnn/2.20.0.240223
export HEXAGON_SDK_ROOT=/opt/qcom/Hexagon_SDK/3.5.0
export TENSORFLOW_HOME=~/.local/lib/python3.8/site-packages/tensorflow/
export PYTHONPATH=${QNN_SDK_ROOT}/lib/python/:${PYTHONPATH}


. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)
if [ $? != 0 ]; then
    echo -e "pls running source build/envsetup.sh firstly\n"
    #exit 1
    return 0
fi


check_host


# clear this variable.  It will be built up again when the vendorsetup.sh
# files are included at the end of this file.
unset LUNCH_MENU_CHOICES
function add_lunch_combo()
{
    local new_combo=$1
    local c
    for c in ${LUNCH_MENU_CHOICES[@]} ; do
        if [ "$new_combo" = "$c" ] ; then
            return
        fi
    done
    LUNCH_MENU_CHOICES=(${LUNCH_MENU_CHOICES[@]} $new_combo)
}


for prj in ${KANTV_PROJECTS};do
    echo -e "  ${prj}"
    add_lunch_combo ${prj}
done

for prj in ${KANTV_PROJECTS_DEBUG};do
    echo -e "  ${prj}"
    add_lunch_combo ${prj}
done


echo -e "\n"
echo "------------------------------------------------------------------------------------------"
echo -e "[*] to continue to build project, pls run\n"
echo -e "lunch\n"
echo -e "[*] or\n"
echo -e "./build/build-all.sh\n"
echo -e "[*] for default target android\n"
echo "------------------------------------------------------------------------------------------"
echo -e "\n"


function destroy_build_var_cache()
{
    #unset FF_PREFIX
    #unset BUILD_TARGET
    local v
    for v in $cached_vars; do
      unset var_cache_$v
    done
    unset cached_vars
}


function print_lunch_menu()
{
    local uname=$(uname)
    echo
    echo "You're building on" $uname
    echo
    echo "Lunch menu... pick a combo:"

    local i=1
    local choice
    for choice in ${LUNCH_MENU_CHOICES[@]}
    do
        echo "     $i. $choice"
        i=$(($i+1))
    done

    echo
}


function lunch()
{
    destroy_build_var_cache

    local debug
    local answer

    debug=0

    if [ "$1" ] ; then
        answer=$1
    else
        print_lunch_menu

        echo -n "Which would you like? [kantv-android] "
        read answer
    fi

    local selection=

    if [ ${debug} -eq 1 ]; then
        echo "answer: ${answer}"
    fi

    if [ -z "$answer" ]
    then
        selection=kantv
    elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$")
    then
        if [ $answer -le ${#LUNCH_MENU_CHOICES[@]} ]
        then
            selection=${LUNCH_MENU_CHOICES[$(($answer-1))]}
        fi
    else
        selection=$answer
    fi

    local project
    project=${selection}


    if [ ${debug} -eq 1 ]; then
        echo "project: ${selection}"
        echo "selection: ${selection}"
    fi

    local product
    local variant_and_version
    local variant
    local version
    product=${selection%%-*} # trim everything after first dash
    variant_and_version=${selection#*-} # trim everything up to first dash

    if [ ${debug} -eq 1 ]; then
        echo "product: ${product}"
        echo "variant_and_version: ${variant_and_version}"
    fi

    if [ "$variant_and_version" != "$selection" ]; then
        variant=${variant_and_version%%-*}
        if [ "$variant" != "$variant_and_version" ]; then
            version=${variant_and_version#*-}
        fi
    fi

    if [ -z "$product" ]
    then
        echo
        echo "invalid lunch combo: $selection"
        return 1
    fi

    if [ ${debug} -eq 1 ]; then
        echo "project name: ${project}"
        echo "target_product: ${product}"
        echo "target_build_variant: ${variant}"
        echo "target_build_type: ${version}"
    fi

    if [ "x${version}" != "x" ]; then
        export PROJECT_BUILD_TYPE=${version}
    else
        export PROJECT_BUILD_TYPE=release
    fi


    if [ "x${project}" != "x" ]; then
        if [ "${project}" == "kantv-android" ]; then
            export BUILD_TARGET=android
        elif [ "${project}" == "kantv-android-debug" ]; then
            export BUILD_TARGET=android
        elif [ "${project}" == "kantv-linux" ]; then
            export BUILD_TARGET=linux
        elif [ "${project}" == "kantv-ios" ]; then
            export BUILD_TARGET=ios
        elif [ "${project}" == "kantv-wasm" ]; then
            export BUILD_TARGET=wasm
        else
            #default target is android
            export BUILD_TARGET=android
        fi

        setup_env

        if [ ${debug} -eq 1 ]; then
            echo "build target: ${BUILD_TARGET}"
            echo "project build command:${PROJECT_BUILD_COMMAND}"
        fi
    else
        echo
        echo -e "${TEXT_RED}invalid lunch combo: $selection ${TEXT_RESET}\n"
        return 1
    fi

    if [ $? -ne 0 ]; then
        echo -e "${TEXT_RED}invalid lunch combo: $selection ${TEXT_RESET}\n"
        return 1
    fi



    dump_global_envs

    case "${BUILD_TARGET}" in
        android)
            check_ndk
        ;;
        ios)
            check_xcode
        ;;
        wasm)
            check_emsdk
        ;;
        linux)
            check_ubuntu
        ;;
        *)
            echo "unknown target ${BUILD_TARGET}"
        ;;
    esac


    echo ""

    echo ""

    echo ""
    echo ""
    echo "------------------------------------------------------------------------------------------"
    echo -e "[*] to continue to build project kantv-${BUILD_TARGET} for ${TEXT_RED} target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST} ${TEXT_RESET}, pls run\n"
    echo ""
    echo -e "${PROJECT_BUILD_COMMAND}"
    echo ""
    echo ""
    echo "in current pwd:  `pwd`"
    echo "------------------------------------------------------------------------------------------"
    echo ""
    echo ""
}
