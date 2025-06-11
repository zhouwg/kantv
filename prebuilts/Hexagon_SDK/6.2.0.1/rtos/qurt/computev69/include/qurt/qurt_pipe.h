#ifndef QURT_PIPE_H
#define QURT_PIPE_H
/**
  @file qurt_pipe.h 
  @brief  Prototypes of the pipe interface API  
   This is a pipe or message queue
	 It blocks when too full (send) or empty (receive).
	 Unless using a nonblocking option, all datagrams are 64 bits.

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

 Copyright (c) 2021,2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#include <stddef.h>
#include <qurt_mutex.h>
#include <qurt_sem.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup pipe_types
@{ */
/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
#define QURT_PIPE_MAGIC  0xF1FEF1FE        /**< Magic. */
#define QURT_PIPE_ATTR_MEM_PARTITION_RAM 0 /**< RAM. */
#define QURT_PIPE_ATTR_MEM_PARTITION_TCM 1 /**< TCM. */

/*=============================================================================
                        TYPEDEFS
=============================================================================*/
/** QuRT pipe data values type. */
typedef unsigned long long int qurt_pipe_data_t;

/** QuRT pipe type.*/
typedef struct {
    /** @cond */
	qurt_mutex_t pipe_lock;
	qurt_sem_t senders;
	qurt_sem_t receiver;
	unsigned int size;
	unsigned int sendidx;
	unsigned int recvidx;
	void (*lock_func)(qurt_mutex_t *);
	void (*unlock_func)(qurt_mutex_t *);
    int (*try_lock_func)(qurt_mutex_t *);
    void (*destroy_lock_func)(qurt_mutex_t *);
	unsigned int magic;
	qurt_pipe_data_t *data;
    /** @endcond */
} qurt_pipe_t;

/**  QuRT pipe attributes type. */
typedef struct {
  /** @cond */
  qurt_pipe_data_t *buffer;
  unsigned int elements;
  unsigned char mem_partition;
  /** @endcond */
} qurt_pipe_attr_t;

/** @} */ /* end_addtogroup pipe_types */
/*=============================================================================
                        FUNCTIONS
=============================================================================*/
/**@ingroup func_qurt_pipe_attr_init
  @xreflabel{hdr:qurt_pipe_attr_init}
  Initializes the structure that sets the pipe attributes when a pipe is created.

  After an attribute structure is initialized, the individual attributes in the structure are
  explicitly set using the pipe attribute operations.

  The attribute structure is assigned the following default values: \n
  - buffer -- 0 \n
  - elements -- 0 \n
  - mem_partition -- #QURT_PIPE_ATTR_MEM_PARTITION_RAM
  
  @datatypes
  #qurt_pipe_attr_t
 
  @param[in,out] attr Pointer to the pipe attribute structure.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_pipe_attr_init(qurt_pipe_attr_t *attr)
{
  attr->buffer = NULL;
  attr->elements = 0;
  attr->mem_partition = QURT_PIPE_ATTR_MEM_PARTITION_RAM;
}

/**@ingroup func_qurt_pipe_attr_set_buffer
  @xreflabel{sec:qurt_pipe_attr_set_buffer}
  Sets the pipe buffer address attribute.\n
  Specifies the base address of the memory area to use for the data buffer of a pipe.

  The base address and size (Section @xref{sec:qurt_pipe_attr_set_elements}) specify the 
  memory area used as a pipe data buffer. The user is responsible for allocating the 
  memory area used for the buffer.

  @datatypes
  #qurt_pipe_attr_t \n
  #qurt_pipe_data_t
  
  @param[in,out] attr Pointer to the pipe attribute structure.
  @param[in] buffer   Pointer to the buffer base address.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_pipe_attr_set_buffer(qurt_pipe_attr_t *attr, qurt_pipe_data_t *buffer)
{
  attr->buffer = buffer;
}

/**@ingroup func_qurt_pipe_attr_set_elements
  @xreflabel{sec:qurt_pipe_attr_set_elements}
  Specifies the length of the memory area to use for the data buffer of a pipe. 
  
  The length is expressed in terms of the number of 64-bit data elements that 
  can be stored in the buffer. 
  
  The base address (Section @xref{sec:qurt_pipe_attr_set_buffer}) and size specify 
  the memory area used as a pipe data buffer. The user is responsible for 
  allocating the memory area used for the buffer.

  @datatypes
  #qurt_pipe_attr_t

  @param[in,out] attr Pointer to the pipe attribute structure.
  @param[in] elements Pipe length (64-bit elements). 

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_pipe_attr_set_elements(qurt_pipe_attr_t *attr, unsigned int elements)
{
  attr->elements = elements;
}

/**@ingroup func_qurt_pipe_attr_set_buffer_partition
  @xreflabel{sec:qurt_pipe_attr_set_buffer_partition}
  Specifies the memory type where a pipe's buffer is allocated.
  Allocate pipes in RAM or TCM/LPM.
 
  @note1hang If a pipe is specified as allocated in TCM/LPM, it must be created
  with the qurt_pipe_init() operation. The qurt_pipe_create() operation results in an error.

  @datatypes
  #qurt_pipe_attr_t

  @param[in,out] attr Pointer to the pipe attribute structure.
  @param[in] mem_partition Pipe memory partition. Values: \n
             - #QURT_PIPE_ATTR_MEM_PARTITION_RAM -- Pipe resides in RAM \n
             - #QURT_PIPE_ATTR_MEM_PARTITION_TCM -- Pipe resides in TCM/LCM @tablebulletend

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_pipe_attr_set_buffer_partition(qurt_pipe_attr_t *attr, unsigned char mem_partition)
{
  attr->mem_partition = mem_partition;
}

/**@ingroup func_qurt_pipe_create
  Creates a pipe.\n
  Allocates a pipe object and its associated data buffer, and initializes the pipe object.

  @note1hang The buffer address and size stored in the attribute structure specify how the
             pipe data buffer is allocated.
  
  @note1cont If a pipe is specified as allocated in TCM/LPM, it must be created
             using the qurt_pipe_init() operation. The qurt_pipe_create() operation results in an error.
  
  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_attr_t
  
  @param[out] pipe  Pointer to the created pipe object.
  @param[in]  attr  Pointer to the attribute structure used to create the pipe.

  @return 
  #QURT_EOK -- Pipe created. \n
  #QURT_EFAILED -- Pipe not created. \n
  #QURT_ENOTALLOWED -- Pipe cannot be created in TCM/LPM.

  @dependencies
  None.
 */
