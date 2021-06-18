#!/usr/bin/env bash

# prepare ffmpeg and openssl codes and build all native libs for Android/iOS and build Android JNI libs automatically

# validate ok on Ubuntu 20.04.2 x86_64 LTS
# validate ok on macOS Catalina(10.15.7) x86_64

# Special thanks to Zhang Rui <bbcallen@gmail.com> & Bilibili

# Special thanks to:
# https://github.com/bilibili/ijkplayer/blob/master/init-android.sh
# https://github.com/bilibili/ijkplayer/blob/master/init-android-openssl.sh
# https://github.com/bilibili/ijkplayer/blob/master/init-ios.sh
# https://github.com/bilibili/ijkplayer/blob/master/init-ios-openssl.sh


# zhou.weiguo <zhouwg2000@gmail.com> 2021-05-12,16:19
# zhou.weiguo <zhouwg2000@gmail.com> 2021-05-22,17:26, refine for target iOS


function setup_env()
{
    export TEXT_BLACK=" \033[30m "
    export TEXT_RED="   \033[31m "
    export TEXT_GREEN=" \033[32m "
    export TEXT_BLUE="  \033[34m "
    export TEXT_BLACK=" \033[35m "
    export TEXT_WHITE=" \033[37m "
    export TEXT_RESET=" \033[0m  "

    export BUILD_USER=$(whoami)
    export BUILD_TIME=`date +"%Y-%m-%d,%H:%M:%S"`
    export BUILD_HOST=Linux
    export BUILD_TARGET=android
    export BUILD_ARCHS="arm64-v8a x86_64"
    export BUILD_TYPE=release


    export HOME_PATH=$(env | grep ^HOME= | cut -c 6-)
    export PROJECT_NAME=ijkplayer
    export PROJECT_ROOT_PATH=$(pwd)
    #ndk-r14b must be used in this project but I guess clang mightbe ok
    export ANDROID_NDK=/opt/android-ndk-r14b
    export XCODE_SELECT=/usr/bin/xcode-select
    #TODO:better idea to get real cpu counts in macOS
    export HOST_CPU_COUNTS=4

    TOOLS=${PROJECT_ROOT_PATH}/tools

    #the speed of fetch code from github.com is very very very slow
    #I have to switch from github.com to gitee.com
    #for domestic users, it will be updated aperiodicity
    #IJK_FFMPEG_UPSTREAM=https://gitee.com/zhouweiguo2020/FFmpeg.git
    #IJK_FFMPEG_FORK=https://gitee.com/zhouweiguo2020/FFmpeg.git

    #for overseas users,  up-to-date codes would be found here to keep pace with hijkplayer
    IJK_FFMPEG_UPSTREAM=https://github.com/zhouwg/FFmpeg.git
    IJK_FFMPEG_FORK=https://github.com/zhouwg/FFmpeg.git
    IJK_FFMPEG_COMMIT=release/4.4
    IJK_FFMPEG_LOCAL_REPO=extra/ffmpeg

    IJK_GASP_UPSTREAM=https://gitee.com/zhouweiguo2020/gas-preprocessor.git

    IJK_OPENSSL_UPSTREAM=https://gitee.com/zhouweiguo2020/openssl.git
    IJK_OPENSSL_FORK=https://gitee.com/zhouweiguo2020/openssl.git
    IJK_OPENSSL_COMMIT=OpenSSL_1_1_1-stable
    IJK_OPENSSL_LOCAL_REPO=extra/openssl

    #https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.16.tar.gz
    IJK_LIBICONV_UPSTREAM=https://gitee.com/zhouweiguo2020/libiconv.git
    IJK_LIBICONV_FORK=https://gitee.com/zhouweiguo2020/libiconv.git
    IJK_LIBICONV_COMMIT=master
    IJK_LIBICONV_LOCAL_REPO=extra/libiconv

    IJK_LIBZ_UPSTREAM=https://gitee.com/zhouweiguo2020/libz.git
    IJK_LIBZ_FORK=https://gitee.com/zhouweiguo2020/libz.git
    IJK_LIBZ_COMMIT=master
    IJK_LIBZ_LOCAL_REPO=extra/libz

    #IJK_LIBXML2_FORK=https://gitlab.gnome.org/GNOME/libxml2.git
    IJK_LIBXML2_UPSTREAM=https://gitee.com/zhouweiguo2020/libxml2.git
    IJK_LIBXML2_FORK=https://gitee.com/zhouweiguo2020/libxml2.git
    IJK_LIBXML2_COMMIT=master
    IJK_LIBXML2_LOCAL_REPO=extra/libxml2

    IJK_LIBCURL_UPSTREAM=https://gitee.com/zhouweiguo2020/curl.git
    IJK_LIBCURL_FORK=https://gitee.com/zhouweiguo2020/curl.git
    IJK_LIBCURL_COMMIT=master
    IJK_LIBCURL_LOCAL_REPO=extra/curl
}


