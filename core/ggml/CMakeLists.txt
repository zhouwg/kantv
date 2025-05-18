# Description: build libggml-jni.so for a standard Android APP

cmake_minimum_required(VERSION 3.22.1)
project(ggml-jni)

set(CMAKE_VERBOSE_MAKEFILE          ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set it to ON to enable ggml-hexagon backend on Qualcomm SoC based Android phone
# set it to OFF if target Android phone is not Qualcomm SoC based phone
set(GGML_ENABLE_HEXAGON             ON)

set(GGML_ENABLE_STABLEDIFFUSION     OFF)

# self-contained build with internal customized/tailored QNN-SDK and Hexagon-SDK,
# without download fully official QNN-SDK and Hexagon-SDK.
# the internal customized/tailored QNN-SDK and Hexagon-SDK also follow Qualcomm's IPR policy.
set(ENABLE_SELF_CONTAINED_BUILD     ON)

set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)
set(WHISPERCPP_SRC_DIR          ${GGML_SRC_DIR}/whispercpp/)
set(LLAMACPP_SRC_DIR            ${GGML_SRC_DIR}/llamacpp/)
set(SDCPP_SRC_DIR               ${GGML_SRC_DIR}/stablediffusioncpp/)
set(PREBUILT_PATH               ${GGML_SRC_DIR}/../../prebuilts/)
set(PREBUILT_LIB_PATH           ${GGML_SRC_DIR}/../../android/kantvplayer/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${GGML_SRC_DIR}/../../prebuilts/include/)
set(KANTV_GGMLJNI_SRC_DIR       ${GGML_SRC_DIR}/jni/)

if (ENABLE_SELF_CONTAINED_BUILD)
set(QNN_SDK_PATH                ${GGML_SRC_DIR}/../../prebuilts/QNN_SDK/2.34.0.250424/)
set(HEXAGON_SDK_PATH            ${GGML_SRC_DIR}/../../prebuilts/Hexagon_SDK/6.2.0.1)
else()
set(QNN_SDK_PATH                "/opt/qcom/aistack/qairt/2.34.0.250424/")
set(HEXAGON_SDK_PATH            "/opt/qcom/Hexagon_SDK/6.2.0.1/")
endif()

set(QNN_INC_PATH                ${QNN_SDK_PATH}/include/QNN/)
set(QNN_LIB_PATH                ${QNN_SDK_PATH}/lib/aarch64-android/)
set(OpenCV_DIR                  ${PREBUILT_PATH}/opencv-mobile-4.9.0-android/sdk/native/jni)
set(NCNN_DIR                    ${CMAKE_SOURCE_DIR}/ncnn)
set(NCNN_SRC_DIR                ${CMAKE_SOURCE_DIR}/ncnn/ncnn-src)

set(OPT_FLAG                    " ")

