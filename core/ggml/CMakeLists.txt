# Description: build libggml-jni.so for a standard Android APP

cmake_minimum_required(VERSION 3.22.1)
project(ggml-jni)

set(CMAKE_VERBOSE_MAKEFILE          ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set it to ON to enable ggml-hexagon backend on Qualcomm SoC based Android phone
# set it to OFF if target Android phone is not Qualcomm SoC based phone

option(GGML_HEXAGON "Enable Hexagon backend for GGML" ON)

if (GGML_HEXAGON)
    #build apk for phone with qcom mobile SoC
    message("set GGML_ENABLE_HEXAGON  to ON")
    set(GGML_ENABLE_HEXAGON ON)
else()
    #build apk for phone with non-qcom mobile SoC
    message("set GGML_ENABLE_HEXAGON  to OFF")
    set(GGML_ENABLE_HEXAGON OFF)
endif()

set(STABLEDIFFUSION_ENABLE_HEXAGON     OFF)

# enable JZ's ggml-hexagon implementation (alternative AP-side implementation)
# when ON, builds ggml-hexagon-jz.cpp + htp/ DSP skel via Makefile
# when OFF, falls back to Qualcomm's official ggml-hexagon.cpp
option(GGML_HEXAGON_JZ "Enable JZ's ggml-hexagon implementation" ON)

# Hexagon SDK is provided via prebuilts/Hexagon_SDK symlink (from ggml-hexagon project)
# QNN SDK is no longer needed — JZ's ggml-hexagon uses Hexagon SDK FastRPC directly

set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)
set(WHISPERCPP_SRC_DIR          ${GGML_SRC_DIR}/whispercpp/)
set(LLAMACPP_SRC_DIR            ${GGML_SRC_DIR}/llamacpp/)
#set(SDCPP_SRC_DIR               ${GGML_SRC_DIR}/stablediffusioncpp/)
set(PREBUILT_PATH               ${GGML_SRC_DIR}/../../prebuilts/)
set(PREBUILT_LIB_PATH           ${GGML_SRC_DIR}/../../android/kantvplayer/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${GGML_SRC_DIR}/../../prebuilts/include/)
set(KANTV_GGMLJNI_SRC_DIR       ${GGML_SRC_DIR}/jni/)

set(HEXAGON_SDK_PATH            ${GGML_SRC_DIR}/../../prebuilts/Hexagon_SDK/6.6.0.0)

# Hexagon Tools path (for hexagon-clang cross-compiler used by htp/Makefile)
# defaults to the Tools subdir inside HEXAGON_SDK_PATH; override with -DHEXAGON_TOOLS_PATH=...
if (NOT DEFINED HEXAGON_TOOLS_PATH)
    set(HEXAGON_TOOLS_PATH ${HEXAGON_SDK_PATH}/tools/HEXAGON_Tools/19.0.07)
endif()

set(OpenCV_DIR                  ${PREBUILT_PATH}/opencv-mobile-4.9.0-android/sdk/native/jni)
set(NCNN_DIR                    ${CMAKE_SOURCE_DIR}/ncnn)
set(NCNN_SRC_DIR                ${CMAKE_SOURCE_DIR}/ncnn/ncnn-src)

set(OPT_FLAG                    " ")

