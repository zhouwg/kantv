# Copyright (c) 2024- KanTV Authors

# Description: build libggml-jni.so for Project KanTV

cmake_minimum_required(VERSION 3.22.1) # make llamacpp happy
project(ggml-jni)

set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(TARGET_XIAOMI14    ON)  # set it to ON to get much better performance on Xiaomi 14 or Qualcomm Snapdragon 8 Gen 3 SoC based Android phone
set(GGML_ENABLE_QNN    ON)  # set it to ON to enable QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK on Qualcomm SoC based Android phone

set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)

#03-05-2024, PoC#64: English realtime subtitle for English on-line TV by whisper.cpp
set(WHISPERCPP_SRC_DIR          ${GGML_SRC_DIR}/whispercpp/)
set(PREBUILT_LIB_PATH           ${GGML_SRC_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${GGML_SRC_DIR}/../../prebuilts/include/)
set(KANTV_GGMLJNI_SRC_DIR       ${GGML_SRC_DIR}/jni/)
set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)


#03-26-2024, integrate llama.cpp to kantv
set(LLAMACPP_SRC_DIR            ${GGML_SRC_DIR}/llamacpp/)


set(SDCPP_SRC_DIR               ${GGML_SRC_DIR}/stablediffusioncpp/)

set(BARKCPP_SRC_DIR             ${GGML_SRC_DIR}/barkcpp/)

#03-29-2024, PoC#121:Add Qualcomm mobile SoC native backend for GGML
set(QNN_INC_PATH                ${GGML_SRC_DIR}/../../prebuilts/include/QNN)
set(QNN_LIB_PATH                ${GGML_SRC_DIR}/../../prebuilts/lib/)
set(QNN_SRC_DIR                 ${GGML_SRC_DIR}/qnnsample/)

message("GGML_SRC_DIR         : ${GGML_SRC_DIR}")
message("WHISPERCPP_SRC_DIR   : ${WHISPERCPP_SRC_DIR}")
message("LLAMACPP_SRC_DIR     : ${LLAMACPP_SRC_DIR}")
message("SDCPP_SRC_DIR        : ${SDCPP_SRC_DIR}")
message("PREBUILT_INC_PATH    : ${PREBUILT_INC_PATH}")
message("PREBUILT_LIB_PATH    : ${PREBUILT_LIB_PATH}")
message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("target name          : ${TARGET_NAME}")
message("build target         : ${BUILD_TARGET}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")
message("CMAKE_BUILD_TYPE     : ${CMAKE_BUILD_TYPE}")


include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h
include_directories(${SDCPP_SRC_DIR})
include_directories(${BARKCPP_SRC_DIR})

if (GGML_ENABLE_QNN)
aux_source_directory(${GGML_SRC_DIR}/stablediffusioncpp/          stablediffusioncpp)

include_directories(${QNN_INC_PATH}/)
include_directories(${QNN_SRC_DIR}/)
include_directories(${QNN_SRC_DIR}/Utils)
include_directories(${QNN_SRC_DIR}/Log)
include_directories(${QNN_SRC_DIR}/WrapperUtils)
include_directories(${QNN_SRC_DIR}/PAL/include)

add_definitions(-DGGML_USE_QNN)

set(SOURCE_FILES
        #03-30-2024, upstream source code in llama.cpp is preferred
        ${LLAMACPP_SRC_DIR}/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml-backend.c
        ${LLAMACPP_SRC_DIR}/ggml-quants.c
        ${LLAMACPP_SRC_DIR}/ggml-qnn.cpp # implementation of PoC S4, https://github.com/zhouwg/kantv/issues/121
        ${LLAMACPP_SRC_DIR}/llama.cpp
        ${LLAMACPP_SRC_DIR}/unicode.cpp
        ${LLAMACPP_SRC_DIR}/unicode-data.cpp
        ${LLAMACPP_SRC_DIR}/sgemm.cpp
        ${LLAMACPP_SRC_DIR}/common/sampling.cpp
        ${LLAMACPP_SRC_DIR}/common/common.cpp
        ${LLAMACPP_SRC_DIR}/common/build-info.cpp
        ${LLAMACPP_SRC_DIR}/common/grammar-parser.cpp
        ${LLAMACPP_SRC_DIR}/common/json-schema-to-grammar.cpp

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${stablediffusioncpp}

        ${BARKCPP_SRC_DIR}/bark.cpp
        ${BARKCPP_SRC_DIR}/encodec.cpp
        ${BARKCPP_SRC_DIR}/ggml-allocr.c

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/mnist-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/tts-inference.cpp

        #05-25-2024, import MiniCPM-V(A GPT-4V Level Multimodal LLM), https://github.com/OpenBMB/MiniCPM-V
        ${KANTV_GGMLJNI_SRC_DIR}/clip.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv_io.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv-cli.cpp
    )
else()

add_definitions(-DGGML_DISABLE_QNN)

set(SOURCE_FILES
        ${LLAMACPP_SRC_DIR}/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml-backend.c
        ${LLAMACPP_SRC_DIR}/ggml-quants.c

        ${LLAMACPP_SRC_DIR}/llama.cpp
        ${LLAMACPP_SRC_DIR}/unicode.cpp
        ${LLAMACPP_SRC_DIR}/unicode-data.cpp
        ${LLAMACPP_SRC_DIR}/sgemm.cpp
        ${LLAMACPP_SRC_DIR}/common/sampling.cpp
        ${LLAMACPP_SRC_DIR}/common/common.cpp
        ${LLAMACPP_SRC_DIR}/common/build-info.cpp
        ${LLAMACPP_SRC_DIR}/common/grammar-parser.cpp
        ${LLAMACPP_SRC_DIR}/common/json-schema-to-grammar.cpp

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/mnist-inference.cpp
    )
endif()

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${GGML_SRC_DIR}/)
include_directories(${KANTV_GGMLJNI_SRC_DIR}/)
include_directories(${PREBUILT_INC_PATH}/)

#re-use the ggml.h in subdirectory llamacpp/ggml.h to avoid NDK complain "error: redefinition of 'ggml_status'"
include_directories(${LLAMACPP_SRC_DIR}/)
include_directories(${LLAMACPP_SRC_DIR}/common/)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)
add_definitions(-DGGML_USE_LLAMAFILE)
add_definitions(-O3)


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


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    add_definitions(-g)
    add_compile_options("-Wall" "-Wno-sign-compare")
    #add_compile_options("-Wall" "-Werror" "-Wno-sign-compare")
else()
    add_definitions(-DNDEBUG)
    add_link_options("-s")
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    add_compile_options("-O3"
            "-ffunction-sections"
            "-fdata-sections"
            "-frtti"
            "-Wno-unused-command-line-argument")
endif()

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

function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    if (GGML_ENABLE_QNN)
        file(GLOB allQNNLibs "${PREBUILT_LIB_PATH}/libQnn*.so")
        #target_link_libraries(${target_name} ${LOG_LIB} kantvmedia kantvffmpeg android ${allQNNLibs})
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android)
    else()
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia android)
    endif()

endfunction()

build_library("ggml-jni")