int qurt_pipe_create(qurt_pipe_t **pipe, qurt_pipe_attr_t *attr);

/**@ingroup func_qurt_pipe_init
  Initializes a pipe object using an existing data buffer.

  @note1hang The buffer address and size stored in the attribute structure must 
             specify a data buffer that the user has already allocated.

  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_attr_t
  
  @param[out] pipe Pointer to the pipe object to initialize.
  @param[in] attr  Pointer to the pipe attribute structure used to initialize the pipe.

  @return 
  #QURT_EOK -- Success. \n
  #QURT_EFAILED -- Failure.

  @dependencies
  None.
 */
int qurt_pipe_init(qurt_pipe_t *pipe, qurt_pipe_attr_t *attr);

/**@ingroup func_qurt_pipe_destroy
  @xreflabel{sec:qurt_pipe_destroy}
  Destroys the specified pipe.

  @note1hang Pipes must be destroyed when they are no longer in use. Failure 
             to do this causes resource leaks in the QuRT kernel.
             Pipes must not be destroyed while they are still in use. If this 
             occurs, the behavior of QuRT is undefined.

  @datatypes
  #qurt_pipe_t
  
  @param[in] pipe Pointer to the pipe object to destroy.

  @return
  None.

  @dependencies
  None.
 */
void qurt_pipe_destroy(qurt_pipe_t *pipe); 

/**@ingroup func_qurt_pipe_delete
  Deletes the pipe.\n
  Destroys the specified pipe (Section @xref{sec:qurt_pipe_destroy}) and deallocates the pipe object and its
  associated data buffer.

  @note1hang Delete pipes only if they were created using qurt_pipe_create
             (and not qurt_pipe_init). Otherwise the behavior of QuRT is undefined. \n
  @note1cont Pipes must be deleted when they are no longer in use. Failure to do this 
             causes resource leaks in the QuRT kernel.\n
  @note1cont Pipes must not be deleted while they are still in use. If this occurs, the
             behavior of QuRT is undefined. 

  @datatypes
  #qurt_pipe_t
  
  @param[in] pipe Pointer to the pipe object to destroy.

  @return 
  None.

  @dependencies
  None.
 */
void qurt_pipe_delete(qurt_pipe_t *pipe);

/**@ingroup func_qurt_pipe_send
  Writes a data item to the specified pipe. \n
  If a thread writes to a full pipe, it is suspended on the pipe. When another thread reads
  from the pipe, the suspended thread is awakened and can then write data to the pipe.

  Pipe data items are defined as 64-bit values. Pipe writes are limited to transferring a single
  64-bit data item per operation.

  @note1hang Transfer data items larger than 64 bits by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_data_t
  
  @param[in] pipe Pointer to the pipe object to write to.
  @param[in] data Data item to write.

  @return
  None.

  @dependencies
  None.
*/
void qurt_pipe_send(qurt_pipe_t *pipe, qurt_pipe_data_t data);

/**@ingroup func_qurt_pipe_receive
  Reads a data item from the specified pipe.

  If a thread reads from an empty pipe, it is suspended on the pipe. When another thread
  writes to the pipe, the suspended thread is awakened and can then read data from the pipe.
  Pipe data items are defined as 64-bit values. Pipe reads are limited to transferring a single
  64-bit data item per operation.

  @note1hang Transfer data items larger than 64 bits by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t
  
  @param[in] pipe Pointer to the pipe object to read from.

  @return
  Integer containing the 64-bit data item from pipe.

  @dependencies
  None.
*/
qurt_pipe_data_t qurt_pipe_receive(qurt_pipe_t *pipe);

