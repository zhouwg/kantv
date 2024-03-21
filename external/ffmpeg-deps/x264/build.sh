#!/usr/bin/env bash

# Copyright (c) zhou.weiguo(zhouwg2000@gmail.com). 2015-2023. All rights reserved.

# Copyright (c) Project KanTV. 2021-2023. All rights reserved.

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.


if [ "x${FF_PREFIX}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "FF_PREFIX is empty, pls check"
    exit 1
fi


./configure --prefix=${FF_PREFIX}
make -j${HOST_CPU_COUNTS}
make install-lib-static
#make install-lib-shared
