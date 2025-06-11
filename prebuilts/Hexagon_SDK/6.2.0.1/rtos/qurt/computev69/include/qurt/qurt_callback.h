#ifndef QURT_CALLBACK_H
#define QURT_CALLBACK_H

/**
  @file qurt_callback.h
    Definitions, macros, and prototypes for QuRT callback framework.
  
  QDI framework allows the development of root process drivers and services that 
  a user process client can interact with in a secure manner. QDI framework does 
  this by elevating the priviledge of user process thread, temporarily allowing 
  the thread execute in root context and letting it fall back to user context once 
  the QDI invocation is finished. 

  The QuRT callback framework provides a safe mechanism for root process drivers 
  to execute callback functions in a user process. The framework hosts 
  dedicated worker threads in corresponding processes that handle the execution
  of the callback function. This ensures that the callbacks occur in context of
  the appropriate process thread, in result maintaining privilege boundaries. 

  Prerequisites for use of this framework are:
  1. Driver is a QDI driver and client communicates with drivers using QDI 
     invocations.
  2. Appropriate callback configuration is specified in cust_config.xml for 
     the user process that intends to use this framework.

  qurt_cb_data_t is the public data structure that allows client to store all
  the required information about the callback, including the callback function
  and the arguments to pass to this function when it executes.
  The client uses QDI interface to register this structure with root driver.
  
  Callback framework provides following APIs that a root driver can use to invoke callback.
  These functions are described in qurt_qdi_driver.h header file.

  qurt_qdi_cb_invoke_async() triggers an asynchronous callback wherein the
  invoking thread does not wait for the callback to finish executing.

  qurt_qdi_cb_invoke_sync()  triggers a synchronous callback. Upon invocation
  the invoking thread gets suspended till the callback function finishes execution.
  
  qurt_qdi_cb_invoke_sync_with_data() invokes a synchronous callback similar to
  qurt_qdi_cb_invoke_sync(). It allows user to pass large data along with 
  the callback invocation to be utlized during the callback execution.
     
 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/
#include "qurt_qdi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int qurt_cb_result_t;

/* Callback framework error codes.
  Callback framework returns a nonzero value if callback invocation is unsuccessful.
  Following macros highlight cause of failure in more detail.
*/
#define QURT_CB_ERROR               -1                  /* Callback registration failed.\n*/
#define QURT_CB_OK                   0                  /* Success.\n*/
#define QURT_CB_MALLOC_FAILED       -2                  /* QuRTOS malloc failure.\n*/
#define QURT_CB_WAIT_CANCEL         -3                  /* Process exit cancelled wait operation.\n*/
#define QURT_CB_CONFIG_NOT_FOUND    -4                  /* Callback configuration for process was not found.\n*/
#define QURT_CB_QUEUE_FULL          -5                  /* Callback queue is serving at maximum capacity.*/
/** @addtogroup cb_types
@{ */
/** Callback registration data structure.
  This data structure is used by a client attempting to register a callback with a QDI driver.
  It holds the address of callback function and the argument supplied to the callback 
  function when it executes.
*/
typedef struct {
  /** @cond */
  void* cb_func;             /*< Pointer to the callback function. */
  unsigned cb_arg;           /*< Not interpreted by the framework.*/
  /** @endcond */
} qurt_cb_data_t;

/** @cond */
/* Defines used as default if cust_config does not specify them. */
#define CALLBACK_WORKER_STACK_SIZE 0x2000
/** @endcond */
/** @} */ /* end_addtogroup cb_typess */
/**@ingroup func_qurt_cb_data_init 
  Initializes the callback data structure.
  Entity registering a callback with the root process driver must call this function
  to initialize callback registration data structure to the default value.

  @datatypes 
  #qurt_cb_data_t

  @param[in]  cb_data         Pointer to the callback data structure.

  @return
  None.

  @dependencies
  None.
 */
static inline void qurt_cb_data_init (qurt_cb_data_t* cb_data){
    cb_data->cb_func = NULL;
    cb_data->cb_arg = 0;
}

/**@ingroup func_qurt_cb_data_set_cbfunc
  Sets up the callback function in the callback registration data structure.
  
  @datatypes 
  #qurt_cb_data_t

  @param[in] cb_data         Pointer to the callback data structure.
  @param[in] cb_func         Pointer to the callback function.

  @return
  None.

  @dependencies
  None.
 */
