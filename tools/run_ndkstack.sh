#!/usr/bin/env bash

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)
if [ $? != 0 ]; then
    echo -e "pls running source build/envsetup.sh firstly\n"
    #exit 1
    return 0
fi


ndk-stack -sym ${PROJECT_ROOT_PATH}/cdeosplayer/kantv/build/intermediates/cmake/all64Debug/obj/arm64-v8a/ -dump crash.log

