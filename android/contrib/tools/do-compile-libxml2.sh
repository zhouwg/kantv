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

#--------------------
set -e

if [ -z "$ANDROID_NDK" ]; then
    echo "You must define ANDROID_NDK before starting."
    echo "They must point to your NDK directories.\n"
    exit 1
fi

#--------------------
# common defines
FF_ARCH=$1
if [ -z "$FF_ARCH" ]; then
    echo "You must specific an architecture 'arm, armv7a, x86, ...'.\n"
    exit 1
fi


FF_BUILD_ROOT=`pwd`

FF_BUILD_NAME=
FF_SOURCE=
FF_CROSS_PREFIX=

FF_DEP_LIBICONV_INC=
FF_DEP_LIBICONV_LIB=

FF_DEP_LIBZ_INC=
FF_DEP_LIBZ_LIB=

FF_CFG_FLAGS=
FF_PLATFORM_CFG_FLAGS=

FF_EXTRA_CFLAGS=
FF_EXTRA_LDFLAGS="-lz"

FF_CMAKE_ABI=
FF_CMAKE_EXTRA_FLAGS=

#----- armv7a begin -----
if [ "$FF_ARCH" = "armv7a" ]; then
    FF_BUILD_NAME=libxml2-armv7a
    FF_BUILD_NAME_LIBICONV=libiconv-armv7a
    FF_BUILD_NAME_LIBZ=libz-armv7a
    FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

    FF_CMAKE_ABI="armeabi-v7a with NEON"
    FF_CMAKE_EXTRA_FLAGS="-DHAVE_WORDS_BIGENDIAN_EXITCODE=1 -DWITH_SIMD=0"

    FF_CROSS_PREFIX=arm-linux-androideabi
	FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_VER}

elif [ "$FF_ARCH" = "armv5" ]; then
    FF_BUILD_NAME=libxml2-armv5
    FF_BUILD_NAME_LIBICONV=libiconv-armv5
    FF_BUILD_NAME_LIBZ=libz-armv5
    FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

    FF_CMAKE_ABI="armeabi"
    FF_CMAKE_EXTRA_FLAGS="-DHAVE_WORDS_BIGENDIAN_EXITCODE=1 -DWITH_SIMD=0"

    FF_CROSS_PREFIX=arm-linux-androideabi
	FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_VER}

elif [ "$FF_ARCH" = "x86" ]; then
    FF_BUILD_NAME=libxml2-x86
    FF_BUILD_NAME_LIBICONV=libiconv-x86
    FF_BUILD_NAME_LIBZ=libz-x86
    FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

    FF_CMAKE_ABI="x86"
    FF_CMAKE_EXTRA_FLAGS="-DHAVE_WORDS_BIGENDIAN_EXITCODE=1"

    FF_CROSS_PREFIX=i686-linux-android
	FF_TOOLCHAIN_NAME=x86-${FF_GCC_VER}
    FF_PLATFORM_CFG_FLAGS="linux-aarch64"

elif [ "$FF_ARCH" = "x86_64" ]; then
    FF_ANDROID_PLATFORM=android-21

    FF_BUILD_NAME=libxml2-x86_64
    FF_BUILD_NAME_LIBICONV=libiconv-x86_64
    FF_BUILD_NAME_LIBZ=libz-x86_64
    FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

    FF_CMAKE_ABI="x86_64"

    FF_CROSS_PREFIX=x86_64-linux-android
    FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_64_VER}
    #can't work
    FF_PLATFORM_CFG_FLAGS="android-x86_64"
    FF_PLATFORM_CFG_FLAGS="linux-x86_64"


elif [ "$FF_ARCH" = "arm64" ]; then
    FF_ANDROID_PLATFORM=android-21

    FF_BUILD_NAME=libxml2-arm64
    FF_BUILD_NAME_LIBICONV=libiconv-arm64
    FF_BUILD_NAME_LIBZ=libz-arm64
    FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

    FF_CMAKE_ABI="arm64-v8a"
    FF_CROSS_PREFIX=aarch64-linux-android
    FF_TOOLCHAIN_NAME=${FF_CROSS_PREFIX}-${FF_GCC_64_VER}
    FF_PLATFORM_CFG_FLAGS="linux-aarch64"

else
    echo "unknown architecture $FF_ARCH";
    exit 1
fi

FF_SYSROOT=$FF_TOOLCHAIN_PATH/sysroot
FF_PREFIX=$FF_BUILD_ROOT/build/$FF_BUILD_NAME/output
FF_DEP_LIBICONV_INC=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBICONV/output/include
FF_DEP_LIBICONV_LIB=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBICONV/output/lib

