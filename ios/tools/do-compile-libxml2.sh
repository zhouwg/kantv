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
echo "===================="
echo "[*] check host"
echo "===================="
set -e


FF_XCRUN_DEVELOPER=`xcode-select -print-path`
if [ ! -d "$FF_XCRUN_DEVELOPER" ]; then
  echo "xcode path is not set correctly $FF_XCRUN_DEVELOPER does not exist (most likely because of xcode > 4.3)"
  echo "run"
  echo "sudo xcode-select -switch <xcode path>"
  echo "for default installation:"
  echo "sudo xcode-select -switch /Applications/Xcode.app/Contents/Developer"
  exit 1
fi

case $FF_XCRUN_DEVELOPER in  
     *\ * )
           echo "Your Xcode path contains whitespaces, which is not supported."
           exit 1
          ;;
esac


#--------------------
# include


#--------------------
# common defines
FF_ARCH=$1
if [ -z "$FF_ARCH" ]; then
    echo "You must specific an architecture 'armv7, armv7s, arm64, i386, x86_64, ...'.\n"
    exit 1
fi


FF_BUILD_ROOT=`pwd`
FF_TAGET_OS="darwin"


# libxml2 build params
export COMMON_FF_CFG_FLAGS=

LIBXML2_CFG_FLAGS=
LIBXML2_EXTRA_CFLAGS=
LIBXML2_CFG_CPU=

# i386, x86_64
LIBXML2_CFG_FLAGS_SIMULATOR=

# armv7, armv7s, arm64
LIBXML2_CFG_FLAGS_ARM=
LIBXML2_CFG_FLAGS_ARM="iphoneos-cross"

echo "build_root: $FF_BUILD_ROOT"

#--------------------
echo "===================="
echo "[*] config arch $FF_ARCH"
echo "===================="

FF_BUILD_NAME="unknown"
FF_XCRUN_PLATFORM="iPhoneOS"
FF_XCRUN_OSVERSION=
FF_GASPP_EXPORT=
FF_XCODE_BITCODE=

if [ "$FF_ARCH" = "i386" ]; then
    FF_BUILD_NAME="libxml2-i386"
    FF_BUILD_NAME_LIBICONV=libiconv-i386
    FF_BUILD_NAME_LIBZ=libz-i386
    FF_XCRUN_PLATFORM="iPhoneSimulator"
    FF_XCRUN_OSVERSION="-mios-simulator-version-min=6.0"
    LIBXML2_CFG_FLAGS="darwin-i386-cc $LIBXML2_CFG_FLAGS"
    FF_CMAKE_ABI="x86"
elif [ "$FF_ARCH" = "x86_64" ]; then
    FF_BUILD_NAME="libxml2-x86_64"
    FF_BUILD_NAME_LIBICONV=libiconv-x86_64
    FF_BUILD_NAME_LIBZ=libz-x86_64
    FF_XCRUN_PLATFORM="iPhoneSimulator"
    FF_XCRUN_OSVERSION="-mios-simulator-version-min=7.0"
    LIBXML2_CFG_FLAGS="darwin64-x86_64-cc $LIBXML2_CFG_FLAGS"
    FF_CMAKE_ABI="x86_64"
elif [ "$FF_ARCH" = "armv7" ]; then
    FF_BUILD_NAME="libxml2-armv7"
    FF_BUILD_NAME_LIBICONV=libiconv-armv7
    FF_BUILD_NAME_LIBZ=libz-armv7
    FF_XCRUN_OSVERSION="-miphoneos-version-min=6.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    LIBXML2_CFG_FLAGS="$LIBXML2_CFG_FLAGS_ARM $LIBXML2_CFG_FLAGS"
    FF_CMAKE_ABI="armeabi-v7a with NEON"
#    LIBXML2_CFG_CPU="--cpu=cortex-a8"
elif [ "$FF_ARCH" = "armv7s" ]; then
    FF_BUILD_NAME="libxml2-armv7s"
    FF_BUILD_NAME_LIBICONV=libiconv-armv7s
    FF_BUILD_NAME_LIBZ=libz-armv7s
    LIBXML2_CFG_CPU="--cpu=swift"
    FF_XCRUN_OSVERSION="-miphoneos-version-min=6.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    LIBXML2_CFG_FLAGS="$LIBXML2_CFG_FLAGS_ARM $LIBXML2_CFG_FLAGS"
    FF_CMAKE_ABI="armeabi"
elif [ "$FF_ARCH" = "arm64" ]; then
    FF_BUILD_NAME="libxml2-arm64"
    FF_BUILD_NAME_LIBICONV=libiconv-arm64
    FF_BUILD_NAME_LIBZ=libz-arm64
    FF_XCRUN_OSVERSION="-miphoneos-version-min=7.0"
    FF_XCODE_BITCODE="-fembed-bitcode"
    LIBXML2_CFG_FLAGS="$LIBXML2_CFG_FLAGS_ARM $LIBXML2_CFG_FLAGS"
    FF_GASPP_EXPORT="GASPP_FIX_XCODE5=1"
    FF_CMAKE_ABI="arm64-v8a"
