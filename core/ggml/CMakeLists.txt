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

set(KANTV_MEDIA_SRC_DIR         ${CMAKE_SOURCE_DIR}/media/)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/ggml)
set(WHISPERCPP_SRC_DIR          ${GGML_SRC_DIR}/whispercpp/)
set(PREBUILT_LIB_PATH           ${GGML_SRC_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${GGML_SRC_DIR}/../../prebuilts/include/)
set(KANTV_GGMLJNI_SRC_DIR       ${GGML_SRC_DIR}/jni/)
set(LLAMACPP_SRC_DIR            ${GGML_SRC_DIR}/llamacpp/)
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


include_directories(${QNN_INC_PATH}/)
include_directories(${KANTV_MEDIA_SRC_DIR}/include) # kantv-asr.h

if (GGML_ENABLE_QNN)
    set(QNN_BACKEND_SRCS ${LLAMACPP_SRC_DIR}/ggml-qnn.cpp)
endif()

set(SOURCE_FILES
        #03-30-2024, upstream source code in llama.cpp is preferred
        ${LLAMACPP_SRC_DIR}/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml-backend.c
        ${LLAMACPP_SRC_DIR}/ggml-quants.c
        ${QNN_BACKEND_SRCS}
        ${LLAMACPP_SRC_DIR}/llama.cpp
        ${LLAMACPP_SRC_DIR}/unicode.cpp
        ${LLAMACPP_SRC_DIR}/sgemm.cpp
        ${LLAMACPP_SRC_DIR}/unicode-data.cpp
        ${LLAMACPP_SRC_DIR}/common/sampling.cpp
        ${LLAMACPP_SRC_DIR}/common/common.cpp
        ${LLAMACPP_SRC_DIR}/common/console.cpp
        ${LLAMACPP_SRC_DIR}/common/build-info.cpp
        ${LLAMACPP_SRC_DIR}/common/grammar-parser.cpp
        ${LLAMACPP_SRC_DIR}/common/json-schema-to-grammar.cpp

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/llm-inference.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/mnist-inference.cpp

        #05-25-2024, import MiniCPM-V(A GPT-4V Level Multimodal LLM), https://github.com/OpenBMB/MiniCPM-V
        ${KANTV_GGMLJNI_SRC_DIR}/clip.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv_io.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/minicpmv-cli.cpp

)

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

if (GGML_ENABLE_QNN)
add_definitions(-DGGML_USE_QNN)
else()
add_definitions(-DGGML_DISABLE_QNN)
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

#add_compile_options("-Wall" "-Werror" "-Wno-sign-compare")
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
