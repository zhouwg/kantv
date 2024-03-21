#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2015-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: build script to build third-party FFmpeg dependent libraries


cd ${PROJECT_ROOT_PATH}/external/ffmpeg-deps


ffmpeg_deps=" x264 x265/source aom libwebp svt vvenc vvdec freetype fribidi harfbuzz libpng"

echo "BUILD_TARGET: ${BUILD_TARGET}"

for item in ${ffmpeg_deps};do
    cd ${PROJECT_ROOT_PATH}/external/ffmpeg-deps/${item}/

        echo "build ffmpeg_deps ${item} in `pwd` for target ${BUILD_TARGET} with arch ${BUILD_ARCHS} in ${PROJECT_BUILD_TYPE} mode on host ${BUILD_HOST}"

        if [ -f build.sh ]; then
            ./build.sh x86
        fi

        cd ${PROJECT_ROOT_PATH}/external/
done

cd ${PROJECT_ROOT_PATH}/external/
