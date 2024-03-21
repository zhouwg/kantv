#!/usr/bin/env bash

CFLAGS="-fpic" ./configure --prefix=${FF_PREFIX} --with-pic --disable-silent-rules
make -j${HOST_CPU_COUNTS}
make install
