# Copyright (c) 2024- KanTV Authors

# Description: build libggml-jni.so for Android APP

cmake_minimum_required(VERSION 3.22.1) # make llamacpp happy
project(ggml-jni)

set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(TARGET_XIAOMI14    ON)    # set it to ON to get much better performance on Xiaomi 14(Snapdragon 8Gen3) or Xiaomi 15(Snapdragon 8Elite(aka 8Gen4) equipped Android phone
set(GGML_ENABLE_HEXAGON    ON) # set it to ON to enable ggml-hexagon backend on Qualcomm SoC equipped Android phone

set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)
set(WHISPERCPP_SRC_DIR          ${GGML_SRC_DIR}/whispercpp/)
set(LLAMACPP_SRC_DIR            ${GGML_SRC_DIR}/llamacpp/)

set(PREBUILT_LIB_PATH           ${GGML_SRC_DIR}/../../android/kantvplayer/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${GGML_SRC_DIR}/../../prebuilts/include/)

set(KANTV_GGMLJNI_SRC_DIR       ${GGML_SRC_DIR}/jni/)

set(QNN_INC_PATH                ${GGML_SRC_DIR}/../../prebuilts/include/QNN)
set(QNN_LIB_PATH                ${GGML_SRC_DIR}/../../prebuilts/lib/)

message("GGML_SRC_DIR         : ${GGML_SRC_DIR}")
message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("LLAMACPP_SRC_DIR     : ${LLAMACPP_SRC_DIR}")
message("PREBUILT_INC_PATH    : ${PREBUILT_INC_PATH}")
message("PREBUILT_LIB_PATH    : ${PREBUILT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")
message("CMAKE_BUILD_TYPE     : ${CMAKE_BUILD_TYPE}")

include_directories(${LLAMACPP_SRC_DIR}/ggml/include)
include_directories(${LLAMACPP_SRC_DIR}/include)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels)

include_directories(${QNN_INC_PATH}/)
include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h

include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/incs)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1//libs/qprintf/inc)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/incs/stddef)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/incs)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/rpcmem/inc)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/remote/ship/android_Debug_aarch64)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/rtos/qurt/computev75/include/qurt)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/utils/examples)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/rtld/ship/android_aarch64)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/libs/atomic/inc)
include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/libs/atomic/android_Debug_aarch64/ship)

if (GGML_ENABLE_HEXAGON)
    #set(HEXAGON_BACKEND_SRCS ${LLAMACPP_SRC_DIR}/ggml/src/ggml-qnn/ggml-hexagon.cpp)
    file(GLOB HEXAGON_BACKEND_SRCS "${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/*.cpp" "${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/stub.c")
endif()

set(SOURCE_FILES
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend-reg.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-quants.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-opt.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-threading.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/gguf.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/cpu-feats-x86.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/binary-ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/unary-ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/vec.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu-aarch64.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu-hbm.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu-quants.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu-traits.cpp
        ${HEXAGON_BACKEND_SRCS}

        ${LLAMACPP_SRC_DIR}/src/llama.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-adapter.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-arch.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-batch.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-chat.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-context.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-cparams.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-grammar.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-hparams.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-impl.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-kv-cache.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-mmap.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model-loader.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-quant.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-sampling.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-vocab.cpp
        ${LLAMACPP_SRC_DIR}/src/unicode.cpp
        ${LLAMACPP_SRC_DIR}/src/unicode-data.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-io.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-graph.cpp

        ${LLAMACPP_SRC_DIR}/common/arg.cpp
        ${LLAMACPP_SRC_DIR}/common/build-info.cpp
        ${LLAMACPP_SRC_DIR}/common/chat.cpp
        ${LLAMACPP_SRC_DIR}/common/common.cpp
        ${LLAMACPP_SRC_DIR}/common/console.cpp
        ${LLAMACPP_SRC_DIR}/common/json-schema-to-grammar.cpp
        ${LLAMACPP_SRC_DIR}/common/llguidance.cpp
        ${LLAMACPP_SRC_DIR}/common/log.cpp
        ${LLAMACPP_SRC_DIR}/common/ngram-cache.cpp
        ${LLAMACPP_SRC_DIR}/common/sampling.cpp
        ${LLAMACPP_SRC_DIR}/common/speculative.cpp

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/llm-inference.cpp
)

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${GGML_SRC_DIR}/)
include_directories(${KANTV_GGMLJNI_SRC_DIR}/)
include_directories(${PREBUILT_INC_PATH}/)

