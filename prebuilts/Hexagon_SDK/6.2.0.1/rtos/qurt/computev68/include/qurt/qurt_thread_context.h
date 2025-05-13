#ifndef QURT_THREAD_CONTEXT_H
#define QURT_THREAD_CONTEXT_H
/**
  @file qurt_thread_context.h 
  @brief Kernel thread context structure
			
EXTERNAL FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2018-2022  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/



#include <qurt_qdi_constants.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond internal_only */

#define THREAD_ITERATOR_END ((qurt_thread_t)(-1))  /**< Thread iterator is complete. */   


/**@ingroup func_qurt_thread_iterator_create
Gives the ability to the caller to enumerate threads in the system.

@return 
Handle of the newly created iterator must be passed for
subsequent operations on the iterator.           

@dependencies
None.
*/
static inline int qurt_thread_iterator_create(void)
{
   return qurt_qdi_handle_invoke(QDI_HANDLE_GENERIC, QDI_OS_THREAD_ITERATOR_CREATE);
}

/**@ingroup func_qurt_thread_iterator_next
Iterates over the list of threads in the system.

@datatypes
#qurt_thread_t

@param[in] iter Iterator handle returned by qurt_thread_iterator_create().

@return 
#THREAD_ITERATOR_END -- iterator has reached the end of the thread list. \n
Other values indicate a valid thread_id.

@dependencies
None.
*/
static inline qurt_thread_t qurt_thread_iterator_next(int iter)
{
   return (qurt_thread_t)qurt_qdi_handle_invoke(iter, QDI_OS_THREAD_ITERATOR_NEXT);
}

/**@ingroup func_qurt_thread_iterator_destroy
Cleans up thread iterator resources.

@param[in] iter Iterator handle returned by qurt_thread_iterator_create().

@return 
#QURT_EOK -- Successful completion of operation \n
#QURT_EFATAL -- Invalid handle passed 
		  
@dependencies
None.
*/
static inline int qurt_thread_iterator_destroy(int iter)
{
   return qurt_qdi_close(iter);
}

/**@ingroup func_qurt_thread_context_get_tname
Gets the name of the thread from the specified thread ID.

@param[in]      thread_id   Thread for which name is returned.
@param[in,out]  name        Pointer to the local buffer where name is copied back.
@param[in]      max_len     Size of the local buffer.

@return 
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_tname(unsigned int thread_id, char *name, unsigned char max_len);

/**@ingroup func_qurt_thread_context_get_prio
Gets the priority for the specified thread.

@param[in]     thread_id   Thread for which priority is returned.
@param[in,out] prio        Pointer to the local variable where priority is written.

@return  
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_prio(unsigned int thread_id, unsigned char *prio);

/**@ingroup func_qurt_thread_context_get_pcycles
Gets pcycles for the specified thread.

@param[in]     thread_id Thread for which processor cycles are returned.
@param[in,out] pcycles   Pointer to the local variable where processor cycles are written.

@return  
#QURT_EOK -- Success \n
Failure otherwise.
		  
@dependencies
None.
*/
int qurt_thread_context_get_pcycles(unsigned int thread_id, unsigned long long int *pcycles);

/**@ingroup func_qurt_thread_context_get_stack_base
Gets the stack base address for the specified thread.

@param[in]     thread_id Thread for which stack base address is returned.
@param[in,out] sbase     Pointer to the local variable where stack base address is written.

@return  
QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_stack_base(unsigned int thread_id, unsigned int *sbase);

/**@ingroup func_qurt_thread_context_get_stack_size
Gets the stack size for the specified thread.

@param[in]      thread_id   Thread for which stack size is returned.
@param[in,out]  ssize       Pointer to the local variable where stack size is written.

@return  
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_stack_size(unsigned int thread_id, unsigned int *ssize);

/**@ingroup func_qurt_thread_context_get_pid
Gets the process ID for the specified thread.

@param[in]     thread_id  Thread for which process ID is returned.
@param[in,out] pid        Pointer to the local variable where process id is written.

@return  
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_pid(unsigned int thread_id, unsigned int *pid);

/**@ingroup func_qurt_thread_context_get_pname
Gets the process name for the specified thread.

@param[in]       thread_id  Represents the thread for which process name is returned.
@param[in, out]  name       Pointer to the local buffer where process name is copied back.
@param[in]       len        Length allocated to the local buffer.

@return  
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_thread_context_get_pname(unsigned int thread_id, char *name, unsigned int len);

/** @addtogroup thread_types
@{ */
/** Structure that defines how TCB is interpreted to crash dump tools.*/
/* Keys are defined in consts.h */
struct qurt_debug_thread_info {
/** @cond */
    char name[QURT_MAX_NAME_LEN];     /**< Name of the thread. */
    struct {
        unsigned key;                 
        unsigned val;
    } os_info[40];  
    unsigned gen_regs[32];            /**< General mode registers. */
    unsigned user_cregs[32];          /**< User mode registers. */
    unsigned guest_cregs[32];         /**< Guest mode registers. */
    unsigned monitor_cregs[64];       /**< Monitor mode registers. */
/** @endcond */
}; /* should add up to 1K */
/** @} */ /* end_addtogroup thread_types */


/**@ingroup func_qurt_system_tcb_dump_get
Cleans up thread iterator resources.

@datatypes
#qurt_thread_t

@param[in]       thread_id  Thread on which the operation must be performed.
@param[in, out]  ptr        Pointer to the local buffer where contents are written.
@param[in]       size       Size of the debug thread information structure obtained by calling
                     qurt_system_tcb_dump_get_size().
	   
@return 
#QURT_EOK -- Success \n
Failure otherwise
		  
@dependencies
None.
*/
int qurt_system_tcb_dump_get(qurt_thread_t thread_id, void *ptr, size_t size);
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_THREAD_CONTEXT_H */
