message(STATUS "Using QNN backend")

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    find_library(LOG_LIB log)

    add_library(cdsprpc
        SHARED
        IMPORTED)
    set_target_properties(cdsprpc
        PROPERTIES
        IMPORTED_LOCATION
        /opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/remote/ship/android_aarch64/libcdsprpc.so)

    add_library(hexagon_skel
        SHARED
        IMPORTED)
    set_target_properties(hexagon_skel
        PROPERTIES
        IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/ggml/src/ggml-qnn/libcalculator.so)

    set(QNN_LINK_LIBRARIES ${LOG_LIB} cdsprpc hexagon_skel)
    set(QNN_DEFAULT_LIB_SEARCH_PATH "/data/local/tmp/" CACHE STRING "customized library search path for QNN backend")
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/incs)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/incs/stddef)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/incs)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/rpcmem/inc)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/remote/ship/android_Debug_aarch64)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/incs/qnx)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/libs/common/qnx/ship/android_Debug_aarch64)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/utils/examples)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/ipc/fastrpc/rtld/ship/android_aarch64)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/libs/atomic/inc)
    include_directories(/opt/qcom/Hexagon_SDK/6.2.0.1/libs/atomic/android_Debug_aarch64/ship)
    include_directories(${CMAKE_SOURCE_DIR}/ggml/src/ggml-qnn/)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QNN_DEFAULT_LIB_SEARCH_PATH "C:\\" CACHE STRING "customized library search path for QNN backend")
elseif(CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    set(QNN_DEFAULT_LIB_SEARCH_PATH "/cygdrive/c/qairt/2.31.0.250130/" CACHE STRING "customized library search path for QNN backend")
else()
    message(FATAL_ERROR "QNN now only available on Android and Windows(Windows on ARM)")
endif()

if(NOT DEFINED GGML_QNN_SDK_PATH)
# try read from environment variable
    if(DEFINED ENV{QNN_SDK_PATH})
        set(GGML_QNN_SDK_PATH $ENV{QNN_SDK_PATH})
    else()
        message(FATAL_ERROR "GGML_QNN_SDK_PATH not defined")
    endif()
endif()

message("QNN_SDK_PATH: ${GGML_QNN_SDK_PATH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGGML_USE_QNN")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

file(GLOB QNN_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.c")
ggml_add_backend_library(ggml-qnn ${QNN_SOURCES})

target_include_directories(ggml-qnn PRIVATE ${GGML_QNN_SDK_PATH}/include/QNN ${HEXAGON_SDK_PATH} ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(ggml-qnn PRIVATE ${QNN_LINK_LIBRARIES})

string(REGEX REPLACE "/$" "" GGML_QNN_DEFAULT_LIB_SEARCH_PATH "${QNN_DEFAULT_LIB_SEARCH_PATH}")
target_compile_definitions(ggml-qnn PRIVATE GGML_QNN_DEFAULT_LIB_SEARCH_PATH="${QNN_DEFAULT_LIB_SEARCH_PATH}/")
