/*
 * Copyright (C) 2000-2014 the xine project
 *
 * Copyright (c) 2015 zhou.weiguo(zhouwg2000@gmail.com).customized myfifo for various project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 * contents:
 *
 * buffer_entry structure - serves as a transport encapsulation
 *   of the mpeg audio/video data through xine
 *
 * free buffer pool management routines
 *
 * FIFO buffer structures/routines
 */

#ifndef _MYFIFO_H_
#define _MYFIFO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <inttypes.h>

#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"

#define BUF_CONTROL_START           0x01000000
#define BUF_CONTROL_QUIT            0x01020000
#define BUF_CONTROL_RESET_DECODER   0x01080000

#define BUF_CONTROL_PARSE_PAT        0x010c0001
#define BUF_CONTROL_PARSE_PMT        0x010c0002
#define BUF_CONTROL_SEARCH_CHANNEL    0x010c0003

#define BUF_VIDEO_UNKNOWN    0x02ff0000 /* no decoder should handle this one */
#define BUF_VIDEO_BASE        0x02000000
#define BUF_VIDEO_MPEG        0x02000000
#define BUF_VIDEO_H264      0x024D0000


#define BUF_AUDIO_BASE      0x03000000
#define BUF_AUDIO_UNKNOWN   0x03ff0000 /* no decoder should handle this one */
#define BUF_AUDIO_MPEG      0x03010000
#define BUF_AUDIO_DTS       0x03050000
#define BUF_AUDIO_AC3       0x03410000
#define BUF_AUDIO_AAC       0x030e0000

typedef struct buf_element_s buf_element_t;


struct buf_element_s
{
    buf_element_t        *next;

    unsigned char        *mem;
    unsigned char        *content;   /* start of raw content in mem (without header etc) */

    /*weiguo added,make kantvrecord happy */
    int                   width;
    int                   height;
    int                   stride;
    enum AVPixelFormat    format;
    double                ptsInDouble ;

    unsigned int          channels;
    unsigned int          samplerate;
    enum AVSampleFormat   sampleformat;
    unsigned int          samplecounts;
    /* end added */

    int64_t               pts ;
    uint32_t              size ;     /* size of _content_                                     */
    int duration;
    int32_t               max_size;  /* size of pre-allocated memory pointed to by "mem"      */
    uint32_t              type;
    void (*free_buffer) (buf_element_t *buf);
    void                 *source;   /* pointer to source of this buffer for */
    int                     id;
} ;


typedef struct fifo_s fifo_t;
typedef struct fifo_s fifo_buffer_t;
struct fifo_s
{
    buf_element_t  *first, *last;

    int             fifo_size;
    uint32_t        fifo_data_size;
    void            *fifo_empty_cb_data;
    const           char *name;

    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;

    /*
     * functions to access this fifo:
     */

    void (*put) (fifo_t *fifo, buf_element_t *buf);

    buf_element_t *(*get) (fifo_t *fifo);

    buf_element_t *(*get_si) (fifo_t *fifo);

    void (*clear) (fifo_t *fifo) ;

    int (*size) (fifo_t *fifo);

    int (*num_free) (fifo_t *fifo);

    uint32_t (*data_size) (fifo_t *fifo);

    void (*destroy) (fifo_t *fifo);

    buf_element_t *(*buffer_alloc) (fifo_t *self);

    /* the same as buffer_alloc but may fail if none is available */
    buf_element_t *(*buffer_try_alloc) (fifo_t *self);

    /* the same as put but insert at the head of the fifo */
    void (*insert) (fifo_t *fifo, buf_element_t *buf);

    /*
     * private variables for buffer pool management
     */
    buf_element_t   *buffer_pool_top;    /* a stack actually */
    pthread_mutex_t  buffer_pool_mutex;
    pthread_cond_t   buffer_pool_cond_not_empty;
    int              buffer_pool_num_free;
    int              buffer_pool_capacity;
    int              buffer_pool_buf_size;
    void            *buffer_pool_base; /*used to free mem chunk */
} ;

/*
 * allocate and initialize new (empty) fifo buffer,
 * init buffer pool for it:
 * allocate num_buffers of buf_size bytes each
 */

fifo_t *fifo_new (const char *name, int num_buffers, uint32_t buf_size);


#ifdef __cplusplus
}
#endif

#endif
