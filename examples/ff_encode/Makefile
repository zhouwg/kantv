# Copyright (c) 2021-2023, zhou.weiguo(zhouwg2000@gmail.com)

# Copyright (c) 2021-2023, Project KanTV

# Copyright (c) 2024- KanTV Authors

# Description: Makefile for ff_encode to study how to implement kantvrecord in Project KanTV
#
#


ifndef PROJECT_ROOT_PATH
$(error undefined PROJECT_ROOT_PATH)
endif

ifndef FF_PREFIX
$(error undefined FF_PREFIX)
endif

ifndef BUILD_TARGET
$(error undefined BUILD_TARGET)
endif


CROSS=
CC=$(CROSS)gcc
CXX=$(CROSS)g++
STRIP=$(CROSS)strip

FF_SRC_PATH := ${PROJECT_ROOT_PATH}/external/ffmpeg/
FF_LIB_PATH := ${PROJECT_ROOT_PATH}/out/${BUILD_TARGET}/lib/

# common for x86,android,ios,wasm
G_LDFLAGS    = -L${FF_LIB_PATH} -lavformat -lavcodec -lx264 -lx265 -laom -lwebp -lsharpyuv -lvvenc -lvvdec -lSvtAv1Enc -lSvtAv1Dec -lswscale -lavutil -lswresample

# for x86
G_LDFLAGS += -lSDL2 -lasound -lXv -lX11 -lXext -lxcb -lXfixes -lXinerama -pthread -lm -latomic -lz -ldl -lstdc++


G_INC       := -I${FF_SRC_PATH}/ -I${FF_SRC_PATH}/libavutil  -I./
SRCS        := $(wildcard *.c)


TARGET = ff_encode_${BUILD_TARGET}

OBJS        :=$(addsuffix .o, $(basename $(SRCS)))


%.o:%.c  $(HDRS)
	$(CC)  $(G_CFLAGS) $(G_INC) -c $< -o $@

%.o:%.cpp  $(HDRS)
	$(CXX)  $(G_CFLAGS) $(G_INC) -c $< -o $@


all: $(OBJS)
	$(CXX)  -o $(TARGET) $(OBJS) $(G_LDFLAGS)
	$(STRIP) $(TARGET)
	/bin/cp -fv ${TARGET} ${FF_PREFIX}/bin/

.PHONY:clean
clean:
	@$(RM) -f $(OBJS)
	@$(RM) -f $(TARGET)
