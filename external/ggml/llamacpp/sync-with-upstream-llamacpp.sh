#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: sync source code between local whisper.cpp and upstream whisper.cpp
#


PWD=`pwd`
REL_PATH=${PWD##*/}
SYNC_TIME=`date +"%Y-%m-%d-%H-%M-%S"`

if [ "x${PROJECT_ROOT_PATH}" == "x" ]; then
    echo "pwd is `pwd`"
    echo "pls run . build/envsetup in project's toplevel directory firstly"
    exit 1
fi

. ${PROJECT_ROOT_PATH}/build/public.sh || (echo "can't find public.sh"; exit 1)

show_pwd
check_upstream_llamacpp
check_local_llamacpp


if [ "${REL_PATH}" != "llamacpp" ]; then
    echo "current path is ${PWD}, it should be kantv/external/ggml/llamacpp, pls check"
    #exit 1
fi


echo -e  "upstream llamacpp path: ${UPSTREAM_LLAMACPP_PATH}\n"
echo -e  "local    llamacpp path: ${LOCAL_LLAMACPP_PATH}\n"
echo -e  "sync source code on ${SYNC_TIME}\n\n"

#the following method borrow from bench-all.sh in GGML's project whisper.cpp
LLAMACPP_SRCS=(ggml-alloc.c ggml-alloc.h ggml-backend.c ggml-backend.h ggml.c ggml.h ggml-quants.c ggml-quants.h llama.cpp llama.h unicode.h unicode.cpp unicode-data.h unicode-data.cpp ggml-common.h)
for file in "${LLAMACPP_SRCS[@]}"; do
    /bin/cp -fv ${UPSTREAM_LLAMACPP_PATH}/$file ${LOCAL_LLAMACPP_PATH}/$file
done

echo -e "\n"
