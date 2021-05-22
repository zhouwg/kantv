#!/bin/bash

# prepare ffmpeg and openssl codes and build all native libs and Android JNI libs automatically
# validate ok on Ubuntu 20.04.2 x86_64 LTS
# validate ok on macOS Catalina(10.15.7) x86_64

# zhou.weiguo, 2021-05-12,16:19
# zhou.weiguo, 2021-05-22,17:26

export TEXT_BLACK=" \033[30m "
export TEXT_RED="   \033[31m "
export TEXT_GREEN=" \033[32m "
export TEXT_BLUE="  \033[34m "
export TEXT_BLACK=" \033[35m "
export TEXT_WHITE=" \033[37m "
export TEXT_RESET=" \033[0m  "

export build_user=$(whoami)
export build_time=`date +"%Y-%m-%d,%H:%M:%S"`
#export build_target=android
export build_target=ios
export prj_root_path=$(pwd)
export home_path=`env | grep ^HOME= | cut -c 6-`

#modify following variable accordingly
#ndk-r14b must be used in this project but I guess clang mightbe ok
export ANDROID_NDK=/opt/android-ndk-r14b

#default release build
export build_type=release
#some files must be modified according to android/patch-debugging-with-lldb.sh for debug build
#becareful there some conflicts between release build and debug build for final Android apk
#export build_type=debug

#ARCHs=(armv5 armeabi-v7a arm64-v8a x86 x86_64)
#TODO: target "armv5 armeabi-v7a x86" not working with OpenSSL_1_1_1-stable
ARCHs=(arm64-v8a x86_64)


function show_pwd() 
{
    echo -e "current working path:$(pwd)\n"
}


function check_ndk()
{
    if [ ! -d ${ANDROID_NDK} ]; then
        echo -e "${TEXT_RED}${ANDROID_NDK} not exist, pls check...${TEXT_RESET}\n"
        exit 1
    fi
}


function check_target()
{
    if [ $build_target == "android" ]; then
        echo -e "${TEXT_BLUE}build_target is $build_target${TEXT_RESET}"
    elif [ $build_target == "ios" ]; then
        echo -e "${TEXT_BLUE}build_target is $build_target${TEXT_RESET}"
    else
        echo -e "${TEXT_RED}${build_target} unknown,pls check...${TEXT_RESET}"
        exit 1
    fi
}