FF_DEP_LIBZ_INC=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBZ/output/include
FF_DEP_LIBZ_LIB=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBZ/output/lib

mkdir -p $FF_PREFIX


#--------------------
echo ""
echo "--------------------"
echo "[*] make NDK standalone toolchain"
echo "--------------------"
. ./tools/do-detect-env.sh
FF_MAKE_TOOLCHAIN_FLAGS=$IJK_MAKE_TOOLCHAIN_FLAGS
FF_MAKE_FLAGS=$IJK_MAKE_FLAG
FF_GCC_VER=$IJK_GCC_VER
FF_GCC_64_VER=$IJK_GCC_64_VER

FF_TOOLCHAIN_PATH=$FF_BUILD_ROOT/build/$FF_BUILD_NAME/toolchain

FF_MAKE_TOOLCHAIN_FLAGS="$FF_MAKE_TOOLCHAIN_FLAGS --install-dir=$FF_TOOLCHAIN_PATH"
FF_TOOLCHAIN_TOUCH="$FF_TOOLCHAIN_PATH/touch"
if [ ! -f "$FF_TOOLCHAIN_TOUCH" ]; then
    $ANDROID_NDK/build/tools/make-standalone-toolchain.sh \
        $FF_MAKE_TOOLCHAIN_FLAGS \
        --platform=$FF_ANDROID_PLATFORM \
        --toolchain=$FF_TOOLCHAIN_NAME
    touch $FF_TOOLCHAIN_TOUCH;
fi

export PATH=$FF_TOOLCHAIN_PATH/bin:$PATH

export COMMON_FF_CFG_FLAGS=

FF_CFG_FLAGS="$FF_CFG_FLAGS $COMMON_FF_CFG_FLAGS"

#--------------------
# Standard options:
FF_CFG_FLAGS="$FF_CFG_FLAGS --prefix=$FF_PREFIX"
FF_CFG_FLAGS="$FF_CFG_FLAGS --without-python"
FF_CFG_FLAGS="$FF_CFG_FLAGS --without-lzma"
FF_CFG_FLAGS="$FF_CFG_FLAGS --enable-static"
FF_CFG_FLAGS="$FF_CFG_FLAGS --with-iconv=$FF_DEP_LIBICONV_INC/../"
FF_CFG_FLAGS="$FF_CFG_FLAGS --with-zlib=$FF_DEP_LIBZ_INC/../"
FF_CFG_FLAGS="$FF_CFG_FLAGS --host=${FF_CROSS_PREFIX}"
#--------------------


echo ""
echo "--------------------"
echo "[*] configurate libxml2"
echo "--------------------"
cd $FF_CMAKE_BUILD_DIR
echo "current working path:$(pwd)"
FF_CMAKE_CFG_FLAGS="-DCMAKE_TOOLCHAIN_FILE=${FF_SOURCE}/android.toolchain.cmake -DANDROID_NDK=$ANDROID_NDK -DBUILD_EXAMPLES=0 -DBUILD_LSR_TESTS=0 -DLIBXML2_WITH_LZMA=0 -DLIBXML2_WITH_PYTHON=0 -DBUILD_SHARED_LIBS=0 -DBUILD_TESTS=0 -DCMAKE_BUILD_TYPE=Release -DWITH_LSR_BINDINGS=0 -DWITH_OPENMP=0 -DWITH_PFFFT=0"
echo "cmake $FF_CMAKE_CFG_FLAGS -DANDROID_ABI=$FF_CMAKE_ABI -DCMAKE_PREFIX_PATH=$FF_PREFIX -DCMAKE_INSTALL_PREFIX=$FF_PREFIX -B $FF_CMAKE_BUILD_DIR"
#cmake $FF_CMAKE_CFG_FLAGS $FF_CMAKE_EXTRA_FLAGS -DANDROID_ABI=$FF_CMAKE_ABI -DCMAKE_PREFIX_PATH=$FF_PREFIX -DCMAKE_INSTALL_PREFIX=$FF_PREFIX $FF_SOURCE
echo "--------------------"
cd $FF_SOURCE
echo "current working path:$(pwd)"
if [ ! -f configure ];then
    ./autogen.sh
fi
./configure $FF_CFG_FLAGS $FF_EXTRA_CFLAGS


#--------------------
echo ""
echo "--------------------"
echo "[*] compile libxml2"
echo "--------------------"
CPU_COUNTS=`cat /proc/cpuinfo | grep "processor" | wc | awk '{print int($1)}'`
echo "make -j${CPU_COUNTS}"
cd $FF_SOURCE
echo -e "current working path:$(pwd)\n"
make -j${CPU_COUNTS}
make install
