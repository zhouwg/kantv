#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2015-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.


export PKG_CONFIG_PATH=${FF_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH
echo "export PKG_CONFIG_PATH=${FF_PREFIX}/lib/pkgconfig:$PKG_CONFIG_PATH"

LD_FLAGS="-L${FF_PREFIX}/lib/ -lx264 -lx265 -laom -lwebp -lsharpyuv -lavformat -lavcodec -lSvtAv1Dec -lSvtAv1Enc -lvvenc -lvvdec -lswscale -lavutil -lswresample -lpthread -ldl -lz  -lxcb -lasound -lSDL2 -lXv -lX11 -lXext -pthread -lm -latomic -lm -lz -pthread -lz -lm -lXfixes -lXinerama -lSDL2 -lstdc++"

INC_FLAGS="-I${FF_PREFIX}/include -I${FF_PREFIX}/include/freetype2  -I${FF_PREFIX}/include/fribidi -I${FF_PREFIX}/include/harfbuzz -I./"

./configure  --prefix=${FF_PREFIX} --disable-shared --enable-static --enable-ffmpeg --enable-ffplay --enable-ffprobe --enable-avfilter --enable-postproc --enable-rpath --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --enable-libx264 --enable-libx265 --enable-libaom --enable-libwebp --enable-libsvtav1 --enable-libvvdec --enable-libvvenc --enable-libfreetype --enable-libfribidi --enable-libharfbuzz --enable-gpl --enable-encoders --enable-pic --enable-rpath --extra-cflags=${INC_FLAGS}  --extra-ldflags=${LD_FLAGS} --pkg-config="/usr/bin/pkg-config"

make -j${HOST_CPU_COUNTS}
make install
