/*
 * Copyright (C) 2000-2014 the xine project
 *
 * Copyright (c) 2015 zhouwg(zhouwg2000@gmail.com).customized myfifo for various project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "cde_log.h"
#include "myfifo.h"


#define BUF_MAJOR_MASK              0xFF000000
#define BUF_CONTROL_BASE            0x01000000

static void dako_xfree(void *ptr)
{
    if (NULL != ptr)
    {
        free(ptr);
        ptr = NULL;
    }
}

static void *dako_xmalloc(size_t size)
{
    void *ptr;

    /* prevent dako_xmalloc(0) of possibly returning NULL */
    if ( !size )
        size++;

    if ((ptr = calloc(1, size)) == NULL)
    {
        LOGJ("malloc(%d) failed: %s.\n",size, strerror(errno));
        return NULL;
    }

    return ptr;
}

static void *dako_xmalloc_aligned(size_t alignment, size_t size, void **base)
{

    char *ptr;

    *base = ptr = dako_xmalloc (size+alignment);

    while ((size_t) ptr % alignment)
        ptr++;

    return ptr;
}


/*
 * put a previously allocated buffer element back into the buffer pool
 */
//fifo_buffer_put
//just make ctags happy
static void my_buffer_pool_free (buf_element_t *element)
{
}
static void buffer_pool_free (buf_element_t *element)
{

    fifo_t *this = (fifo_t *) element->source;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    element->next = this->buffer_pool_top;
    this->buffer_pool_top = element;

    this->buffer_pool_num_free++;
    if (this->buffer_pool_num_free > this->buffer_pool_capacity)
    {
        LOGJ("kantv-ffmpeg:buffer: Their has been a fatal error: TOO MANY FREE's\n");
    }

    pthread_cond_signal (&this->buffer_pool_cond_not_empty);

    pthread_mutex_unlock (&this->buffer_pool_mutex);
}

/*
 * allocate a buffer from buffer pool
 */

//buffer_pool_free
static buf_element_t *buffer_pool_alloc (fifo_t *this)
{

    buf_element_t *buf;
    int i;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    //LOGJ("this->buffer_pool_num_free %d\n", this->buffer_pool_num_free);
    /* we always keep one free buffer for emergency situations like
     * decoder flushes that would need a buffer in buffer_pool_try_alloc() */
    while (this->buffer_pool_num_free < 2)
    {
        //LOGJ("buffer num < 2, waiting....\n");
        pthread_cond_wait (&this->buffer_pool_cond_not_empty, &this->buffer_pool_mutex);
    }

    buf = this->buffer_pool_top;
    this->buffer_pool_top = this->buffer_pool_top->next;
    this->buffer_pool_num_free--;



    //2009-11-19, move pthread_mutex_unlock to last
    /* set sane values to the newly allocated buffer */
    buf->content = buf->mem; /* 99% of demuxers will want this */
    buf->size = 0;
    //2009-12-8,zhouwg,add it
    buf->type = 0;
    pthread_mutex_unlock (&this->buffer_pool_mutex);

    return buf;
}

/*
 * allocate a buffer from buffer pool - may fail if none is available
 */

static buf_element_t *buffer_pool_try_alloc (fifo_t *this)
{

    buf_element_t *buf;

    pthread_mutex_lock (&this->buffer_pool_mutex);

    if (this->buffer_pool_top)
    {

        buf = this->buffer_pool_top;
        this->buffer_pool_top = this->buffer_pool_top->next;
        this->buffer_pool_num_free--;

    }
    else
    {

        buf = NULL;

    }

    pthread_mutex_unlock (&this->buffer_pool_mutex);

    /* set sane values to the newly allocated buffer */
    if ( buf )
    {
        buf->content = buf->mem; /* 99% of demuxers will want this */
        buf->size = 0;
    }
    return buf;
}


/*
 * append buffer element to fifo buffer
 */