/**@ingroup func_qurt_pipe_try_send
  Writes a data item to the specified pipe (without suspending the thread if the pipe is full).\n

  If a thread writes to a full pipe, the operation returns immediately with success set to -1.
  Otherwise, success is always set to 0 to indicate a successful write operation.

  Pipe data items are defined as 64-bit values. Pipe writes are limited to transferring a single
  64-bit data item per operation.

  @note1hang Transfer data items larger than 64 bits  by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_data_t
  
  @param[in] pipe Pointer to the pipe object to write to.
  @param[in] data Data item to write.

  @return
  0 -- Success. \n
  -1 -- Failure (pipe full).

  @dependencies
  None.
*/ 
int qurt_pipe_try_send(qurt_pipe_t *pipe, qurt_pipe_data_t data);

/**@ingroup func_qurt_pipe_try_receive
  Reads a data item from the specified pipe (without suspending the thread if the pipe is
  empty).\n
  If a thread reads from an empty pipe, the operation returns immediately with success set
  to -1. Otherwise, success is always set to 0 to indicate a successful read operation.\n

  Pipe data items are defined as 64-bit values. Pipe reads are limited to transferring a single
  64-bit data item per operation.

  @note1hang Transfer data items larger than 64 bits by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t
  
  @param[in] pipe     Pointer to the pipe object to read from.
  @param[out] success Pointer to the operation status result.

  @return
  Integer containing a 64-bit data item from pipe.

  @dependencies
  None.
*/
qurt_pipe_data_t qurt_pipe_try_receive(qurt_pipe_t *pipe, int *success);

/**@ingroup func_qurt_pipe_receive_cancellable  
  Reads a data item from the specified pipe (with suspend), cancellable.

  If a thread reads from an empty pipe, it is suspended on the pipe. When another thread
  writes to the pipe, the suspended thread is awakened and can then read data from the pipe.
  The operation is cancelled if the user process of the calling thread is killed, 
  or if the calling thread must finish its current QDI invocation and return to user space.
  Root pd thread can use this api to wait on pipe for receiving and gets resumed with QURT_EDESTROY
  if the pipe gets destroyed .
  Pipe data items are defined as 64-bit values. Pipe reads are limited to transferring a single
  64-bit data item per operation. 

  @note1hang Transfer data items larger than 64 bits by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_data_t

  @param[in] pipe     Pointer to the pipe object to read from.
  @param[in] result   Pointer to the integer containing the 64-bit data item from pipe.

  @return     	
  #QURT_EOK -- Receive completed. \n
  #QURT_ECANCEL -- Receive canceled. \n
  #QURT_EDESTROY -- Receive destroyed. \n
  #QURT_ENOTALLOWED -- Pipe is not initialized

 
  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_pipe_receive_cancellable(qurt_pipe_t *pipe, qurt_pipe_data_t *result);

/**@ingroup func_qurt_pipe_send_cancellable  
  @xreflabel{hdr:qurt_pipe_send_cancellable}
  Writes a data item to the specified pipe (with suspend), cancellable. \n
  If a thread writes to a full pipe, it is suspended on the pipe. When another thread reads
  from the pipe, the suspended thread is awakened and can then write data to the pipe.
  The operation is canceled if the user process of the calling thread is killed, or if the 
  calling thread must finish its current QDI invocation and return to user space.
  Root pd thread can use this api to wait on pipe for receiving and gets resumed with QURT_EDESTROY
  if the pipe gets destroyed .

  Pipe data items are defined as 64-bit values. Pipe writes are limited to transferring a single
  64-bit data item per operation.

  @note1hang Transfer data items larger than 64 bits by reading and writing
             pointers to the data, or by transferring the data in consecutive 64-bit chunks.

  @datatypes
  #qurt_pipe_t \n
  #qurt_pipe_data_t

  @param[in] pipe      Pointer to the pipe object to read from.
  @param[in] data      Data item to write.

  @return     	
  #QURT_EOK -- Send completed. \n
  #QURT_ECANCEL -- Send canceled. \n
  #QURT_EDESTROY -- Send destroyed. \n
  #QURT_ENOTALLOWED -- Pipe is not initialized

 
  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_pipe_send_cancellable(qurt_pipe_t *pipe, qurt_pipe_data_t data);

/**@ingroup func_qurt_pipe_is_empty
  Returns a value indicating whether the specified pipe contains any data.

  @datatypes
  #qurt_pipe_t

  @param[in] pipe     Pointer to the pipe object to read from.

  @return
  1 -- Pipe contains no data. \n
  0 -- Pipe contains data.

  @dependencies
  None.
*/
int qurt_pipe_is_empty(qurt_pipe_t *pipe);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  /* QURT_PIPE_H */