function dump_envs()
{
    echo -e "\n"

    echo "build_user:    $build_user"
    echo "build_time:    $build_time"
    echo "build_type:    $build_type"
    echo "build_target:  $build_target"
    echo "home_path:     $home_path"
    echo "prj_root_path: $prj_root_path"
    echo "ANDROID_NDK:   $ANDROID_NDK"

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
    #TODO: should I check input args here?

    cd ${prj_root_path}
    echo "======enter function check sources of $module_name ======"

    #it should be local variable in a standalone function
    #but be a global variable might be a better idea because the same variable 
    #would be defined in other place
    #ARCHs=(armv5 armeabi-v7a arm64-v8a x86 x86_64)

    notexist=0
    for index in ${ARCHs[@]};do
        if [ ${index} == "arm64-v8a" ]; then
            realname="arm64"
        elif [ ${index} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${index}
        fi
        if [ $build_target == "android" ]; then
            test_path=./$build_target/contrib/${module_name}-${realname}
        elif [ $build_target == "ios" ]; then
            test_path=./$build_target/${module_name}-${realname}
        else
            echo -e "${TEXT_RED}${build_target} unknown,pls check...${TEXT_RESET}"
            exit 1
        fi

        if [ ! -d $test_path ]; then
            echo_left_align "$test_path" "not exist"
            let notexist++
        else
            echo_left_align "$test_path" "exist"
        fi
        let index++
    done

    if [ $notexist -eq 0 ];then
        echo_left_align "${module_name} source directory" "exist"
    else
        echo -e "${TEXT_RED}${module_name} source directory not exist${TEXT_RESET}"

        if [ $module_name == "ffmpeg" ]; then
            echo "prepare $module_name codes..."
	    if [ $build_target == "android" ]; then
            	./init-android.sh
            elif [ $build_target == "ios" ]; then
                ./init-ios.sh
            else
        	echo -e "${TEXT_RED}${build_target} unknown,pls check...${TEXT_RESET}"
            fi
           
        elif [ $module_name == "openssl" ]; then
            echo "prepare $module_name codes..."
	    if [ $build_target == "android" ]; then
            	./init-android-openssl.sh
            elif [ $build_target == "ios" ]; then
            	./init-ios-openssl.sh
            else
        	echo -e "${TEXT_RED}${build_target} unknown,pls check...${TEXT_RESET}"
            fi
        else
            echo "module_name $module_name unknown, pls check why?"
        fi
    fi

    cd ${prj_root_path}
    echo "======leave function check sources of $module_name ======"
    echo -e "\n"
}


function build_native_debug()
{
    set -e
    cd ${prj_root_path}
    show_pwd
    echo "build JNI..."

    if [ $build_target != "android" ]; then
        #focus on Android debug build at the moment
        echo -e "${TEXT_RED}do nothing because build_target is $build_target, not android ...${TEXT_RESET}"
        return;
    fi

    for index in ${ARCHs[@]};do
        echo "arch = ${index}"
        if [ ${index} == "arm64-v8a" ]; then
            realname="arm64"
        elif [ ${index} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${index}
        fi

        if [ ! -d ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jni ]; then
            echo "${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jni not exist"
            mkdir -p ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jni
        else
            echo "${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jni already exist"
        fi
        cd ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jni

        show_pwd
        ${ANDROID_NDK}/ndk-build APP_BUILD_SCRIPT=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Android.mk NDK_APPLICATION_MK=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Application.mk APP_ABI=${index} NDK_ALL_ABIS=${index} NDK_DEBUG=1 APP_PLATFORM=android-21 NDK_OUT=${prj_root_path}/android/ijkplayer/ijkplayer-${realnaem}/build/intermediates/ndkBuild/debug/obj NDK_LIBS_OUT=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib NDK_LOG=1 V=1 clean
        ${ANDROID_NDK}/ndk-build APP_BUILD_SCRIPT=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Android.mk NDK_APPLICATION_MK=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/jni/Application.mk APP_ABI=${index} NDK_ALL_ABIS=${index} NDK_DEBUG=1 APP_PLATFORM=android-21 NDK_OUT=${prj_root_path}/android/ijkplayer/ijkplayer-${realnaem}/build/intermediates/ndkBuild/debug/obj NDK_LIBS_OUT=${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib NDK_LOG=1 V=1

        if [ ! -d ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} ]; then
            echo -e "${TEXT_RED}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} not exist${TEXT_RESET}"
            mkdir -p ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}
        else
            echo -e "${TEXT_BLUE}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} already exist${TEXT_RESET}"
        fi
        echo -e "/bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${index}/libijksdl.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}"
        /bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${index}/libijksdl.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}


        echo -e "/bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${index}/libijkplayer.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}"
        /bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/build/intermediates/ndkBuild/debug/lib/${index}/libijkplayer.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}

        let index++
    done
    set +e
}


function build_native_release()
{
    cd ${prj_root_path}
    show_pwd
    echo "build JNI..."

    for index in ${ARCHs[@]};do
        echo "arch = ${index}"
        if [ ${index} == "arm64-v8a" ]; then
            realname="arm64"
        elif [ ${index} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${index}
        fi

    	if [ $build_target == "android" ]; then
            cd ${prj_root_path}/android/contrib
    	elif [ $build_target == "ios" ]; then
            cd ${prj_root_path}/ios
        else
            echo -e "${TEXT_RED} build_target $build_target unknown,pls check...${TEXT_RESET}"
            continue;
        fi
        show_pwd
        echo "build openssl..."
        ./compile-openssl.sh $realname
        if [ $? != 0 ]; then
            echo -e "${TEXT_RED} build openssl failed ${TEXT_RESET}"
            exit 1
        else
            echo -e "${TEXT_BLUE} build openssl successed ${TEXT_RESET}"
        fi

        echo "build ffmpeg..."
        ./compile-ffmpeg.sh  $realname
        if [ $? != 0 ]; then
            echo -e "${TEXT_RED}build ffmpeg failed ${TEXT_RESET}"
            exit 1
        else
            echo -e "${TEXT_BLUE} build ffmpeg successed ${TEXT_RESET}"
        fi

    	if [ $build_target != "android" ]; then
            #TODO: build ijksdl and ijkplayer in this script for ios
            return;
	fi

        echo "build ijksdl.so ijkplayer.so..."
        cd ${prj_root_path}/android
        show_pwd
        ./compile-ijk.sh $realname
        if [ $? != 0 ]; then
            echo -e "${TEXT_RED}build ijkplayer.so ijksdl.so failed ${TEXT_RESET}"
            exit 1
        else
            echo -e "${TEXT_BlUE}build ijkplayer.so ijksdl.so successed ${TEXT_RESET}"
        fi


        echo -e "\n"
        if [ ! -d ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} ]; then
            echo -e "${TEXT_RED}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} not exist${TEXT_RESET}"
            mkdir -p ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}
        else
            echo -e "${TEXT_BLUE}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index} already exist${TEXT_RESET}"
        fi
        cd ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs
        show_pwd
        ls -l ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${index}/libijk*.so
        echo "/bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${index}/libijk*.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}"
        /bin/cp -fv ${prj_root_path}/android/ijkplayer/ijkplayer-${realname}/src/main/obj/local/${index}/libijk*.so  ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs/${index}
        let index++
    done

    echo -e "\n"
}


