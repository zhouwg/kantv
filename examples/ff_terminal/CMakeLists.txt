project(ff_terminal)

SET(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DPLATFORM_CDELINUX)
add_definitions(-D__DEBUG_FB)

set(FF_SRC_PATH ${CMAKE_SOURCE_DIR}/../../external/ffmpeg/)
set(FF_INC_PATH ${CMAKE_SOURCE_DIR}/../../external/ffmpeg/)
set(FF_LIB_PATH ${CMAKE_SOURCE_DIR}/../../out/linux/lib/)

message("FF_INC_PATH     : ${FF_INC_PATH}")
message("FF_SRC_PATH     : ${FF_SRC_PATH}")
message("FF_LIB_PATH     : ${FF_LIB_PATH}")

add_executable(ff-terminal main.c fb.c ${FF_SRC_PATH}/libavutil/cde_log.c terminal/get_key.c terminal/terminal.c)

include_directories(${FF_INC_PATH}/)
include_directories(./)
include_directories(./terminal)

install(
        TARGETS
        ff-terminal
        RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
)
