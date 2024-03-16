#!/usr/bin/env bash

set -e

TARGET=bench
BUILD_TYPE=Release

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd
check_upstream_whispercpp
check_local_whispercpp

if [ "${PROJECT_BUILD_TYPE}" == "release" ]; then
    BUILD_TYPE=Release
fi

if [ "${PROJECT_BUILD_TYPE}" == "debug" ]; then
    BUILD_TYPE=Debug
fi

echo -e  "upstream whispercpp path: ${UPSTREAM_WHISPERCPP_PATH}\n"
echo -e  "local    whispercpp path: ${LOCAL_WHISPERCPP_PATH}\n"
echo -e  "build               type: ${BUILD_TYPE}"

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi


function build_x86
{
cmake -H. -B./out/x86 -DPROJECT_ROOT_PATH=${PROJECT_ROOT_PATH} -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DBUILD_TARGET="x86" -DWHISPERCPP_PATH=${LOCAL_WHISPERCPP_PATH}
cd ./out/x86
make

ls -l ${TARGET}
ls -lah ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}
cd -
}

build_x86
