#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

#Description: check difference between local whisper.cpp and upstream whisper.cpp
#



PWD=`pwd`
REL_PATH=${PWD##*/}


#begin sanity check
if [  -z ${UPSTREAM_WHISPERCPP_PATH} ]; then
    echo "pls set UPSTREAM_WHISPERCPP_PATH properly"
    echo "or run . build/envsetup.sh firstly"
    exit 1
fi


if [ ! -d ${UPSTREAM_WHISPERCPP_PATH} ]; then
    echo "${UPSTREAM_WHISPERCPP_PATH} not exist, pls check"
    exit 1
fi



if [ "${REL_PATH}" != "whispercpp" ]; then
    echo "current path is ${PWD}, it should be kantv/src/main/jni/whispercpp, pls check"
    exit 1
fi
#end sanity check


echo "diff ggml-alloc.c     $UPSTREAM_WHISPERCPP_PATH/ggml-alloc.c"
diff ggml-alloc.c           $UPSTREAM_WHISPERCPP_PATH/ggml-alloc.c

echo "diff ggml-alloc.h     $UPSTREAM_WHISPERCPP_PATH/ggml-alloc.h"
diff ggml-alloc.h           $UPSTREAM_WHISPERCPP_PATH/ggml-alloc.h

echo "diff ggml-backend.c   $UPSTREAM_WHISPERCPP_PATH/ggml-backend.c"
diff ggml-backend.c         $UPSTREAM_WHISPERCPP_PATH/ggml-backend.c

echo "diff ggml-backend.h   $UPSTREAM_WHISPERCPP_PATH/ggml-backend.h"
diff ggml-backend.h         $UPSTREAM_WHISPERCPP_PATH/ggml-backend.h

echo "diff ggml.h           $UPSTREAM_WHISPERCPP_PATH/ggml.h"
diff ggml.h                 $UPSTREAM_WHISPERCPP_PATH/ggml.h

echo "diff ggml.c           $UPSTREAM_WHISPERCPP_PATH/ggml.c"
diff ggml.c                 $UPSTREAM_WHISPERCPP_PATH/ggml.c

echo "diff ggml-quants.h    $UPSTREAM_WHISPERCPP_PATH/ggml-quants.h"
diff ggml-quants.h          $UPSTREAM_WHISPERCPP_PATH/ggml-quants.h

echo "diff ggml-quants.c    $UPSTREAM_WHISPERCPP_PATH/ggml-quants.c"
diff ggml-quants.c          $UPSTREAM_WHISPERCPP_PATH/ggml-quants.c

echo "diff whisper.h        $UPSTREAM_WHISPERCPP_PATH/whisper.h"
diff whisper.h              $UPSTREAM_WHISPERCPP_PATH/whisper.h

echo "diff whisper.cpp      $UPSTREAM_WHISPERCPP_PATH/whisper.cpp"
diff whisper.cpp            $UPSTREAM_WHISPERCPP_PATH/whisper.cpp