message("CMAKE_BUILD_TYPE     : ${CMAKE_BUILD_TYPE}")
message("GGML_SRC_DIR         : ${GGML_SRC_DIR}")
message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("LLAMACPP_SRC_DIR     : ${LLAMACPP_SRC_DIR}")
message("PREBUILT_INC_PATH    : ${PREBUILT_INC_PATH}")
message("PREBUILT_LIB_PATH    : ${PREBUILT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("HEXAGON_SDK_PATH     : ${HEXAGON_SDK_PATH}")
message("HEXAGON_TOOLS_PATH   : ${HEXAGON_TOOLS_PATH}")
message("OpenCV_DIR           : ${OpenCV_DIR}")
message("NCNN_DIR             : ${NCNN_DIR}")
message("NCNN_SRC_DIR         : ${NCNN_SRC_DIR}")
message("GGML_ENABLE_HEXAGON  : ${GGML_ENABLE_HEXAGON}")
message("GGML_HEXAGON_JZ      : ${GGML_HEXAGON_JZ}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG_FLAG "-DDEBUG -Wall")
    message("Debug mode:${DEBUG_FLAG}")
else()
    set(DEBUG_FLAG "-DNDEBUG -Wall")
    #manually disable all foreground logs in ggml-hexagon/CMakeLists.txt to
    #make compare NPU performance through llama-bench more clear
    #set(DEBUG_FLAG "-DNDEBUG -Wall -DDISABLE_ALL_LOG")
    message("Release mode:${DEBUG_FLAG}")
endif()

if (GGML_ENABLE_HEXAGON)
    #available htp arch version:
    #v73 --- Snapdragon 8 Gen2
    #v75 --- Snapdragon 8 Gen3
    #v79 --- Snapdragon 8 Elite(aka Gen4)
    #v81 --- Snapdragon 8 Elite Gen5(aka 8 Gen5)
    if(NOT DEFINED HTP_ARCH_VERSION)
        message(WARNING "HTP_ARCH_VERSION not defined, valid htp arch: v73,v75,v79,v81")
        #manually set to v75
        set(HTP_ARCH_VERSION v75)
    endif()
    #check whether user's specified htp arch is valid
    set(CHECK_HTP_ARCH "WRONG")
    foreach (feat v73 v75 v79 v81)
        if (${feat} STREQUAL ${HTP_ARCH_VERSION})
            set(CHECK_HTP_ARCH "GOOD")
        endif()
    endforeach()
    if (${CHECK_HTP_ARCH} STREQUAL "WRONG")
        message(FATAL_ERROR "ggml-hexagon backend only support htp arch v73,v75,v79,v81")
    endif()

    #check optimization flags
    if(DEFINED HTP_ARCH_VERSION)
        if (${HTP_ARCH_VERSION} STREQUAL "v75" OR ${HTP_ARCH_VERSION} STREQUAL "v79")
            set(OPT_FLAG " -O3 -march=armv8.7-a -mcpu=cortex-x1 -mtune=cortex-x1 -ffp-model=fast -fno-finite-math-only")
            #refer to:https://github.com/kantv-ai/kantv/issues/287
            #set(OPT_FLAG " -O3 -march=armv8.7-a -flto -D_GNU_SOURCE -fvectorize -ffp-model=fast -fno-finite-math-only")
        else()
            set(OPT_FLAG " -O3 -march=armv8.7-a -ffp-model=fast -fno-finite-math-only")
        endif()
    else()
        set(OPT_FLAG " -O3 -march=armv8.7-a -ffp-model=fast -fno-finite-math-only")
    endif()
else()
    #TODO:verified on Android phone equipped without Qualcomm mobile SoC
    set(OPT_FLAG " -O3 -march=armv8.7-a -ffp-model=fast -fno-finite-math-only")
endif() #if (GGML_ENABLE_HEXAGON)

message("OPT_FLAG:          ${OPT_FLAG}")
message("DEBUG_FLAGS:       ${DEBUG_FLAG}")

include_directories(${LLAMACPP_SRC_DIR}/ggml/include)
include_directories(${LLAMACPP_SRC_DIR}/include)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu)
include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${GGML_SRC_DIR}/)
include_directories(${KANTV_GGMLJNI_SRC_DIR}/)
include_directories(${PREBUILT_INC_PATH}/)
include_directories(${LLAMACPP_SRC_DIR}/)
include_directories(${LLAMACPP_SRC_DIR}/common/)
# NOTE: src/ must come AFTER common/ — src/unicode.h shadows common/unicode.h
include_directories(${LLAMACPP_SRC_DIR}/src/)
# NOTE: do NOT add common/jinja/ to include path — its string.h shadows libc <string.h>
include_directories(${LLAMACPP_SRC_DIR}/tools/mtmd)
include_directories(${LLAMACPP_SRC_DIR}/vendor)
include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h
#include_directories(${SDCPP_SRC_DIR}/)
include_directories(${OpenCV_DIR}/include/)
include_directories(${NCNN_SRC_DIR}/include/)

