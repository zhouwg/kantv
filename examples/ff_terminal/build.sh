#!/usr/bin/env bash

set -e

TARGET=ff-terminal
BUILD_TYPE=Release

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd

function build_x86
{
cmake -H. -B./out/x86 -DCMAKE_BUILD_TYPE=${PROJECT_BUILD_TYPE} -DPREFIX=${FF_PREFIX} -DCMAKE_INSTALL_PREFIX=${FF_PREFIX}
cd ./out/x86
make
make install

cd -
}


build_x86