function do_clean()
{
    cd ${prj_root_path}
    show_pwd

    if [ $build_target == "android" ]; then
	    echo -e "${TEXT_RED}remove android/contrib/build${TEXT_RESET}"
	    if [ -d android/contrib/build ]; then
		rm -rf android/contrib/build
	    fi

	    if [ ! -d ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs ]; then
		echo -e "${TEXT_RED}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs not exist${TEXT_RESET}\n"
	    else
		echo -e "${TEXT_BLUE}${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs exist${TEXT_RESET}, remove it\n"
		rm -rf ${prj_root_path}/android/ijkplayer/ijkplayer-example/src/main/jniLibs
	    fi
    elif [ $build_target == "ios" ]; then
	    echo -e "${TEXT_RED}remove ios/build${TEXT_RESET}"
	    if [ -d ios/build ]; then
		rm -rf ios/build
	    fi
    fi

    for index in ${ARCHs[@]};do
        echo "arch = ${index}"
        if [ ${index} == "arm64-v8a" ]; then
            realname="arm64"
        elif [ ${index} == "armeabi-v7a" ]; then
            realname="armv7a"
        else
            realname=${index}
        fi

    	if [ $build_target == "android" ]; then
            cd ${prj_root_path}/android/contrib
    	elif [ $build_target == "ios" ]; then
            cd ${prj_root_path}/ios
        else
            echo -e "${TEXT_RED} build_target $build_target unknown,pls check...${TEXT_RESET}"
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

    cd ${prj_root_path}
    show_pwd

    if [ $build_target == "android" ]; then
        echo -e "${TEXT_RED}remove android/contrib/ffmpeg-*${TEXT_RESET}"
        rm -rf android/contrib/ffmpeg-*

        echo -e "${TEXT_RED}remove android/contrib/openssl-*${TEXT_RESET}"
        rm -rf android/contrib/openssl-*
    elif [ $build_target == "ios" ]; then
        echo -e "${TEXT_RED}remove ios/ffmpeg-*${TEXT_RESET}"
        rm -rf ios/ffmpeg-*

        echo -e "${TEXT_RED}remove ios/openssl-*${TEXT_RESET}"
        rm -rf ios/openssl-*
    else
        echo -e "${TEXT_RED} build_target $build_target unknown,pls check...${TEXT_RESET}"
    fi
}


function do_init()
{
    dump_envs
    check_ndk
    check_target
    check_sources  "openssl"
    check_sources  "ffmpeg"
}


function do_build()
{
    cd ${prj_root_path}
    if [ $build_target == "android" ]; then
        if  [ ! -d android/contrib/ffmpeg-x86_64 ] || [ ! -d android/contrib/ffmpeg-arm64 ];then
	    echo -e "${TEXT_RED}$0 init mightbe required firstly${TEXT_RESET}"
	    exit 1
	fi

	#if [ ! -d android/contrib/ffmpeg-x86 ] || [ ! -d android/contrib/ffmpeg-armv7a ];then
	#    echo -e "${TEXT_RED}$0 init mightbe required firstly${TEXT_RESET}"
	#    exit 1
	#fi
    elif [ $build_target == "ios" ]; then
	if  [ ! -d ios/ffmpeg-x86_64 ] || [ ! -d ios/ffmpeg-arm64 ];then
	    echo -e "${TEXT_RED}$0 init mightbe required firstly${TEXT_RESET}"
	    exit 1
	fi
    else
        echo -e "${TEXT_RED} build_target $build_target unknown,pls check...${TEXT_RESET}"
        exit 1
    fi

    if [ ${build_type} = "release" ];then
        build_native_release
    elif [ ${build_type} = "debug" ];then
        build_native_debug
    else
        echo -e "${TEXT_RED} $build_type unknown,pls check${TEXT_RESET}"
        exit 1
    fi

    cd ${prj_root_path}
    show_pwd
    if [ $build_target == "android" ]; then
    	echo -e "${TEXT_BLUE} all dependent android native libs and JNI libs build finished, pls build ijkplayer APK via latest Andriod Studio IDE ${TEXT_RESET}"
    elif [ $build_target == "ios" ]; then
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

dump_envs

main