function show_pwd()
{
    echo -e "current working path:$(pwd)\n"
}


function check_ndk()
{
    if [ $BUILD_TARGET == "android" ]; then
        if [ ! -d ${ANDROID_NDK} ]; then
            echo -e "${TEXT_RED}${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
            exit 1
        fi
    fi
}


function check_xcode()
{
    if [ $BUILD_TARGET == "ios" ]; then
        echo -e "${TEXT_BLUE}check xcode...${TEXT_RESET}\n"
        if [ ! -f ${XCODE_SELECT} ]; then
            echo -e "${TEXT_RED}${XCODE_SELECT} not exist, pls check...${TEXT_RESET}\n"
            exit 1
        fi
    fi
}


function check_ubuntu()
{
    if [ ${BUILD_HOST} != "Linux" ]; then
        echo -e "${TEXT_RED}host os is ${BUILD_HOST}${TEXT_RESET}\n"
        return 0;
    fi

    if [ -f /etc/issue ]; then
        host_os=`cat /etc/issue | awk '{print $1 $2}'`
        if [ ${host_os} == "Ubuntu20.04.2" ]; then
            echo -e "${TEXT_BLUE}host os is Ubuntu 20.04.2${TEXT_RESET}\n"
        else
            echo -e "${TEXT_RED}host os is ${host_os}, not Ubuntu 20.04.2${TEXT_RESET}\n"
            echo -e "${TEXT_RED}this script only validated ok on host Ubuntu 20.04.2 x86_64 LTS for target Android at the moment${TEXT_RESET}\n"
            echo -e "${TEXT_RED}it might be running ok on other Linux-like OS${TEXT_RESET}\n"
            exit 1
        fi
    else
        echo -e "${TEXT_RED}/etc/issue not exist, pls check...${TEXT_RESET}\n"
    fi
    return 0;
}


