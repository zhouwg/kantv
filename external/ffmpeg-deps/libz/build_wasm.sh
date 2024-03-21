#!/usr/bin/env bash

TARGET=libz.a

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

if [ "x${FF_PREFIX}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "FF_PREFIX is empty, pls check"
    exit 1
fi


echo "build ${TARGET} for target ${BUILD_TARGET}"

emconfigure ./configure --prefix=${FF_PREFIX}/  --static
emmake make clean
make -j${HOST_CPU_COUNTS}
#weiguo:make curl-wasm happy
emmake make install

if [ ! -d  ${FF_PREFIX}/lib ]; then
    mkdir -p ${FF_PREFIX}/lib
fi
/bin/cp -fv ${TARGET} ${FF_PREFIX}/lib/
