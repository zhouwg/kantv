#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2015-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

./configure --prefix=${FF_PREFIX} --disable-shared --enable-static
make -j${HOST_CPU_COUNTS}

make install