function check_host()
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd

    uname_result=`uname -s`
    case ${uname_result} in
        Linux*)  machine="Linux";;
        Darwin*) machine="Mac";;
        *)       machine="Unknown:${uname_result}";;
    esac

    if [ $machine = "Linux" ]; then
        echo -e "${TEXT_BLUE}host os is ${machine}, reset BUILD_TARGET to android${TEXT_RESET}"
        export BUILD_TARGET=android
        export BUILD_HOST=Linux
        export HOST_CPU_COUNTS=`cat /proc/cpuinfo | grep "processor" | wc | awk '{print int($1)}'`
        export BUILD_TYPE=release

        #some files must be modified according to android/patch-debugging-with-lldb.sh for debug build
        #becareful there some conflicts between release build and debug build for final Android apk
        #only validated in my Linux dev-machine and careful sanity check is required before committed to github
        #export BUILD_TYPE=debug

        #TODO:OpenSSL_1_1_1-stable & FFmpeg4.4 not working with arch-eabi "armv5 armeabi-v7a x86"
        #working well with arch-eabi "arm64-v8a x86_64" for target Android
        #full archs for Android
        #export BUILD_ARCHS="armv5 armeabi-v7a arm64-v8a x86 x86_64"
        export BUILD_ARCHS="arm64-v8a x86_64"
    elif [ $machine = "Mac" ]; then
        echo -e "${TEXT_BLUE}host os is ${machine}, reset BUILD_TARGET to ios${TEXT_RESET}"
        export BUILD_TARGET=ios
        export BUILD_HOST=Mac
        export XCODE_PATH=`xcode-select -p`
        export BUILD_TYPE=release

        #OpenSSL_1_1_1-stable & FFmpeg4.4
        #working well with arch-eabi "armv7 arm64 i386 x86_64" for target iOS
        export BUILD_ARCHS="armv7 arm64 i386 x86_64"
    else
        echo -e "${TEXT_RED}host os is ${machine}, not supported at the moment...${TEXT_RESET}\n"
        exit 1
    fi

    if [ $BUILD_TARGET == "android" ]; then
        echo -e "${TEXT_BLUE}BUILD_TARGET is $BUILD_TARGET${TEXT_RESET}"
        check_ndk
        check_ubuntu

        #make Android Studio happy
        if [ -f android/ijkplayer/local.properties ]; then
            echo "ANDROID_NDK is: ${ANDROID_NDK}"
            if cat './android/ijkplayer/local.properties' | grep $ANDROID_NDK ; then
                echo "found $ANDROID_NDK in ./android/ijkplayer/local.properties"
            else
                echo -e "\n" >> android/ijkplayer/local.properties
                echo "ndk.dir=${ANDROID_NDK}" >> android/ijkplayer/local.properties
            fi
        fi
    elif [ $BUILD_TARGET == "ios" ]; then
        echo -e "${TEXT_BLUE}BUILD_TARGET is $BUILD_TARGET${TEXT_RESET}"
        check_xcode
    else
        echo -e "${TEXT_RED}${BUILD_TARGET} unknown,pls check...${TEXT_RESET}"
        exit 1
    fi

    dump_envs
}


function dump_envs()
{
    echo -e "\n"
    echo "BUILD_USER:        $BUILD_USER"
    echo "BUILD_TIME:        $BUILD_TIME"
    echo "BUILD_TYPE:        $BUILD_TYPE"
    echo "BUILD_HOST:        $BUILD_HOST"
    echo "BUILD_TARGET:      $BUILD_TARGET"
    echo "BUILD_ARCHS:"
    for arch in ${BUILD_ARCHS};do
        echo "                   $arch"
    done
    echo "HOME_PATH:         $HOME_PATH"
    if [ $BUILD_TARGET == "android" ]; then
        echo "ANDROID_NDK:       $ANDROID_NDK"
    fi
    if [ $BUILD_TARGET == "ios" ]; then
        echo "XCODE_PATH:        $XCODE_PATH"
    fi
    echo "PROJECT_ROOT_PATH: $PROJECT_ROOT_PATH"
    echo "host cpu counts  : $HOST_CPU_COUNTS"
    echo "PATH=${PATH}"
    echo -e "\n"
}


