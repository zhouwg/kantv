#!/usr/bin/env bash

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)
if [ $? != 0 ]; then
    echo -e "pls running source build/envsetup.sh firstly\n"
    exit 1
fi


ndk-stack -sym ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/build/intermediates/cmake/all64Debug/obj/arm64-v8a/ -dump crash.log

