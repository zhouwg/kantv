#ifndef QURT_SPACE_H
#define QURT_SPACE_H
/**
  @file qurt_space.h
  @brief Prototypes of QuRT process control APIs

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2021, 2023 by Qualcomm Technologies, Inc. All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#include <qurt_types.h>
#include <qurt_signal.h>
#include <qurt_process.h>

#ifdef __cplusplus
extern "C" {
#endif

/** This flag is a request to the OS to suspend the processes just before calling main()
But it is going to be obsoleted and replaced by QURT_PROCESS_SUSPEND_ON_STARTUP */
#define SPAWNN_FLAG_SUSPEND_ON_STARTUP QURT_PROCESS_SUSPEND_ON_STARTUP

/**
 * Creates and starts a process from ELF of a specified name. The slash symbols
 * "/" or "\" are ignored. Do not include the directory name in the input. This function
 * accepts the the SPAWN flags. Multiple SPAWN flags can be specified by OR'ing the flags.
 *
 * @param name      ELF name of the executable. Name shall not contain directories,
 *                  use "dsp2.elf", instead of "/prj/qct/.../dsp2.elf"
 *
 * @param return
   Process ID -- Success \n
   Negative error code -- failure\n
   #QURT_EPRIVILEGE --                    Caller does not have enough privilege for this operation\n
   #QURT_EMEM       --                    Not enough memory to perform the operation \n
   #QURT_EFAILED     --                   Operation failed \n
   #QURT_ENOTALLOWED --                   Operation not allowed \n
   #QURT_ENOREGISTERED --                 Not registered \n
   #QURT_ENORESOURCE  --                  Resource exhaustion \n
   #QURT_EINVALID --                      Invalid argument value
*/

int qurt_spawn_flags(const char * name, int flags);

/**
   Creates and starts a process from an ELF of the specified name. The slash symbols
   "/" or "\" are ignored. Do not include the directory name in the input.

   @param name      ELF name of the executable. Name shall not contain directories,
                    use "dsp2.elf", instead of "/prj/qct/.../dsp2.elf".

   @return
   Process ID -- Success. \m
   Negative error code -- Failure.

*/
static inline int qurt_spawn(const char *name)
{
    return qurt_spawn_flags(name,0);
}

/**
 * Returns the process ID of the current process.
 *
 * @return
 * Process ID
 *
*/
#define qurt_getpid qurt_process_get_id

/**
 * The qurt_wait() function  waits for status change in a child process. It could be used by parent
 * process to block on any child process terminates.
 *
 * This API returns error if there are no user processes or all user processes got detached.
 *
 * @param status    Pointer to status variable. The variable provides the status value of child process.
 *                  The value comes from exit() system call made by child process.
 *
 * @return
   Process ID of the child process that changes status -- Success \n
 * Negative error code -- Failure
 *
*/

int qurt_wait(int *status);


/** @cond */
/* APIs that allow registering callbacks on spawn of user pd */
typedef void (*QURT_SPAWN_PFN)(int client_handle, void *data_ptr);  //no return, since we won't be error checking it in spawn 
typedef int (*QURT_CB_PFN)(int client_handle, void *user_data, void *info);
typedef union {
    QURT_SPAWN_PFN spawn_pfn;
    QURT_CB_PFN cb_pfn;
} qurt_process_callback_pfn_t;
/** @endcond */

/** @cond internal_only */

/**@ingroup func_qurt_event_register
Sets the specified bits by mask in the signal passed by the caller. The signal gets set
when the client handle indicated by value goes away (at process exit). Multiple clients can register for the signal
to be set.

@datatypes

@param[in]  type     QURT_PROCESS_EXIT is the only event that can be registered for.
@param[in]  value    Indicates the client handle of the process for which the event is registered.
@param[in]  signal   Pointer to the signal object to set when the event occurs.
@param[in]  mask     Mask bits to set in the signal.
@param[out] data     Pointer to the variable that would receive the exit code of the exiting process.
@param[in]  datasize Size of the data variable.

@return
#QURT_EOK -- Success \n
#QURT_EMEM -- Not enough memory to allocate resources \n
#QURT_EVAL -- Invalid values passed to the API

@dependencies
None.
*/
int qurt_event_register(int type, int value, qurt_signal_t *psig, unsigned int mask, void *data, unsigned int data_size);

/**@ingroup func_qurt_callback_register_onspawn
Allows registering for a callback on spawn of any user process.

@datatypes
#QURT_SPAWN_PFN

@param[in] pFn         Callback function to call when any user process is spawned.
@param[in] user_data   Pointer to the argument that the callback must be called with.


@return   If positive value is obtained, handle to be used while deregistering the callback.
          Mutliple clients can register for callback on spawn and some clients might choose to deregister.

          If failed, QURT_EFATAL will be returned.

@dependencies
None.
*/
int qurt_callback_register_onspawn(QURT_SPAWN_PFN pFn, void *user_data);

/**@ingroup func_qurt_callback_deregister_onspawn
Allows de-registering callback on spawn.

@param[in] callback_handle   Handle returned by qurt_callback_register_onspawn.

@return
#QURT_EOK --de-registering was successful

@dependencies
None.
*/
int qurt_callback_deregister_onspawn(int callback_handle);

/**@ingroup func_qurt_process_callback_register
Allows registering for a callback during or after image loading.
Generic callback types:
    Functions similarly to qurt_callback_register_onspawn(). Callback is called after process is
    loaded, before process thread starts. Callback has no return value and has no info provided
    from OS.
        pFn - QURT_SPAWN_PFN
        type - QURT_PROCESS_CB_GENERIC
        arg1 - not used 
        arg2 - not used
        arg3 - not used
Note callback types:
    Callback is called during process loading: before segment loading(QURT_PROCESS_NOTE_CB_PRE_MAP),
    or after segment loading (QURT_PROCESS_NOTE_CB_POST_MAP). OS provides info to the callback. info
    argument in callback is populated with pointer to the mapped note corresponding to the callback.
    Callback has return value, loader fails if callback returns a value that is not QURT_EOK.
        pFn - QURT_CB_PFN
        type - QURT_PROCESS_NOTE_CB_PRE_MAP or QURT_PROCESS_NOTE_CB_POST_MAP
        arg1 - note type (ex: NOTE_TYPE_POOL_INFO, NOTE_TYPE_SEGMENT_INFO, NOTE_TYPE_ARB_INFO)
        arg2 - note name
        arg3 - not used

@datatypes

@param[in] pFn          Callback function to call
@param[in] type         Callback type
@param[in] user_data    Pointer to the argument that the callback must be called with.
@param[in] arg1         Arguments interpreted by OS based on callback type
@param[in] arg2         Arguments interpreted by OS based on callback type
@param[in] arg3         Arguments interpreted by OS based on callback type (currently not used)


@return   If positive value is obtained, handle to be used while deregistering the callback.
          Mutliple clients can register for callback on spawn and some clients might choose to deregister.

          If failed, QURT_EFATAL will be returned.

@dependencies
None.
*/
int qurt_process_callback_register(qurt_process_callback_pfn_t pFn, 
                                   qurt_process_cb_type_t type, 
                                   void *user_data, 
                                   qurt_process_callback_arg_t arg1, 
                                   qurt_process_callback_arg_t arg2, 
                                   qurt_process_callback_arg_t arg3);



/**@ingroup func_qurt_process_callback_deregister
Allows de-registering callback for imate loading.
@param[in] callback_handle   Handle returned by qurt_process_callback_register.

@return
#QURT_EOK --de-registering was successful

@dependencies
None.
*/
int qurt_process_callback_deregister(int callback_handle);
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_SPACE_H */