function dump_usage()
{
    echo "Usage:"
    echo "  $0 clean"
    echo "  $0 cleanall"
    echo "  $0 init"
    echo "  $0 build"
    exit 1
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


function check_sources()
{
    module_name=$1
    if [ $# -ne 1 ]; then
        echo -e "${TEXT_RED}empty module_name with function check_sources,pls check...${TEXT_RESET}"
        return 1
    fi

    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "======enter function check sources of $module_name ======"

    #it should be local variable in a standalone function
    #but be a global variable might be a better idea because the same variable
    #would be defined in other place
    #BUILD_ARCHS=(armv5 armeabi-v7a arm64-v8a x86 x86_64)

    notexist=0
    for arch in ${BUILD_ARCHS};do
        if [ ${arch} == "arm64-v8a" ] || [ ${arch} == "arm64_v8a" ]; then
            realname="arm64"
        elif [ ${arch} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${arch}
        fi
        if [ ${BUILD_TARGET} == "android" ]; then
            test_path=./${BUILD_TARGET}/contrib/${module_name}-${realname}
            echo "test_path:$test_path"
        elif [ ${BUILD_TARGET} == "ios" ]; then
            test_path=./${BUILD_TARGET}/${module_name}-${realname}
            echo "test_path:$test_path"
        else
            echo -e "${TEXT_RED}${BUILD_TARGET} unknown,pls check...${TEXT_RESET}"
            exit 1
        fi

        if [ ! -d ${test_path} ]; then
            echo_left_align "${TEXT_RED}${test_path}${TEXT_RESET}" "  not exist"
            echo -e "${TEXT_RED}prepare $module_name codes for target ${realname}...${TEXT_RESET}"
            if [ $module_name == "ffmpeg" ]; then
                git --version
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh ./init-config.sh
                    sh ./init-android-libyuv.sh
                    sh ./init-android-soundtouch.sh
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    echo "== pull gas-preprocessor base =="
                    sh $TOOLS/pull-repo-base.sh $IJK_GASP_UPSTREAM extra/gas-preprocessor
                fi

                echo "== pull ffmpeg base =="
                sh $TOOLS/pull-repo-base.sh $IJK_FFMPEG_UPSTREAM $IJK_FFMPEG_LOCAL_REPO
                echo "== pull ffmpeg fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    echo "$TOOLS/pull-repo-ref.sh $IJK_FFMPEG_FORK android/contrib/ffmpeg-${realname} ${IJK_FFMPEG_LOCAL_REPO}"
                    sh $TOOLS/pull-repo-ref.sh $IJK_FFMPEG_FORK android/contrib/ffmpeg-${realname} ${IJK_FFMPEG_LOCAL_REPO}
                    cd android/contrib/ffmpeg-${realname}
                    show_pwd
                    git checkout ${IJK_FFMPEG_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    echo "$TOOLS/pull-repo-ref.sh $IJK_FFMPEG_FORK ios/ffmpeg-${realname} ${IJK_FFMPEG_LOCAL_REPO}"
                    sh $TOOLS/pull-repo-ref.sh $IJK_FFMPEG_FORK ios/ffmpeg-${realname} ${IJK_FFMPEG_LOCAL_REPO}
                    cd ios/ffmpeg-${realname}
                    show_pwd
                    git checkout ${IJK_FFMPEG_COMMIT}
                fi
                cd ${PROJECT_ROOT_PATH}

                #./init-${BUILD_TARGET}.sh ${realname}
            elif [ $module_name == "openssl" ]; then
                git --version
                echo "== pull openssl base =="
                sh $TOOLS/pull-repo-base.sh $IJK_OPENSSL_UPSTREAM $IJK_OPENSSL_LOCAL_REPO
                echo "== pull openssl fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_OPENSSL_FORK android/contrib/openssl-${realname} ${IJK_OPENSSL_LOCAL_REPO}
                    cd android/contrib/openssl-${realname}
                    show_pwd
                    git checkout ${IJK_OPENSSL_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_OPENSSL_FORK ios/openssl-${realname} ${IJK_OPENSSL_LOCAL_REPO}
                    cd ios/openssl-${realname}
                    show_pwd
                    git checkout ${IJK_OPENSSL_COMMIT}
                fi
                cd ${PROJECT_ROOT_PATH}
                #./init-${BUILD_TARGET}-openssl.sh ${realname}
            elif [ $module_name == "libiconv" ]; then
                git --version
                echo "== pull libiconv base =="
                sh $TOOLS/pull-repo-base.sh $IJK_LIBICONV_UPSTREAM $IJK_LIBICONV_LOCAL_REPO
                echo "== pull libiconv fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBICONV_FORK android/contrib/libiconv-${realname} ${IJK_LIBICONV_LOCAL_REPO}
                    cd android/contrib/libiconv-${realname}
                    show_pwd
                    git checkout ${IJK_LIBICONV_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBICONV_FORK ios/libiconv-${realname} ${IJK_LIBICONV_LOCAL_REPO}
                    cd ios/libiconv-${realname}
                    show_pwd
                    git checkout ${IJK_LIBICONV_COMMIT}
                fi
                cd ${PROJECT_ROOT_PATH}
            elif [ $module_name == "libz" ]; then
                git --version
                echo "== pull libz base =="
                sh $TOOLS/pull-repo-base.sh $IJK_LIBZ_UPSTREAM $IJK_LIBZ_LOCAL_REPO
                echo "== pull libz fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBZ_FORK android/contrib/libz-${realname} ${IJK_LIBZ_LOCAL_REPO}
                    cd android/contrib/libz-${realname}
                    show_pwd
                    git checkout ${IJK_LIBZ_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBZ_FORK ios/libz-${realname} ${IJK_LIBZ_LOCAL_REPO}
                    cd ios/libz-${realname}
                    show_pwd
                    git checkout ${IJK_LIBZ_COMMIT}
                fi
                cd ${PROJECT_ROOT_PATH}
            elif [ $module_name == "libxml2" ]; then
                git --version
                echo "== pull libxml2 base =="
                sh $TOOLS/pull-repo-base.sh $IJK_LIBXML2_UPSTREAM $IJK_LIBXML2_LOCAL_REPO
                echo "== pull libxml2 fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBXML2_FORK android/contrib/libxml2-${realname} ${IJK_LIBXML2_LOCAL_REPO}
                    cd android/contrib/libxml2-${realname}
                    show_pwd
                    git checkout ${IJK_LIBXML2_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBXML2_FORK ios/libxml2-${realname} ${IJK_LIBXML2_LOCAL_REPO}
                    cd ios/libxml2-${realname}
                    show_pwd
                    git checkout ${IJK_LIBXML2_COMMIT}
                fi
                cd ${PROJECT_ROOT_PATH}
            elif [ $module_name == "curl" ]; then
                git --version
                echo "== pull libcurl base =="
                sh $TOOLS/pull-repo-base.sh $IJK_LIBCURL_UPSTREAM $IJK_LIBCURL_LOCAL_REPO
                echo "== pull libcurl fork ${realname} =="
                if [ ${BUILD_TARGET} == "android" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBCURL_FORK android/contrib/curl-${realname} ${IJK_LIBCURL_LOCAL_REPO}
                    cd android/contrib/curl-${realname}
                    show_pwd
                    git checkout ${IJK_LIBCURL_COMMIT}
                fi
                if [ ${BUILD_TARGET} == "ios" ]; then
                    sh $TOOLS/pull-repo-ref.sh $IJK_LIBCURL_FORK ios/curl-${realname} ${IJK_LIBCURL_LOCAL_REPO}
                    cd ios/curl-${realname}
                    show_pwd
                    git checkout ${IJK_LIBCURL_COMMIT}
                fi
            else
                echo -e "${TEXT_RED}dependent module ${module_name} unknown, pls check...${TEXT_RESET}"
            fi
            let notexist++
        else
            echo_left_align "${test_path}" "exist"
        fi
    done

    cd ${PROJECT_ROOT_PATH}
    echo "======leave function check sources of $module_name ======"
    echo -e "\n"
}


function build_native_debug()
{
    set -e
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build native libs and JNI..."

    if [ $BUILD_TARGET != "android" ]; then
        #focus on Android debug build at the moment
        echo -e "${TEXT_RED}do nothing because BUILD_TARGET is $BUILD_TARGET, not android ...${TEXT_RESET}"
        return;
    fi

    for arch in ${BUILD_ARCHS};do
        echo "arch = ${arch}"
        if [ ${arch} == "arm64-v8a" ] || [ ${arch} == "arm64_v8a" ]; then
            realname="arm64"
        elif [ ${arch} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${arch}
        fi

        if [ ! -d ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jni ]; then
            echo "${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jni not exist"
            mkdir -p ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jni
        else
            echo "${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jni already exist"
        fi
        cd ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jni

        show_pwd
        ${ANDROID_NDK}/ndk-build APP_BUILD_SCRIPT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Android.mk NDK_APPLICATION_MK=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Application.mk APP_ABI=${arch} NDK_ALL_ABIS=${arch} NDK_DEBUG=1 APP_PLATFORM=android-21 NDK_OUT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realnaem}/build/intermediates/ndkBuild/debug/obj NDK_LIBS_OUT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib NDK_LOG=1 V=1 clean
        ${ANDROID_NDK}/ndk-build APP_BUILD_SCRIPT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Android.mk NDK_APPLICATION_MK=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Application.mk APP_ABI=${arch} NDK_ALL_ABIS=${arch} NDK_DEBUG=1 APP_PLATFORM=android-21 NDK_OUT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realnaem}/build/intermediates/ndkBuild/debug/obj NDK_LIBS_OUT=${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib NDK_LOG=1 V=1

        if [ ! -d ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} ]; then
            echo -e "${TEXT_RED}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} not exist${TEXT_RESET}"
            mkdir -p ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}
        else
            echo -e "${TEXT_BLUE}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} already exist${TEXT_RESET}"
        fi
        echo -e "/bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${arch}/libijksdl.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}"
        /bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${arch}/libijksdl.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}


        echo -e "/bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${arch}/libijkplayer.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}"
        /bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${arch}/libijkplayer.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}
    done
    set +e
}


function build_module()
{
    module_name=$1
    target_name=$2
    if [ $# -ne 2 ]; then
        echo -e "${TEXT_RED}empty module_name target_name with function build_module,pls check...${TEXT_RESET}"
        return 1
    fi

    echo "build ${module_name}-${target_name}..."
    if [ -f ./compile-${module_name}.sh ]; then
        ./compile-${module_name}.sh ${target_name}

        if [ $? != 0 ]; then
            echo -e "${TEXT_RED} build ${module_name} failed ${TEXT_RESET}"
            exit 1
        else
            echo -e "${TEXT_BLUE} build ${module_name} successed ${TEXT_RESET}"
        fi
    else
        show_pwd
        echo -e "${TEXT_RED} ./compile-${module_name}.sh not exist, it shouldn't happen here, pls check...${TEXT_RESET}"
    fi
}


function build_native_release()
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd
    echo "build native libs..."

    for arch in ${BUILD_ARCHS};do
        echo "arch = ${arch}"
        if [ ${arch} == "arm64-v8a" ] || [ ${arch} == "arm64_v8a" ]; then
            realname="arm64"
        elif [ ${arch} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${arch}
        fi

        if [ $BUILD_TARGET == "android" ]; then
            cd ${PROJECT_ROOT_PATH}/android/contrib
        elif [ $BUILD_TARGET == "ios" ]; then
            cd ${PROJECT_ROOT_PATH}/ios
        else
            echo -e "${TEXT_RED} BUILD_TARGET $BUILD_TARGET unknown,pls check...${TEXT_RESET}"
            continue;
        fi
        show_pwd

        build_module libiconv ${realname}
        build_module libz     ${realname}
        build_module libxml2  ${realname}
        build_module curl     ${realname}
        build_module openssl  ${realname}
        build_module ffmpeg   ${realname}

        if [ $BUILD_TARGET != "android" ]; then
            #TODO: build ijksdl and ijkplayer via this script in macOS for target ios
            continue
        fi

        cd ${PROJECT_ROOT_PATH}/android
        show_pwd
        build_module ijk     ${realname}

        echo -e "\n"
        if [ ! -d ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} ]; then
            echo -e "${TEXT_RED}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} not exist${TEXT_RESET}"
            mkdir -p ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}
        else
            echo -e "${TEXT_BLUE}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch} already exist${TEXT_RESET}"
        fi
        cd ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs
        show_pwd
        if [ ! -f ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${arch}/libijksdl.so ]; then
            echo -e "${TEXT_RED}${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${arch}/libijksdl.so not exist, it shouldn't happen, pls check ${BUILD_ARCHS}"
            exit 1
        fi
        ls -l ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${arch}/libijk*.so
        echo "/bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${arch}/libijk*.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}"
        /bin/cp -fv ${PROJECT_ROOT_PATH}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${arch}/libijk*.so  ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs/${arch}
    done

    if [ $BUILD_TARGET == "ios" ]; then
        cd ${PROJECT_ROOT_PATH}/ios
        ./compile-libiconv.sh lipo
        ./compile-libz.sh lipo
        ./compile-libxml2.sh lipo
        ./compile-curl.sh lipo
        ./compile-openssl.sh lipo
        ./compile-ffmpeg.sh lipo
        cd ${PROJECT_ROOT_PATH}
    fi

    echo -e "\n"
}


function do_clean()
{
    cd ${PROJECT_ROOT_PATH}
    show_pwd

    if [ $BUILD_TARGET == "android" ]; then
        echo -e "${TEXT_RED}remove android/contrib/build${TEXT_RESET}"
        if [ -d android/contrib/build ]; then
            rm -rf android/contrib/build
        fi

        if [ ! -d ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs ]; then
            echo -e "${TEXT_RED}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs not exist${TEXT_RESET}\n"
        else
            echo -e "${TEXT_BLUE}${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs exist${TEXT_RESET}, remove it\n"
            rm -rf ${PROJECT_ROOT_PATH}/android/ijkplayer/kantv/src/main/jniLibs
        fi
    elif [ $BUILD_TARGET == "ios" ]; then
        echo -e "${TEXT_RED}remove ios/build${TEXT_RESET}"
        if [ -d ios/build ]; then
            rm -rf ios/build
        fi
    fi

    for arch in ${BUILD_ARCHS};do
        if [ ${arch} == "arm64-v8a" ] || [ ${arch} == "arm64_v8a" ]; then
            realname="arm64"
        elif [ ${arch} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${arch}
        fi

        if [ $BUILD_TARGET == "android" ]; then
            cd ${PROJECT_ROOT_PATH}/android/contrib
        elif [ $BUILD_TARGET == "ios" ]; then
            cd ${PROJECT_ROOT_PATH}/ios
        else
            echo -e "${TEXT_RED} BUILD_TARGET $BUILD_TARGET unknown,pls check...${TEXT_RESET}"
            continue;
        fi
        show_pwd
        if [ -f ffmpeg-${realname}/config.h ];then
            echo -e "${TEXT_BLUE} ffmpeg-${realname}/config.h exist${TEXT_RESET}, remove it\n"
            rm -f ffmpeg-${realname}/config.h
        else
            echo -e "${TEXT_RED} ffmpeg-${realname}/config.h not exist${TEXT_RESET}\n"
        fi
        if [ -f openssl-${realname}/Makefile ];then
            echo -e "${TEXT_BLUE} openssl-${realname}/Makefile exist${TEXT_RESET}, remove it\n"
            rm -f openssl-${realname}/Makefile
        else
            echo -e "${TEXT_RED} openssl-${realname}/Makefile not exist${TEXT_RESET}\n"
        fi
    done
}


function do_cleanall()
{
    do_clean

    cd ${PROJECT_ROOT_PATH}
    show_pwd

    if [ $BUILD_TARGET == "android" ]; then
        echo -e "${TEXT_RED}remove android/contrib/ffmpeg-*${TEXT_RESET}"
        rm -rf android/contrib/ffmpeg-*

        echo -e "${TEXT_RED}remove android/contrib/openssl-*${TEXT_RESET}"
        rm -rf android/contrib/openssl-*

        echo -e "${TEXT_RED}remove android/contrib/curl-*${TEXT_RESET}"
        rm -rf android/contrib/curl-*

        echo -e "${TEXT_RED}remove android/contrib/libxml2-*${TEXT_RESET}"
        rm -rf android/contrib/libxml2-*

        echo -e "${TEXT_RED}remove android/contrib/libiconv-*${TEXT_RESET}"
        rm -rf android/contrib/libiconv-*

        echo -e "${TEXT_RED}remove android/contrib/libz-*${TEXT_RESET}"
        rm -rf android/contrib/libz-*
    elif [ $BUILD_TARGET == "ios" ]; then
        echo -e "${TEXT_RED}remove ios/ffmpeg-*${TEXT_RESET}"
        rm -rf ios/ffmpeg-*

        echo -e "${TEXT_RED}remove ios/openssl-*${TEXT_RESET}"
        rm -rf ios/openssl-*

        echo -e "${TEXT_RED}remove ios/curl-*${TEXT_RESET}"
        rm -rf ios/curl-*

        echo -e "${TEXT_RED}remove ios/libxml2-*${TEXT_RESET}"
        rm -rf ios/libxml2-*

        echo -e "${TEXT_RED}remove ios/libiconv-*${TEXT_RESET}"
        rm -rf ios/libiconv-*

        echo -e "${TEXT_RED}remove android/contrib/libz-*${TEXT_RESET}"
        rm -rf android/contrib/libz-*
    else
        echo -e "${TEXT_RED} BUILD_TARGET $BUILD_TARGET unknown,pls check...${TEXT_RESET}"
    fi
}


function do_init()
{
    check_host
    check_sources  "libiconv"
    check_sources  "libz"
    check_sources  "libxml2"
    check_sources  "curl"
    check_sources  "openssl"
    check_sources  "ffmpeg"
}


function do_build()
{
    cd ${PROJECT_ROOT_PATH}

    for arch in ${BUILD_ARCHS};do
        if [ ${arch} == "arm64-v8a" ] || [ ${arch} == "arm64_v8a" ]; then
            realname="arm64"
        elif [ ${arch} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${arch}
        fi
        module_names="ffmpeg openssl"
        for module_name in ${module_names};do
            if [ ${BUILD_TARGET} == "android" ]; then
                test_path=./${BUILD_TARGET}/contrib/${module_name}-${realname}
            elif [ ${BUILD_TARGET} == "ios" ]; then
                test_path=./${BUILD_TARGET}/${module_name}-${realname}
            else
                echo -e "${TEXT_RED}${BUILD_TARGET} unknown,pls check...${TEXT_RESET}"
                exit 1
            fi

            if [ ! -d ${test_path} ]; then
                echo_left_align "${TEXT_RED}${test_path}${TEXT_RESET}" " not exist"
                echo -e "${TEXT_RED}$0 init mightbe required firstly${TEXT_RESET}"
                exit 1
            else
                echo "test_path:$test_path exist"
            fi
        done
    done

    if [ ${BUILD_TYPE} = "release" ];then
        build_native_release
    elif [ ${BUILD_TYPE} = "debug" ];then
        build_native_debug
    else
        echo -e "${TEXT_RED} $BUILD_TYPE unknown,pls check${TEXT_RESET}"
        exit 1
    fi

    cd ${PROJECT_ROOT_PATH}
    show_pwd
    if [ $BUILD_TARGET == "android" ]; then
        echo -e "${TEXT_BLUE} all dependent android native libs and JNI libs build finished, pls build ijkplayer APK via latest Andriod Studio IDE ${TEXT_RESET}"
    elif [ $BUILD_TARGET == "ios" ]; then
        echo -e "${TEXT_BLUE} all dependent ios native libs build finished, pls build IJKMediaDemo  via latest Xcode IDE ${TEXT_RESET}"
    fi
}



function main()
{
case "$user_command" in
    clean)
        do_clean
    ;;
    cleanall)
        do_cleanall
    ;;
    init)
        do_init
    ;;
    build)
        do_build
    ;;
    *)
        dump_usage
        exit 1
    ;;
esac
}


user_command=$1

setup_env
check_host

main
