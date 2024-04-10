# Copyright (c) 2024, zhou.weiguo(zhouwg2000@gmail.com)

# Copyright (c) 2024- KanTV Authors

# Description: build libggml-jni.so for target Android

cmake_minimum_required(VERSION 3.22.1) # make llamacpp happy
project(ggml-jni)

set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(TARGET_XIAOMI14    OFF) # set it to ON to get much better performance on Xiaomi 14 or Qualcomm Snapdragon 8 Gen 3 SoC based Android phone
set(GGML_JNI_QNN       ON)  # set it to ON to enable QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK on Qualcomm SoC based Android phone

#03-05-2024, PoC#64: English realtime subtitle for English on-line TV by whisper.cpp
set(WHISPERCPP_SRC_DIR          ${CMAKE_SOURCE_DIR}/whispercpp/)
set(PREBUILT_LIB_PATH           ${CMAKE_SOURCE_DIR}/../../cdeosplayer/kantv/src/main/jniLibs/${ANDROID_ABI}/)
set(PREBUILT_INC_PATH           ${CMAKE_SOURCE_DIR}/../../prebuilts/include/)
set(KANTV_GGMLJNI_SRC_DIR       ${CMAKE_SOURCE_DIR}/jni)
set(GGML_SRC_DIR                ${CMAKE_SOURCE_DIR}/)

#03-26-2024, integrate llama.cpp to kantv
set(LLAMACPP_SRC_DIR            ${CMAKE_SOURCE_DIR}/llamacpp/)


set(SDCPP_SRC_DIR                ${CMAKE_SOURCE_DIR}/stablediffusioncpp/)

#03-29-2024, PoC#121:Add Qualcomm mobile SoC native backend for GGML
set(QNN_INC_PATH  ${CMAKE_SOURCE_DIR}/../../prebuilts/include/QNN)
set(QNN_LIB_PATH  ${CMAKE_SOURCE_DIR}/../../prebuilts/lib/)
set(QNN_SRC_DIR   ${CMAKE_SOURCE_DIR}/qnnsample/)

include_directories(${QNN_INC_PATH}/)
include_directories(${QNN_SRC_DIR}/)
include_directories(${QNN_SRC_DIR}/Utils)
include_directories(${QNN_SRC_DIR}/Log)
include_directories(${QNN_SRC_DIR}/WrapperUtils)
include_directories(${QNN_SRC_DIR}/PAL/include)

aux_source_directory(${GGML_SRC_DIR}/stablediffusioncpp/          stablediffusioncpp)
include_directories(${SDCPP_SRC_DIR})

set(SOURCE_FILES
        #03-30-2024, upstream source code in llama.cpp is preferred
        ${LLAMACPP_SRC_DIR}/ggml.c
        ${LLAMACPP_SRC_DIR}/ggml-alloc.c
        ${LLAMACPP_SRC_DIR}/ggml-backend.c
        ${LLAMACPP_SRC_DIR}/ggml-quants.c

        ${WHISPERCPP_SRC_DIR}/whisper.cpp

        ${LLAMACPP_SRC_DIR}/llama.cpp
        ${LLAMACPP_SRC_DIR}/unicode.cpp
        ${LLAMACPP_SRC_DIR}/unicode-data.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni.c
        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-jni-impl-external.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/ggml-qnn.cpp

        ${KANTV_GGMLJNI_SRC_DIR}/saver_output.c

        ${KANTV_GGMLJNI_SRC_DIR}/tinywav.c
        ${KANTV_GGMLJNI_SRC_DIR}/sampling.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/common.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/grammar-parser.cpp
        ${KANTV_GGMLJNI_SRC_DIR}/llama-bench.cpp

        ${QNN_SRC_DIR}/QnnSampleMain.cpp
        ${QNN_SRC_DIR}/QnnSampleApp.cpp
        ${QNN_SRC_DIR}/Log/Logger.cpp
        ${QNN_SRC_DIR}/Log/LogUtils.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/Directory.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/DynamicLoading.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/FileOp.cpp
        ${QNN_SRC_DIR}/PAL/src/linux/Path.cpp
        ${QNN_SRC_DIR}/PAL/src/common/GetOpt.cpp
        ${QNN_SRC_DIR}/PAL/src/common/StringOp.cpp
        ${QNN_SRC_DIR}/Utils/DataUtil.cpp
        ${QNN_SRC_DIR}/Utils/DynamicLoadUtil.cpp
        ${QNN_SRC_DIR}/Utils/IOTensor.cpp
        ${QNN_SRC_DIR}/Utils/QnnSampleAppUtils.cpp
        ${QNN_SRC_DIR}/WrapperUtils/QnnWrapperUtils.cpp


        ${stablediffusioncpp}
)


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

include_directories(${WHISPERCPP_SRC_DIR}/)
include_directories(${GGML_SRC_DIR}/)
include_directories(${KANTV_GGMLJNI_SRC_DIR}/)
include_directories(${PREBUILT_INC_PATH}/)

#re-use the ggml.h in subdirectory llamacpp/ggml.h to avoid NDK complain "error: redefinition of 'ggml_status'"
include_directories(${LLAMACPP_SRC_DIR}/)

add_definitions(-DTARGET_ANDROID)
add_definitions(-D__ARM_NEON)

add_definitions(-DNDEBUG)
add_definitions(-O3)

if (TARGET_XIAOMI14)

#weiguo:2024-03-11
# the below special build optimization ONLY validated ok on Xiaomi 14
# works very well on Xiaomi 14 and got best ASR performance until now(less then 1 second)
# manully enable it for Xiaomi 14
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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options("-s")
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    add_compile_options("-O3"
            "-ffunction-sections"
            "-fdata-sections"
            "-frtti"
            "-Wno-unused-command-line-argument")
endif()

if (GGML_JNI_QNN)
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


add_library(QNNSaver
        SHARED
        IMPORTED)

set_target_properties(QNNSaver
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libQnnSaver.so)

add_library(QNNCpu
        SHARED
        IMPORTED)

set_target_properties(QNNCpu
        PROPERTIES
        IMPORTED_LOCATION
        ${PREBUILT_LIB_PATH}/libQnnCpu.so)

function(build_library target_name)
    add_library(
            ${target_name}
            SHARED
            ${SOURCE_FILES}
    )

    if (GGML_JNI_QNN)
        file(GLOB allQNNLibs "${PREBUILT_LIB_PATH}/libQnn*.so")
        #target_link_libraries(${target_name} ${LOG_LIB} kantvmedia kantvffmpeg android ${allQNNLibs})
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia kantvffmpeg QNNCpu QNNSaver android )
    else()
        target_link_libraries(${target_name} ${LOG_LIB} kantvmedia kantvffmpeg android)
    endif()

endfunction()

build_library("ggml-jni")