if (GGML_ENABLE_HEXAGON)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp)
include_directories(${HEXAGON_SDK_PATH}/incs)
include_directories(${HEXAGON_SDK_PATH}/incs/stddef)
include_directories(${HEXAGON_SDK_PATH}/libs/qprintf/inc)
include_directories(${HEXAGON_SDK_PATH}/ipc/fastrpc/incs)
include_directories(${HEXAGON_SDK_PATH}/utils/examples)
include_directories(${HEXAGON_SDK_PATH}/ipc/fastrpc/rpcmem/inc)
include_directories(${HEXAGON_SDK_PATH}/ipc/fastrpc/remote/ship/android_Debug_aarch64)
include_directories(${HEXAGON_SDK_PATH}/ipc/fastrpc/rtld/ship/android_aarch64)
include_directories(${HEXAGON_SDK_PATH}/libs/atomic/inc)
include_directories(${HEXAGON_SDK_PATH}/libs/atomic/android_Debug_aarch64/ship)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp/)

# Generate ggml_dsp_{stub.c,skel.c,h} from ggml_dsp.idl via qaic (JZ only)
if (GGML_HEXAGON_JZ)
    set(GGML_DSP_STUB ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp/ggml_dsp_stub.c)
    set(GGML_DSP_HEADER ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp/ggml_dsp.h)
    set(QAIC_BIN ${HEXAGON_SDK_PATH}/ipc/fastrpc/qaic/bin/qaic)
    if ((NOT EXISTS "${GGML_DSP_STUB}" OR NOT EXISTS "${GGML_DSP_HEADER}") AND EXISTS "${QAIC_BIN}")
        message(STATUS "hexagon-jz: generating ggml_dsp stubs from ggml_dsp.idl")
        execute_process(
            COMMAND ${QAIC_BIN} -mdll
                -o ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp
                -I${HEXAGON_SDK_PATH}/incs
                -I${HEXAGON_SDK_PATH}/incs/stddef
                -I${HEXAGON_SDK_PATH}/ipc/fastrpc/incs
                ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp/ggml_dsp.idl
            RESULT_VARIABLE QAIC_RESULT)
        if (NOT QAIC_RESULT EQUAL 0)
            message(FATAL_ERROR "hexagon-jz: qaic failed to generate ggml_dsp stubs")
        endif()
    endif()

    set(HEXAGON_BACKEND_SRCS
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/ggml-hexagon-jz.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp-drv.cpp
    )
    if (EXISTS "${GGML_DSP_STUB}")
        list(APPEND HEXAGON_BACKEND_SRCS ${GGML_DSP_STUB})
    endif()
else()
    file(GLOB HEXAGON_BACKEND_SRCS "${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/*.cpp")
endif()
endif()

#aux_source_directory(${GGML_SRC_DIR}/stablediffusioncpp/          stablediffusioncpp)

# glob model implementations (ggml-hexagon splits model archs into src/models/)
file(GLOB LLAMA_MODELS_SOURCES "${LLAMACPP_SRC_DIR}/src/models/*.cpp")

# glob mtmd model implementations (vision/audio model archs in tools/mtmd/models/)
file(GLOB MTMD_MODELS_SOURCES "${LLAMACPP_SRC_DIR}/tools/mtmd/models/*.cpp")

# Define build-info variables (normally provided by cmake/build-info.cmake)
if(NOT DEFINED LLAMA_BUILD_NUMBER)
    set(LLAMA_BUILD_NUMBER 0)
endif()
if(NOT DEFINED LLAMA_BUILD_COMMIT)
    set(LLAMA_BUILD_COMMIT "${GGML_BUILD_COMMIT}")
