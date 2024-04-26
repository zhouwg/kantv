cmake_minimum_required(VERSION 3.22.1)
project(ggml-qnn-test)

set(CMAKE_VERBOSE_MAKEFILE          on)
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#set to ON if target Android phone is based on Qualcomm Snapdragon 8 Gen 3
set(TARGET_SNAPDRAGON_8_GEN3        OFF)

set(LLAMACPP_SRC_PATH               ${PROJECT_ROOT_PATH})
set(QNN_INC_PATH                    ${QNN_SDK_PATH}/include/QNN)
set(QNN_LIB_PATH                    ${QNN_SDK_PATH}/lib/aarch64-android)

include_directories(${QNN_INC_PATH})
include_directories(${LLAMACPP_SRC_PATH})
include_directories(${LLAMACPP_SRC_PATH}/common)
include_directories(${LLAMACPP_SRC_PATH}/../../../external/ffmpeg)

set(SOURCE_FILES
        ${LLAMACPP_SRC_PATH}/ggml.c
        ${LLAMACPP_SRC_PATH}/ggml-alloc.c
        ${LLAMACPP_SRC_PATH}/ggml-backend.c
        ${LLAMACPP_SRC_PATH}/ggml-quants.c
        ${LLAMACPP_SRC_PATH}/ggml-qnn.cpp
        ${LLAMACPP_SRC_PATH}/tests/test-backend-ops.cpp
        ${LLAMACPP_SRC_PATH}/../../../external/ffmpeg/libavutil/cde_log.c
)


message("PROJECT_ROOT_PATH    : ${PROJECT_ROOT_PATH}")
message("LLAMACPP_SRC_PATH    : ${LLAMACPP_SRC_PATH}")
message("QNN_SDK_PATH         : ${QNN_SDK_PATH}")
message("QNN_INC_PATH         : ${QNN_INC_PATH}")
message("QNN_LIB_PATH         : ${QNN_LIB_PATH}")
message("target name          : ${TARGET_NAME}")


add_definitions(-D__ARM_NEON)
add_definitions(-DGGML_USE_QNN)

if(CMAKE_BUILD_TYPE STREQUAL "release")
add_definitions(-DNDEBUG)
add_definitions(-O3)
endif()

if (TARGET_SNAPDRAGON_8_GEN3)
# the below build optimization only works well on Qualcomm SM8650-AB Snapdragon 8 Gen 3
add_definitions(-march=armv8.7-a)
add_definitions(-mcpu=cortex-x1)
add_definitions(-mtune=cortex-x1)

else()
# the below build optimization might be works well on ALL mainstream Android phone based on Qualcomm mobile SoC
add_definitions(-mcpu=cortex-a72)

endif()

add_compile_options("-Wall" "-Wno-sign-compare")

find_library(LOG_LIB log)

link_libraries(${LOG_LIB} android)

add_executable(${TARGET_NAME}
    ${SOURCE_FILES}
)
