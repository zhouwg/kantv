#!/usr/bin/env bash

# Copyright (c) 2024- KanTV Authors

# Description: check difference between local llama.cpp and upstream llama.cpp
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
check_upstream_llamacpp
check_local_llamacpp


if [ "${REL_PATH}" != "llamacpp" ]; then
    echo "current path is ${PWD}, it should be kantv/external/ggml/llamacpp, pls check"
    #exit 1
fi


echo -e  "upstream llamacpp path: ${UPSTREAM_LLAMACPP_PATH}\n"
echo -e  "local    llamacpp path: ${LOCAL_LLAMACPP_PATH}\n"

#the following method borrow from bench-all.sh in GGML's project whisper.cpp
LLAMACPP_SRCS=(ggml-alloc.c ggml-alloc.h ggml-backend.c ggml-backend.h ggml.c ggml.h ggml-quants.c ggml-quants.h llama.cpp llama.h unicode.h unicode.cpp unicode-data.h unicode-data.cpp ggml-common.h examples/main/main.cpp)
for file in "${LLAMACPP_SRCS[@]}"; do
    echo "diff $file     ${UPSTREAM_LLAMACPP_PATH}/$file"
    diff ${LOCAL_LLAMACPP_PATH}/$file     ${UPSTREAM_LLAMACPP_PATH}/$file
done