endif()
set(BUILD_COMPILER "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
set(BUILD_TARGET "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

# generate build-info.cpp from template (ggml-hexagon uses build-info.cpp.in)
configure_file(
    ${LLAMACPP_SRC_DIR}/common/build-info.cpp.in
    ${CMAKE_BINARY_DIR}/build-info.cpp
    @ONLY)

set(SOURCE_FILES
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend-reg.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend-meta.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-backend-dl.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-quants.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-opt.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-threading.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/gguf.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ggml-cpu.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/binary-ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/unary-ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/vec.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/ops.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/repack.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/hbm.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/quants.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/traits.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/arch/arm/quants.c
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/arch/arm/repack.cpp
        ${LLAMACPP_SRC_DIR}/ggml/src/ggml-cpu/llamafile/sgemm.cpp
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
        ${LLAMACPP_SRC_DIR}/src/llama-kv-cache-iswa.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-kv-cache-dsa.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-kv-cache-dsv4.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-memory.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-memory-hybrid.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-memory-hybrid-iswa.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-memory-recurrent.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-mmap.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model-loader.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model-saver.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-quant.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-sampler.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-vocab.cpp
        ${LLAMACPP_SRC_DIR}/src/unicode.cpp
        ${LLAMACPP_SRC_DIR}/src/unicode-data.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-io.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-graph.cpp
        ${LLAMA_MODELS_SOURCES}

        ${LLAMACPP_SRC_DIR}/common/arg.cpp
        ${CMAKE_BINARY_DIR}/build-info.cpp
        ${LLAMACPP_SRC_DIR}/vendor/cpp-httplib/httplib.cpp
        ${LLAMACPP_SRC_DIR}/common/chat.cpp
        ${LLAMACPP_SRC_DIR}/common/chat-auto-parser-generator.cpp
        ${LLAMACPP_SRC_DIR}/common/chat-auto-parser-helpers.cpp
        ${LLAMACPP_SRC_DIR}/common/chat-diff-analyzer.cpp
        ${LLAMACPP_SRC_DIR}/common/chat-peg-parser.cpp
        ${LLAMACPP_SRC_DIR}/common/common.cpp
        ${LLAMACPP_SRC_DIR}/common/console.cpp
        ${LLAMACPP_SRC_DIR}/common/debug.cpp
        ${LLAMACPP_SRC_DIR}/common/download.cpp
        ${LLAMACPP_SRC_DIR}/common/fit.cpp
        ${LLAMACPP_SRC_DIR}/common/hf-cache.cpp
        ${LLAMACPP_SRC_DIR}/common/imatrix-loader.cpp
        ${LLAMACPP_SRC_DIR}/common/json-schema-to-grammar.cpp
        ${LLAMACPP_SRC_DIR}/common/llguidance.cpp
        ${LLAMACPP_SRC_DIR}/common/log.cpp
        ${LLAMACPP_SRC_DIR}/common/ngram-cache.cpp
        ${LLAMACPP_SRC_DIR}/common/ngram-map.cpp
        ${LLAMACPP_SRC_DIR}/common/ngram-mod.cpp
        ${LLAMACPP_SRC_DIR}/common/peg-parser.cpp
        ${LLAMACPP_SRC_DIR}/common/preset.cpp
        ${LLAMACPP_SRC_DIR}/common/reasoning-budget.cpp
        ${LLAMACPP_SRC_DIR}/common/sampling.cpp
        ${LLAMACPP_SRC_DIR}/common/speculative.cpp
        ${LLAMACPP_SRC_DIR}/common/unicode.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/caps.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/lexer.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/parser.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/runtime.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/string.cpp
        ${LLAMACPP_SRC_DIR}/common/jinja/value.cpp

        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd-helper.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd-audio.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd-image.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/clip.cpp
        ${MTMD_MODELS_SOURCES}

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/llm-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ndkcamera.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/myndkcamera.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/bmp.c
        ${KANTV_GGMLJNI_SRC_DIR}/realtime-video-recognition.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/mtmd-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-context.cpp
)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)
add_definitions(-DGGML_USE_LLAMAFILE)

add_definitions(-DSTANDARD_ANDROID_APP)
add_definitions(-DPROJECT_NAME=\"KANTV\")

add_definitions(-D__aarch64__)
add_definitions(-D__linux__)
add_definitions(-DGGML_USE_CPU)
add_definitions(-O3)

# Define GGML_USE_HEXAGON when hexagon backend is enabled (expected by JNI layer)
if (GGML_ENABLE_HEXAGON)
    add_definitions(-DGGML_USE_HEXAGON)
endif()

# GGML_KQ_MASK_PAD: padding for KQ mask tensor (required by whisper.cpp, normally in newer ggml.h)
add_definitions(-DGGML_KQ_MASK_PAD=32)

# GGML version/commit (required by ggml.c — normally provided by ggml/CMakeLists.txt)
find_program(GIT_EXE NAMES git git.exe NO_CMAKE_FIND_ROOT_PATH)
if(GIT_EXE)
    execute_process(COMMAND ${GIT_EXE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GGML_BUILD_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
endif()
if(NOT GGML_BUILD_COMMIT)
    set(GGML_BUILD_COMMIT "unknown")
endif()
add_definitions(-DGGML_VERSION="0.17.0")
add_definitions(-DGGML_COMMIT="${GGML_BUILD_COMMIT}")

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

#2024-05-28, fix issue in this PR:https://github.com/kantv-ai/kantv/pull/204
add_definitions(-fexceptions)
add_definitions(-D_LIBCPP_EXCEPTIONS)
add_definitions(-D_LIBCXXABI_EXCEPTIONS)
add_link_options( "-lc++_shared" )

if (GGML_ENABLE_HEXAGON)
    if (STABLEDIFFUSION_ENABLE_HEXAGON)
        message("ggml_use_hexagon and sd_use_hexagon")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
    else()
        message("ggml_use_hexagon")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
    endif()
else()
    message("no ggml_use_hexagon")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}    ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  ${DEBUG_FLAG} ${OPT_FLAG}")
endif()

message("CMAKE_C_FLAGS:                 ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS:               ${CMAKE_CXX_FLAGS}")
message("CMAKE_C_FLAGS_RELEASE:         ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_RELEASE:       ${CMAKE_CXX_FLAGS_RELEASE}")

find_library(LOG_LIB log)
set(CMAKE_MODULE_PATH "${OpenCV_DIR};${CMAKE_MODULE_PATH}")
find_package(OpenCV REQUIRED core imgproc)

add_library(kantvmedia
        SHARED
        IMPORTED)

set_target_properties(kantvmedia
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libkantv-media.so)

if (GGML_ENABLE_HEXAGON)
add_library(cdsprpc
        SHARED
        IMPORTED)
set_target_properties(cdsprpc
        PROPERTIES
        IMPORTED_LOCATION
        ${HEXAGON_SDK_PATH}/ipc/fastrpc/remote/ship/android_aarch64/libcdsprpc.so)
endif()

function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    if (GGML_ENABLE_HEXAGON)
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android cdsprpc ${OpenCV_LIBS} camera2ndk mediandk ncnn)
    else()
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android ${OpenCV_LIBS} camera2ndk mediandk ncnn)
    endif()

endfunction()

build_library("ggml-jni")

#cross compiling source codes of hexagon DSP skel which running on cDSP side
function(ggml_hexagon_build_kernel KNAME)
    message(STATUS "ggml_hexagon: build hexagon DSP skel ${KNAME}")

    set(HTP_SKEL_DIR ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/htp)
    set(ASSETS_DIR   ${GGML_SRC_DIR}/../../android/kantvplayer/src/main/assets/models/)

    if (GGML_HEXAGON_JZ)
        # JZ path: use htp/Makefile with HEXAGON_TOOLS_PATH
        # regenerate IDL stubs before build (in case ggml_dsp.idl changed)
        add_custom_command(
                TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND ${HEXAGON_SDK_PATH}/ipc/fastrpc/qaic/bin/qaic -mdll
                    -o ${HTP_SKEL_DIR}
                    -I${HEXAGON_SDK_PATH}/incs
                    -I${HEXAGON_SDK_PATH}/incs/stddef
                    -I${HEXAGON_SDK_PATH}/ipc/fastrpc/incs
                    ${HTP_SKEL_DIR}/ggml_dsp.idl
                COMMAND make -C ${HTP_SKEL_DIR} clean
                COMMAND make -C ${HTP_SKEL_DIR}
                    HEXAGON_SDK_PATH=${HEXAGON_SDK_PATH}
                    HEXAGON_TOOLS_PATH=${HEXAGON_TOOLS_PATH}
                    HTP_ARCH_VERSION=${HTP_ARCH_VERSION}
                    DEBUG_FLAG=${DEBUG_FLAG}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSETS_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${HTP_SKEL_DIR}/libggmldsp-skel.so ${ASSETS_DIR}/
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${HTP_SKEL_DIR}/libggmldsp-skel.so
                    ${ASSETS_DIR}/libggmldsp-skel-${HTP_ARCH_VERSION}.so
                COMMAND echo "current working path:`pwd`\n"
                COMMENT "build JZ hexagon DSP skel (${HTP_ARCH_VERSION})"
        )
    else()
        # Qualcomm path: use kernels/Makefile (legacy)
        add_custom_command(
                TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND make -C ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/ clean
                COMMAND make -C ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/ HEXAGON_SDK_PATH=${HEXAGON_SDK_PATH} HTP_ARCH_VERSION=${HTP_ARCH_VERSION} DEBUG_FLAG=${DEBUG_FLAG}
                COMMAND echo "current working path:`pwd`\n"
                COMMENT "build hexagon-kernel"
        )
    endif()
endfunction()

if (GGML_ENABLE_HEXAGON)
ggml_hexagon_build_kernel("cdsp")
endif()