//fifo_new
//fifo_buffer_get
static void fifo_buffer_put (fifo_t *fifo, buf_element_t *element)
{
    int i;

    pthread_mutex_lock (&fifo->mutex);


    if (fifo->last)
        fifo->last->next = element;
    else
        fifo->first = element;

    fifo->last = element;
    element->next = NULL;
    fifo->fifo_size++;
    fifo->fifo_data_size += element->size;

    LOGJ("put:index %d, fifo->size is %d, this->buffer_pool_num_free %d\n", element->id, fifo->fifo_size, fifo->buffer_pool_num_free);
    pthread_cond_signal (&fifo->not_empty);

    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * append buffer element to fifo buffer
 */
static void dummy_fifo_buffer_put (fifo_t *fifo, buf_element_t *element)
{
    int i;

    pthread_mutex_lock (&fifo->mutex);

    pthread_mutex_unlock (&fifo->mutex);

    element->free_buffer(element);
}

/*
 * insert buffer element to fifo buffer (demuxers MUST NOT call this one)
 */
static void fifo_buffer_insert (fifo_t *fifo, buf_element_t *element)
{

    pthread_mutex_lock (&fifo->mutex);

    element->next = fifo->first;
    fifo->first = element;

    if ( !fifo->last )
        fifo->last = element;

    fifo->fifo_size++;
    fifo->fifo_data_size += element->size;

    pthread_cond_signal (&fifo->not_empty);

    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * insert buffer element to fifo buffer (demuxers MUST NOT call this one)
 */
static void dummy_fifo_buffer_insert (fifo_t *fifo, buf_element_t *element)
{

    element->free_buffer(element);
}

/*
 * get element from fifo buffer
 */
//fifo_buffer_put
static buf_element_t *fifo_buffer_get (fifo_t *fifo)
{
    int i;
    buf_element_t *buf;

    pthread_mutex_lock (&fifo->mutex);

#if 0
    while (fifo->first==NULL)
    {
        LOGJ("fifo_buffer_get:fifo_size is %d, queue is empty, waiting...\n", fifo->fifo_size);
        pthread_cond_wait (&fifo->not_empty, &fifo->mutex);
    }
#else
    if (fifo->first == NULL)
    {
        //LOGJ("fifo_buffer_get:fifo_size is %d, queue is empty, return NULL\n", fifo->fifo_size);
        pthread_mutex_unlock (&fifo->mutex);
        return NULL;
    }
#endif

    buf = fifo->first;

    fifo->first = fifo->first->next;
    if (fifo->first==NULL)
        fifo->last = NULL;

    fifo->fifo_size--;
    fifo->fifo_data_size -= buf->size;
    LOGJ("get:index %d, fifo->size is %d, this->buffer_pool_num_free %d\n", buf->id, fifo->fifo_size, fifo->buffer_pool_num_free);

    pthread_mutex_unlock (&fifo->mutex);

    return buf;
}

static buf_element_t *fifo_buffer_get_si(fifo_t *fifo)
{
    int i;
    buf_element_t *buf;

    pthread_mutex_lock (&fifo->mutex);

#if 0
    while (fifo->first==NULL)
    {
        LOGJ("fifo_buffer_get:fifo_size is %d, queue is empty, waiting...\n", fifo->fifo_size);
        pthread_cond_wait (&fifo->not_empty, &fifo->mutex);
    }
#else
    if (fifo->first == NULL)
    {
        pthread_mutex_unlock (&fifo->mutex);
        return NULL;
    }
#endif

    buf = fifo->first;

    fifo->first = fifo->first->next;
    if (fifo->first==NULL)
        fifo->last = NULL;

    fifo->fifo_size--;
    fifo->fifo_data_size -= buf->size;

    pthread_mutex_unlock (&fifo->mutex);

    return buf;
}



/*
 * clear buffer (put all contained buffer elements back into buffer pool)
 */
static void fifo_buffer_clear (fifo_t *fifo)
{

    buf_element_t *buf, *next, *prev;

    pthread_mutex_lock (&fifo->mutex);

    buf = fifo->first;
    prev = NULL;

    while (buf != NULL)
    {

        next = buf->next;

        if ((buf->type & BUF_MAJOR_MASK) !=  BUF_CONTROL_BASE)
        {
            /* remove this buffer */

            if (prev)
                prev->next = next;
            else
                fifo->first = next;

            if (!next)
                fifo->last = prev;

            fifo->fifo_size--;
            fifo->fifo_data_size -= buf->size;

            buf->free_buffer(buf);
        }
        else
            prev = buf;

        buf = next;
    }

    LOGJ("Free buffers after clear: %d\n", fifo->buffer_pool_num_free);
    pthread_mutex_unlock (&fifo->mutex);
}

/*
 * Return the number of elements in the fifo buffer
 */
static int fifo_buffer_size (fifo_t *this)
{
    int size;

    pthread_mutex_lock(&this->mutex);
    size = this->fifo_size;
    pthread_mutex_unlock(&this->mutex);

    return size;
}

/*
 * Return the amount of the data in the fifo buffer
 */
static uint32_t fifo_buffer_data_size (fifo_t *this)
{
    uint32_t data_size;

    pthread_mutex_lock(&this->mutex);
    data_size = this->fifo_data_size;
    pthread_mutex_unlock(&this->mutex);

    return data_size;
}

/*
 * Return the number of free elements in the pool
 */
static int fifo_buffer_num_free (fifo_t *this)
{
    int buffer_pool_num_free;

    pthread_mutex_lock(&this->mutex);
    buffer_pool_num_free = this->buffer_pool_num_free;
    pthread_mutex_unlock(&this->mutex);

    return buffer_pool_num_free;
}

/*
 * Destroy the buffer
 */
static void fifo_buffer_dispose (fifo_t *this)
{
    ENTER_FUNC();
    buf_element_t *buf, *next;
    int received = 0;

    this->clear( this );
    buf = this->buffer_pool_top;

    while (buf != NULL)
    {

        next = buf->next;

        free (buf);
        received++;

        buf = next;
    }

    while (received < this->buffer_pool_capacity)
    {

        buf = this->get(this);

        free(buf);
        received++;
    }

    free (this->buffer_pool_base);
    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->not_empty);
    pthread_mutex_destroy(&this->buffer_pool_mutex);
    pthread_cond_destroy(&this->buffer_pool_cond_not_empty);
    free(this->name);
    free (this);
    LEAVE_FUNC();
}


/*
 * allocate and initialize new (empty) fifo buffer
 */
fifo_t *fifo_new (const char *name, int num_buffers, uint32_t buf_size)
{

    fifo_t *this;
    int            i;
    //int            alignment = 2048;
    int            alignment = 1;
    char          *multi_buffer = NULL;

    this = dako_xmalloc (sizeof (fifo_t));

    this->name                = strdup(name);
    this->first               = NULL;
    this->last                = NULL;
    this->fifo_size           = 0;
    this->put                 = fifo_buffer_put;
    this->insert              = fifo_buffer_insert;
    this->get                 = fifo_buffer_get;
    this->get_si              = fifo_buffer_get_si;
    this->clear               = fifo_buffer_clear;
    this->size                = fifo_buffer_size;
    this->num_free            = fifo_buffer_num_free;
    this->data_size           = fifo_buffer_data_size;
    this->destroy             = fifo_buffer_dispose;
    pthread_mutex_init (&this->mutex, NULL);
    pthread_cond_init (&this->not_empty, NULL);

    /*
     * init buffer pool, allocate nNumBuffers of buf_size bytes each
     */


    if (buf_size % alignment != 0)
        buf_size += alignment - (buf_size % alignment);

    LOGJ("[%s]Allocating %d buffers of %ld bytes in one chunk (alignment = %d)\n", name, num_buffers, (long int) buf_size, alignment);
    LOGI("[%s]Allocating %d Mbytes memory(alignment = %d)\n", name, (num_buffers * buf_size) / 1024 / 1024, alignment);

    multi_buffer = dako_xmalloc_aligned (alignment, num_buffers * buf_size, &this->buffer_pool_base);

    assert(multi_buffer);
    this->buffer_pool_top = NULL;

    pthread_mutex_init (&this->buffer_pool_mutex, NULL);
    pthread_cond_init (&this->buffer_pool_cond_not_empty, NULL);

    this->buffer_pool_num_free  = 0;
    this->buffer_pool_capacity  = num_buffers;
    this->buffer_pool_buf_size  = buf_size;
    this->buffer_alloc     = buffer_pool_alloc;
    this->buffer_try_alloc = buffer_pool_try_alloc;

    for (i = 0; i<num_buffers; i++)
    {
        buf_element_t *buf;

        buf = dako_xmalloc (sizeof (buf_element_t));

        buf->id = i;
        buf->mem = multi_buffer;
        multi_buffer += buf_size;

        buf->max_size    = buf_size;
        buf->free_buffer = buffer_pool_free;
        buf->source      = this;

        buffer_pool_free (buf);
    }
    return this;
}