include_directories(${LLAMACPP_SRC_DIR}/)
include_directories(${LLAMACPP_SRC_DIR}/common/)
include_directories(${LLAMACPP_SRC_DIR}/common/minja/)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)
add_definitions(-DGGML_USE_LLAMAFILE)

#2025-01:make the new sw arch of latest upstream llama.cpp happy on Android port
#add_definitions(-D__ARM_ARCH) #FIXME:why NDK complains NEON issue in ggml-quants.c
add_definitions(-D__aarch64__)
add_definitions(-D__linux__)
add_definitions(-DGGML_USE_CPU)

if (GGML_ENABLE_HEXAGON)
add_definitions(-DGGML_USE_HEXAGON)
endif()

add_definitions(-O3) #otherwise app's performance will be very bad on Xiaomi14 with debug build


if (TARGET_XIAOMI14)

#weiguo:2024-03-11
# the below special build optimization ONLY validated ok on Xiaomi 14
# works very well on Xiaomi 14 and got best ASR performance until now(less then 1 second)
# manually enable it for Xiaomi 14
add_definitions(-march=armv8.7-a)
add_definitions(-mcpu=cortex-x1)
add_definitions(-mtune=cortex-x1)

else()

#weiguo:2024-03-10
# the below build optimization might be works well on ALL mainstream Android phones
# but NOT work with realtime subtitle feature(app would crash)
# so it's default enabled
add_definitions(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)

endif()

add_compile_options("-Wall" "-Wno-sign-compare")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    add_definitions(-g)
else()
    add_definitions(-DNDEBUG)
    add_definitions(-O3)
    add_link_options("-s")
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    add_compile_options("-O3"
            "-ffunction-sections"
            "-fdata-sections"
            "-Wno-unused-command-line-argument")
endif()

#weiguo:2024-05-28, fix issue in this PR:https://github.com/zhouwg/kantv/pull/204
add_definitions(-fexceptions)
add_definitions(-D_LIBCPP_EXCEPTIONS)
add_definitions(-D_LIBCXXABI_EXCEPTIONS)
add_link_options( "-lc++_shared" )

if (GGML_ENABLE_QNN)
    file(GLOB allPrebuiltQNNLibs "${QNN_LIB_PATH}/libQnn*.so")

    #file(COPY ${allPrebuiltQNNLibs}  DESTINATION ${PREBUILT_LIB_PATH}/ )

endif()

find_library(LOG_LIB log)

add_library(kantvmedia
        SHARED
        IMPORTED)

set_target_properties(kantvmedia
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libkantv-media.so)

add_library(kantvffmpeg
        SHARED
        IMPORTED)

set_target_properties(kantvffmpeg
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libkantv-ffmpeg.so)

add_library(cdsprpc
        SHARED
        IMPORTED)
set_target_properties(cdsprpc
        PROPERTIES
        IMPORTED_LOCATION
        /opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/remote/ship/android_aarch64/libcdsprpc.so)

add_library(ggmlop_skel
        SHARED
        IMPORTED)
set_target_properties(ggmlop_skel
        PROPERTIES
        IMPORTED_LOCATION
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-qnn/kernels/libggmlop.so)

function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    if (GGML_ENABLE_QNN)
        file(GLOB allQNNLibs "${PREBUILT_LIB_PATH}/libQnn*.so")
        #target_link_libraries(${target_name} ${LOG_LIB} kantvmedia kantvffmpeg android ${allQNNLibs})
        #target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android cdsprpc ggmlop_skel)
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android cdsprpc)
    else()
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android cdsprpc)
    endif()

endfunction()

build_library("ggml-jni")