else
    echo "unknown architecture $FF_ARCH";
    exit 1
fi

echo "build_name: $FF_BUILD_NAME"
echo "platform:   $FF_XCRUN_PLATFORM"
echo "osversion:  $FF_XCRUN_OSVERSION"

#--------------------
echo "===================="
echo "[*] make ios toolchain $FF_BUILD_NAME"
echo "===================="


FF_BUILD_SOURCE="$FF_BUILD_ROOT/$FF_BUILD_NAME"
FF_BUILD_PREFIX="$FF_BUILD_ROOT/build/$FF_BUILD_NAME/output"

FF_DEP_LIBICONV_INC=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBICONV/output/include
FF_DEP_LIBICONV_LIB=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBICONV/output/lib

FF_DEP_LIBZ_INC=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBZ/output/include
FF_DEP_LIBZ_LIB=$FF_BUILD_ROOT/build/$FF_BUILD_NAME_LIBZ/output/lib
FF_SOURCE=$FF_BUILD_ROOT/$FF_BUILD_NAME

mkdir -p $FF_BUILD_PREFIX


FF_XCRUN_SDK=`echo $FF_XCRUN_PLATFORM | tr '[:upper:]' '[:lower:]'`
FF_XCRUN_SDK_PLATFORM_PATH=`xcrun -sdk $FF_XCRUN_SDK --show-sdk-platform-path`
FF_XCRUN_SDK_PATH=`xcrun -sdk $FF_XCRUN_SDK --show-sdk-path`
FF_XCRUN_CC="xcrun -sdk $FF_XCRUN_SDK clang"

export CROSS_TOP="$FF_XCRUN_SDK_PLATFORM_PATH/Developer"
export CROSS_SDK=`echo ${FF_XCRUN_SDK_PATH/#$CROSS_TOP\/SDKs\//}`
export BUILD_TOOL="$FF_XCRUN_DEVELOPER"
export CC="$FF_XCRUN_CC -arch $FF_ARCH $FF_XCRUN_OSVERSION $FF_XCODE_BITCODE"

echo "build_source: $FF_BUILD_SOURCE"
echo "build_prefix: $FF_BUILD_PREFIX"
echo "CROSS_TOP: $CROSS_TOP"
echo "CROSS_SDK: $CROSS_SDK"
echo "BUILD_TOOL: $BUILD_TOOL"
echo "CC: $CC"


#--------------------
echo "\n--------------------"
echo "[*] configurate libxml2"
echo "--------------------"


if [ "$FF_ARCH" = "arm64" ]; then
    LIBXML2_CFG_FLAGS="--prefix=$FF_BUILD_PREFIX --enable-static --enable-shared --without-python --without-lzma --with-iconv=$FF_DEP_LIBICONV_INC/../ --with-zlib=$FF_DEP_LIBZ_INC/../  --host=armv8"
else
    LIBXML2_CFG_FLAGS="--prefix=$FF_BUILD_PREFIX --enable-static --enable-shared --without-python --without-lzma --with-iconv=$FF_DEP_LIBICONV_INC/../ --with-zlib=$FF_DEP_LIBZ_INC/../  --host=$FF_ARCH"
fi

# xcode configuration
export DEBUG_INFORMATION_FORMAT=dwarf-with-dsym

cd $FF_BUILD_SOURCE


FF_CMAKE_CFG_FLAGS="-DCMAKE_TOOLCHAIN_FILE=${FF_SOURCE}/android.toolchain.cmake -DBUILD_EXAMPLES=0 -DBUILD_LSR_TESTS=0 -DLIBXML2_WITH_LZMA=0 -DLIBXML2_WITH_PYTHON=0 -DBUILD_SHARED_LIBS=1 -DBUILD_TESTS=0 -DCMAKE_BUILD_TYPE=Release -DWITH_LSR_BINDINGS=0 -DWITH_OPENMP=0 -DWITH_PFFFT=0"
#cmake $FF_CMAKE_CFG_FLAGS $FF_CMAKE_EXTRA_FLAGS  -DCMAKE_PREFIX_PATH=$FF_BUILD_PREFIX -DCMAKE_INSTALL_PREFIX=$FF_BUILD_PREFIX $FF_SOURCE
echo "current working path:$(pwd)"
if [ ! -f configure ];then
    ./autogen.sh
fi
echo "./configure $LIBXML2_CFG_FLAGS"
./configure $LIBXML2_CFG_FLAGS
make clean

#--------------------
echo "\n--------------------"
echo "[*] compile libxml2"
echo "--------------------"
set +e
make
make install
