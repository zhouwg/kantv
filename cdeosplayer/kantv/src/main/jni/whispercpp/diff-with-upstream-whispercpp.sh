#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors. All Rights Reserved.

# Description: check difference between local whisper.cpp and upstream whisper.cpp
#


PWD=`pwd`
REL_PATH=${PWD##*/}

echo -e  "current path: ${PWD}\n"

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

check_upstream_whispercpp
check_local_whispercpp


if [ "${REL_PATH}" != "whispercpp" ]; then
    echo "current path is ${PWD}, it should be kantv/src/main/jni/whispercpp, pls check"
    #exit 1
fi


echo -e  "upstream whispercpp path: ${UPSTREAM_WHISPERCPP_PATH}\n"
echo -e  "local    whispercpp path: ${UPSTREAM_WHISPERCPP_PATH}\n"


echo "diff ggml-alloc.c     ${UPSTREAM_WHISPERCPP_PATH}/ggml-alloc.c"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-alloc.c          ${UPSTREAM_WHISPERCPP_PATH}/ggml-alloc.c

echo "diff ggml-alloc.h     ${UPSTREAM_WHISPERCPP_PATH}/ggml-alloc.h"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-alloc.h          ${UPSTREAM_WHISPERCPP_PATH}/ggml-alloc.h

echo "diff ggml-backend.c   ${UPSTREAM_WHISPERCPP_PATH}/ggml-backend.c"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-backend.c        ${UPSTREAM_WHISPERCPP_PATH}/ggml-backend.c

echo "diff ggml-backend.h   ${UPSTREAM_WHISPERCPP_PATH}/ggml-backend.h"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-backend.h        ${UPSTREAM_WHISPERCPP_PATH}/ggml-backend.h

echo "diff ggml.h           ${UPSTREAM_WHISPERCPP_PATH}/ggml.h"
diff ${LOCAL_WHISPERCPP_PATH}/ggml.h                ${UPSTREAM_WHISPERCPP_PATH}/ggml.h

echo "diff ggml.c           ${UPSTREAM_WHISPERCPP_PATH}/ggml.c"
diff ${LOCAL_WHISPERCPP_PATH}/ggml.c                ${UPSTREAM_WHISPERCPP_PATH}/ggml.c

echo "diff ggml-quants.h    ${UPSTREAM_WHISPERCPP_PATH}/ggml-quants.h"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-quants.h         ${UPSTREAM_WHISPERCPP_PATH}/ggml-quants.h

echo "diff ggml-quants.c    ${UPSTREAM_WHISPERCPP_PATH}/ggml-quants.c"
diff ${LOCAL_WHISPERCPP_PATH}/ggml-quants.c         ${UPSTREAM_WHISPERCPP_PATH}/ggml-quants.c

echo "diff whisper.h        ${UPSTREAM_WHISPERCPP_PATH}/whisper.h"
diff ${LOCAL_WHISPERCPP_PATH}/whisper.h             ${UPSTREAM_WHISPERCPP_PATH}/whisper.h

echo "diff whisper.cpp      ${UPSTREAM_WHISPERCPP_PATH}/whisper.cpp"
diff ${LOCAL_WHISPERCPP_PATH}/whisper.cpp           ${UPSTREAM_WHISPERCPP_PATH}/whisper.cpp