static inline void qurt_cb_data_set_cbfunc (qurt_cb_data_t* cb_data, void* cb_func){
  cb_data->cb_func = cb_func;
}

/**@ingroup func_qurt_cb_data_set_cbarg
  Sets up the callback argument.
  This function sets up the argument passed to the callback function when it executes.
  
  @datatypes 
  #qurt_cb_data_t

  @param[in] cb_data         Pointer to the callback data structure.
  @param[in] cb_arg          Argument for the callback function.

  @return
  None.

  @dependencies
  None.
 */
static inline void qurt_cb_data_set_cbarg (qurt_cb_data_t* cb_data, unsigned cb_arg){
  cb_data->cb_arg = cb_arg;
}

/** @cond */
/**@ingroup driver_support_functions
  Invokes an asynchronous callback for a specified process. 
  A driver that resides in the root process calls this API to launch a callback in
  a process described by the client_handle.
  After the callback is invoked, the framework queues the callback as per its 
  priority and subsequently executes it.
  The caller of this function is not suspended during the callback execution period.
  The API returns immediately with a success/failure error code.

  @note1hang  This function is only accessible to drivers in the root process. 
              User process invocations shall fail with a negative error code return value.

  @param  client_handle   Obtained from the current invocation function (Section 4.3.1).
  @param  cb_data         Pointer to the callback data structure (refer to qurt_callback.h).
  @param  prio            Priority at which the callback should execute.
                          This paraemter is optional. If -1 is passed, the callback frameowrk 
                          executes the callback at the priority of the API caller.
  @return
  QURT_EOK -- Callback was successfully communicated to the framework.
  Negative error code -- Callback cannot be communicated to the framework.
 */
qurt_cb_result_t qurt_qdi_cb_invoke_async(int client_handle,
                                          qurt_cb_data_t* cb_data,
                                          int prio);


/**@ingroup driver_support_functions
  Invokes a synchronous callback for a specified process. 
  A driver that resides in a root process calls this API to launch a sync callback in
  a process described by the client_handle.
  AFter the callback is invoked, the framework queues the callback as per its 
  priority and subsequently executes it.
  The caller of this function is suspended during the callback execution period.
  If the process in which to execute the callback exits or terminates, the caller is
  woken up with error code #QURT_CB_WAIT_CANCEL (refer to qurt_callback.h).

  @note1hang  This function is only accessible to drivers in the root process. 
              User process invocations shall fail with a negative error code return value.

  @param  client_handle   Obtained from the current invocation function (Section 4.3.1).
  @param  cb_data         Pointer to the callback data structure (refer to qurt_callback.h).
  @param  prio            Priority at which the callback should execute.
                          This paraemter is optional. If -1 is passed, callback frameowrk 
                          executes the callback at the priority of the API caller.
  @return
  QURT_EOK -- Callback was successfully communicated to the framework.
  Negative error code -- Callback cannot be communicated to the framework.
 */
qurt_cb_result_t qurt_qdi_cb_invoke_sync(int client_handle,
                                         qurt_cb_data_t* cb_data,
                                         int prio);

/**@ingroup driver_support_functions
  Invokes a synchronous callback for a specified process, passing driver data to the user PD.
  This function is similar to qurt_qdi_cb_invoke_sync() and allows the driver to pass arbitrary data to
  the user process as part of the callback invocation.

  @param  client_handle   Obtained from the current invocation function (Section 4.3.1).
  @param  cb_data         Pointer to the callback data structure (refer to qurt_callback.h).
  @param  prio            Priority at which the callback should execute.
                          This paraemter is optional. If -1 is passed, the callback frameowrk
                          executes the callback at the priority of the API caller.
  @param  data            Driver arbitrary data to pass to the user process. Memory pointed to by data
                          must be accessible to the user PD. The root driver can allocate such memory by
                          using qurt_mem_mmap().
  @param  data_len        Driver arbitrary data length.
  
  @return
  QURT_EOK -- Callback was successfully communicated to the framework.
  Negative error code -- Callback cannot be communicated to the framework.
 */
qurt_cb_result_t qurt_qdi_cb_invoke_sync_with_data( int client_handle,
                                                    qurt_cb_data_t* cb_data,
                                                    int prio,
                                                    void *data,
                                                    unsigned data_len
                                                    );
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif

