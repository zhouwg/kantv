#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: check difference between local whisper.cpp and upstream whisper.cpp
#


PWD=`pwd`
REL_PATH=${PWD##*/}

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd
check_upstream_whispercpp
check_local_whispercpp


if [ "${REL_PATH}" != "whispercpp" ]; then
    echo "current path is ${PWD}, it should be kantv/src/main/jni/whispercpp, pls check"
    #exit 1
fi


echo -e  "upstream whispercpp path: ${UPSTREAM_WHISPERCPP_PATH}\n"
echo -e  "local    whispercpp path: ${LOCAL_WHISPERCPP_PATH}\n"

#the following method borrow from bench-all.sh in GGML's project whisper.cpp
WHISPERCPP_SRCS=(ggml-alloc.c ggml-alloc.h ggml-backend.c ggml-backend.h ggml.c ggml.h ggml-quants.c ggml-quants.h whisper.cpp whisper.h)
for file in "${WHISPERCPP_SRCS[@]}"; do
    echo "diff $file     ${UPSTREAM_WHISPERCPP_PATH}/$file"
    diff ${LOCAL_WHISPERCPP_PATH}/$file     ${UPSTREAM_WHISPERCPP_PATH}/$file
done