message("CMAKE_BUILD_TYPE     : ${CMAKE_BUILD_TYPE}")
message("GGML_SRC_DIR         : ${GGML_SRC_DIR}")
message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("LLAMACPP_SRC_DIR     : ${LLAMACPP_SRC_DIR}")
message("SDCPP_SRC_DIR        : ${SDCPP_SRC_DIR}")
message("PREBUILT_INC_PATH    : ${PREBUILT_INC_PATH}")
message("PREBUILT_LIB_PATH    : ${PREBUILT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("QNN_SDK_PATH         : ${QNN_SDK_PATH}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")
message("HEXAGON_SDK_PATH     : ${HEXAGON_SDK_PATH}")
message("OpenCV_DIR           : ${OpenCV_DIR}")
message("NCNN_DIR             : ${NCNN_DIR}")
message("NCNN_SRC_DIR         : ${NCNN_SRC_DIR}")

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
    #v68 --- Snapdragon 888
    #v69 --- Snapdragon 8 Gen1
    #v73 --- Snapdragon 8 Gen2
    #v75 --- Snapdragon 8 Gen3
    #v79 --- Snapdragon 8 Elite(aka Gen4)
    if(NOT DEFINED HTP_ARCH_VERSION)
        message(WARNING "HTP_ARCH_VERSION not defined, valid htp arch: v68,v69,v73,v75,v79")
        #manually set to v75
        set(HTP_ARCH_VERSION v75)
    endif()
    #check whether user's specified htp arch is valid
    set(CHECK_HTP_ARCH "WRONG")
    foreach (feat v68 v69 v73 v75 v79)
        if (${feat} STREQUAL ${HTP_ARCH_VERSION})
            set(CHECK_HTP_ARCH "GOOD")
        endif()
    endforeach()
    if (${CHECK_HTP_ARCH} STREQUAL "WRONG")
        message(FATAL_ERROR "ggml-hexagon backend only support htp arch v68,v69,v73,v75,v79")
    endif()

    #check optimization flags
    if(DEFINED HTP_ARCH_VERSION)
        if (${HTP_ARCH_VERSION} STREQUAL "v75" OR ${HTP_ARCH_VERSION} STREQUAL "v79")
            set(OPT_FLAG " -O3 -march=armv8.7-a -mcpu=cortex-x1 -mtune=cortex-x1 -ffp-model=fast -fno-finite-math-only")
            #refer to:https://github.com/kantv-ai/kantv/issues/287
            #set(OPT_FLAG " -O3 -march=armv8.7-a -flto -D_GNU_SOURCE -fvectorize -ffp-model=fast -fno-finite-math-only")
        else()
            set(OPT_FLAG " -mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8")
        endif()
    else()
        set(OPT_FLAG " -mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8")
    endif()
else()
    #verified on Android phone equipped with mainstream high-end mobile SoC
    set(OPT_FLAG " -O3 -march=armv8.7-a -mcpu=cortex-x1 -mtune=cortex-x1 ")
endif() #if (GGML_ENABLE_HEXAGON)

message("OPT_FLAG:${OPT_FLAG}")

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
include_directories(${LLAMACPP_SRC_DIR}/common/minja/)
include_directories(${LLAMACPP_SRC_DIR}/tools/mtmd)
include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h
include_directories(${SDCPP_SRC_DIR}/)
include_directories(${OpenCV_DIR}/include/)
include_directories(${NCNN_SRC_DIR}/include/)

if (GGML_ENABLE_HEXAGON)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon)
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels)
include_directories(${QNN_INC_PATH}/)
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
include_directories(${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/)
#set(HEXAGON_BACKEND_SRCS ${LLAMACPP_SRC_DIR}/ggml/src/ggml-qnn/ggml-hexagon.cpp)
file(GLOB HEXAGON_BACKEND_SRCS "${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/*.cpp" "${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/stub.c")
endif()

aux_source_directory(${GGML_SRC_DIR}/stablediffusioncpp/          stablediffusioncpp)
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
        ${LLAMACPP_SRC_DIR}/src/llama-mmap.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model-loader.cpp
        ${LLAMACPP_SRC_DIR}/src/llama-model-saver.cpp
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

        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd-helper.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/clip.cpp
        ${LLAMACPP_SRC_DIR}/tools/mtmd/mtmd-cli.cpp

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${stablediffusioncpp}

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/llm-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/sd-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ndkcamera.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/bmp.c
        ${KANTV_GGMLJNI_SRC_DIR}/realtime-video-recognition.cpp
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
    if (GGML_ENABLE_STABLEDIFFUSION)
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGGML_USE_HEXAGON -DSD_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
    else()
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGGML_USE_HEXAGON ${DEBUG_FLAG} ${OPT_FLAG}")
    endif()
else()
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}    ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  ${DEBUG_FLAG} ${OPT_FLAG}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  ${DEBUG_FLAG} ${OPT_FLAG}")
endif()

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
        #file(GLOB allPrebuiltQNNLibs "${QNN_LIB_PATH}/libQnn*.so")
        #file(COPY ${allPrebuiltQNNLibs}  DESTINATION ${PREBUILT_LIB_PATH}/ )
        #target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android ${allPrebuiltQNNLibs})
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android cdsprpc ${OpenCV_LIBS} camera2ndk mediandk ncnn)
    else()
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android ${OpenCV_LIBS} camera2ndk mediandk ncnn)
    endif()

endfunction()

build_library("ggml-jni")

#cross compiling source codes of hexagon kernels which running on cDSP side
function(ggml_hexagon_build_kernel KNAME)
    message(STATUS "ggml_hexagon: build hexagon-kernel ${KNAME}")

    add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND make -C ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/ clean
            COMMAND make -C ${LLAMACPP_SRC_DIR}/ggml/src/ggml-hexagon/kernels/ HEXAGON_SDK_PATH=${HEXAGON_SDK_PATH} HTP_ARCH_VERSION=${HTP_ARCH_VERSION} DEBUG_FLAG=${DEBUG_FLAG}
            COMMAND echo "current working path:`pwd`\n"
            COMMENT "build hexagon-kernel"
    )
endfunction()

if (GGML_ENABLE_HEXAGON)
ggml_hexagon_build_kernel("cdsp")
endif()
