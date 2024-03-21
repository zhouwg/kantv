#!/usr/bin/env bash

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


echo "build brotli for target ${BUILD_TARGET}"

./bootstrap
emconfigure ./configure --prefix=${FF_PREFIX}/  --disable-shared --enable-static
emmake make clean
make -j${HOST_CPU_COUNTS}
#weiguo:make curl-wasm happy
emmake make install

if [ ! -d  ${FF_PREFIX}/lib ]; then
    mkdir -p ${FF_PREFIX}/lib
fi
/bin/cp -fv  .libs/libbrotlicommon.a ${FF_PREFIX}/lib/
/bin/cp -fv  .libs/libbrotlidec.a ${FF_PREFIX}/lib/
