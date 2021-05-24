#! /usr/bin/env bash
#
# Copyright (C) 2021- Zhou Weiguo <zhouwg2000@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#----------
FF_ALL_ARCHS_IOS8_SDK="armv7 arm64 i386 x86_64"

FF_ALL_ARCHS=$FF_ALL_ARCHS_IOS8_SDK

#----------
UNI_BUILD_ROOT=`pwd`
UNI_TMP="$UNI_BUILD_ROOT/tmp"
UNI_TMP_LLVM_VER_FILE="$UNI_TMP/llvm.ver.txt"
FF_TARGET=$1
set -e

#----------
FF_LIBS="libxml2"

#----------
echo_archs() {
    echo "===================="
    echo "[*] check xcode version"
    echo "===================="
    echo "FF_ALL_ARCHS = $FF_ALL_ARCHS"
}

do_lipo () {
    LIB_FILE=$1
    LIPO_FLAGS=
    for ARCH in $FF_ALL_ARCHS
    do
        LIPO_FLAGS="$LIPO_FLAGS $UNI_BUILD_ROOT/build/libxml2-$ARCH/output/lib/$LIB_FILE"
    done

    xcrun lipo -create $LIPO_FLAGS -output $UNI_BUILD_ROOT/build/universal/lib/$LIB_FILE
    xcrun lipo -info $UNI_BUILD_ROOT/build/universal/lib/$LIB_FILE
}

do_lipo_all () {
    mkdir -p $UNI_BUILD_ROOT/build/universal/lib
    echo "lipo archs: $FF_ALL_ARCHS"
    for FF_LIB in $FF_LIBS
    do
        do_lipo "$FF_LIB.a";
    done

    cp -R $UNI_BUILD_ROOT/build/libxml2-x86_64/output/include $UNI_BUILD_ROOT/build/universal/
}

#----------
if [ "$FF_TARGET" = "armv7" -o "$FF_TARGET" = "armv7s" -o "$FF_TARGET" = "arm64" ]; then
    echo_archs
    sh tools/do-compile-libxml2.sh $FF_TARGET
    do_lipo_all
elif [ "$FF_TARGET" = "i386" -o "$FF_TARGET" = "x86_64" ]; then
    echo_archs
    sh tools/do-compile-libxml2.sh $FF_TARGET
    do_lipo_all
elif [ "$FF_TARGET" = "lipo" ]; then
    echo_archs
    do_lipo_all
elif [ "$FF_TARGET" = "all" ]; then
    echo_archs
    for ARCH in $FF_ALL_ARCHS
    do
        sh tools/do-compile-libxml2.sh $ARCH
    done

    do_lipo_all
elif [ "$FF_TARGET" = "check" ]; then
    echo_archs
elif [ "$FF_TARGET" = "clean" ]; then
    echo_archs
    for ARCH in $FF_ALL_ARCHS
    do
        cd libxml2-$ARCH && git clean -xdf && cd -
    done
else
    echo "Usage:"
    echo "  compile-libxml2.sh armv7|arm64|i386|x86_64"
    echo "  compile-libxml2.sh lipo"
    echo "  compile-libxml2.sh all"
    echo "  compile-libxml2.sh clean"
    echo "  compile-libxml2.sh check"
    exit 1
fi
