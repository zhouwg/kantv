#ifndef QURT_PROCESS_H
#define QURT_PROCESS_H
/**
  @file qurt_process.h
  @brief Prototypes of QuRT process control APIs.

 EXTERNALIZED FUNCTIONS
 None

 INITIALIZATION AND SEQUENCING REQUIREMENTS
 None

 Copyright (c) 2009-2013, 2021-2023 Qualcomm Technologies, Inc.
 All rights reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/
#include "qurt_callback.h"
#include "qurt_consts.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup process_types
@{ */
#define QURT_PROCESS_ATTR_NAME_MAXLEN       QURT_MAX_NAME_LEN   /**< Maximum length of the process name. */
#define QURT_PROCESS_ATTR_BIN_PATH_MAXLEN   128                 /**< Maximum length of the path of binary/ELF for this process. */
#define QURT_PROCESS_ATTR_CAP_MAXLEN        128                 /**< Maximum length for a resource name. */

/** QuRT process capability wildcard strings */
#define QURT_PROCESS_ATTR_CAP_ALLOW_ALL     "ALLOW_ALL"         /**< Capability wild-card for full access */
#define QURT_PROCESS_ATTR_CAP_ALLOW_NONE    "ALLOW_NONE"        /**< Capability wild-card for no access */

/** QuRT process capability states */  
#define QURT_PROCESS_ATTR_CAP_ENABLED       0x1                 /**< Capability enabled*/
#define QURT_PROCESS_ATTR_CAP_DISABLED      0x0                 /**< Capability disabled*/  

/* QuRT process thread attributes. */
#define QURT_PROCESS_DEFAULT_CEILING_PRIO 0        /**< Default ceiling priority of the threads in the new process. */
#define QURT_PROCESS_DEFAULT_MAX_THREADS  -1       /**< Default number of threads in the new process.
                                                        -1 indicates that the limit is set to the maximum supported by the system. */

/* QuRT process flags. */
#define QURT_PROCESS_SUSPEND_ON_STARTUP  (1U)      /**< Suspend the new processes just before calling main(). */
#define QURT_PROCESS_NON_SYSTEM_CRITICAL (1u << 1) /**< Starts the new process as non system-critical. */
#define QURT_PROCESS_ISLAND_RESIDENT     (1u << 2) /**< Process is island resident. */
#define QURT_PROCESS_RESTARTABLE         (1u << 3) /**< Indicates that the process is restartable */
#define QURT_PROCESS_UNTRUSTED           (1u << 7) /**< Starts the new process as unsigned process. */

/* QuRT process debugging session status.*/
#define QURT_DEBUG_NOT_START         0  /**< Debug is not started. */
#define QURT_DEBUG_START             1  /**< Debug has started. */

/** Process Suspend Options */
#define QURT_PROCESS_SUSPEND_DEFAULT   0

/** Process Resume Options   */
#define QURT_PROCESS_RESUME_DEFAULT    0


/* QuRT process types. */
typedef enum {
    QURT_PROCESS_TYPE_RESERVED,            /**< Process type is reserved. \n */
    QURT_PROCESS_TYPE_KERNEL,              /**< Kernel process. \n*/
    QURT_PROCESS_TYPE_SRM,                 /**< SRM process.    \n*/
    QURT_PROCESS_TYPE_SECURE,              /**< Secure process. \n*/
    QURT_PROCESS_TYPE_ROOT,                /**< Root process.   \n*/
    QURT_PROCESS_TYPE_USER,                /**< User process.   */
}qurt_process_type_t;

/** QuRT process callback types. */
typedef enum {
   QURT_PROCESS_DUMP_CB_ROOT,             /**< Register the callback that executes in the
                                               root process context. \n */
   QURT_PROCESS_DUMP_CB_ERROR,            /**< Register the user process callback that is 
                                               called after threads in the process are frozen. \n */
   QURT_PROCESS_DUMP_CB_PRESTM,           /**< Register the user process callback that is
                                               called before threads in the process are frozen. \n*/
   QURT_PROCESS_DUMP_CB_MAX               /**< Reserved for error checking. */
}qurt_process_dump_cb_type_t;

/** QuRT process dump attributes. */
typedef struct _qurt_pd_dump_attr{
  /** @cond */
  unsigned int enabled;                    /**< Process dump is enabled. */
  const char *path;                        /**< Process dump path. */
  unsigned int path_len;                   /**< Length of process dump path. */
  /** @endcond */
}qurt_pd_dump_attr_t;                    

/** QuRT process capability resource type */
enum qurt_process_cap_type_t {
    QURT_PROCESS_CAP_TYPE_NUM_ENTRIES=0,       /**< Number of entries in the capability structure*/
    QURT_PROCESS_CAP_TYPE_DRIVER=1,            /**< Driver resource */
    QURT_PROCESS_CAP_TYPE_MAX                  /**< Maximum identifier */        
};

/** QuRT process capability structure */
typedef struct _qurt_capability {
    enum qurt_process_cap_type_t type;             /**< Resource type */
    char name[QURT_PROCESS_ATTR_CAP_MAXLEN];       /**< Resource name*/ 
    unsigned long long cap;                        /**< Capabilities allowed for this resource */
}qurt_capability_t;

/** QuRT process attributes. */
typedef struct _qurt_process_attr {
    /** @cond */
    char name[QURT_PROCESS_ATTR_NAME_MAXLEN];           /**< Name of the new process. */
    char path[QURT_PROCESS_ATTR_BIN_PATH_MAXLEN];       /**< Path of the binary for the new process. */
    char dtb_path[QURT_PROCESS_ATTR_BIN_PATH_MAXLEN];   /**< Path of the DTB ELF for the new process. */
    int flags;                                          /**< Flags as indicated by QuRT process flags. */
    unsigned int sw_id;                                 /**< Software ID of the process be load. */
    unsigned sid;                                       /**< Stream ID of the process being spawned. */
    unsigned max_threads;                               /**< Maximum number of threads that the new process can create. */
    unsigned short ceiling_prio;                        /**< Maximum priority at which threads can be 
                                                             created by new process. */
    qurt_process_type_t type;                           /**< Process type as indicated by 
                                                             #qurt_process_type_t. */
    qurt_pd_dump_attr_t dump_attr;                      /**< Process dump attributes for the new process 
                                                             as indicated by #qurt_pd_dump_attr_t. */ 
    qurt_capability_t *capabilities;                    /**< Pointer to array of structure of type
                                                             qurt_capability_t */
    /** @endcond */
} qurt_process_attr_t; 

/** @} */ /* end_addtogroup process_types */

/*=============================================================================
FUNCTIONS
=============================================================================*/
 /** @cond rest_reg_dist */
/**@ingroup func_qurt_process_create
  Creates a process with the specified attributes, and starts the process.

  The process executes the code in the specified executable ELF file.

  @datatypes
  #qurt_process_attr_t

  @param[out] attr Accepts an initialized process attribute structure, which specifies
                   the attributes of the created process.

  @return
  Postive return value Indicates Process ID.
  Negative return value Indicates any of follwoing error,
  #-QURT_EPRIVILEGE      --   Caller does not have privilege for this operation \n
  #-QURT_EMEM            --   Not enough memory to perform the operation \n
  #-QURT_EFAILED         --   Operation failed \n
  #-QURT_ENOTALLOWED     --   Operation not allowed \n
  #-QURT_ENOREGISTERED   --   Not registered    \n
  #-QURT_ENORESOURCE     --   Resource exhaustion   \n
  #-QURT_EINVALID        --   Invalid argument value    
  #QURT_EFATAL           --   attr is NULL

  @dependencies
  None.
*/
int qurt_process_create (qurt_process_attr_t *attr);

/**@ingroup func_qurt_process_get_id
  Returns the process identifier for the current thread. 

  @return
  None.

  @dependencies
  Process identifier for the current thread.
*/
int qurt_process_get_id (void);
/** @endcond */

/** @cond internal_only*/
/**@ingroup func_qurt_process_get_uid
  Returns the user identifier for the current thread. 

  @return
  None.

  @dependencies
  User identifier for the current thread.
*/
int qurt_process_get_uid (void);
/** @endcond */
/** @cond rest_reg_dist */
/**@ingroup func_qurt_process_attr_init
  Initializes the structure that sets the process attributes when a thread is created.

  After an attribute structure is initialized, the individual attributes in the structure can 
  be explicitly set using the process attribute operations.

  Table @xref{tbl:processAttrDefaults} lists the default attribute values set by the initialize 
  operation.

  @inputov{table_process_attribute_defaults}

  @datatypes
  #qurt_process_attr_t

  @param[out] attr Pointer to the structure to initialize.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_process_attr_init (qurt_process_attr_t *attr)
{
    attr->name[0] = '\0';
    attr->path[0] = '\0';
    attr->dtb_path[0] = '\0';
    attr->flags = 0;
    attr->sw_id = 0;
    attr->sid = 0;
    attr->max_threads = (unsigned)QURT_PROCESS_DEFAULT_MAX_THREADS;
    attr->ceiling_prio = QURT_PROCESS_DEFAULT_CEILING_PRIO;
    attr->type = QURT_PROCESS_TYPE_RESERVED;
    attr->dump_attr.enabled = 0;
    attr->dump_attr.path = NULL;
    attr->dump_attr.path_len = 0;
    attr->capabilities = NULL;
}

/**@ingroup func_qurt_process_attr_set_executable
  Sets the process name in the specified process attribute structure.

  Process names identify process objects that are already 
  loaded in memory as part of the QuRT system.

  @note1hang Process objects are incorporated into the QuRT system at build time.

  @note1hang Maximum length of name string is limited to QURT_PROCESS_ATTR_NAME_MAXLEN - 1.

  @datatypes
  #qurt_process_attr_t

  @param[in] attr Pointer to the process attribute structure.
  @param[in] name Pointer to the process name.
 
  @return
  None.

  @dependencies
  None.
*/
void qurt_process_attr_set_executable (qurt_process_attr_t *attr, const char *name);

/**@ingroup func_qurt_process_attr_set_binary_path
  Sets the binary path for the process loading in the specified process attribute structure.

  Path specifies the binary to load for this process.
  
  @note1hang Max length of path string is limited to QURT_PROCESS_ATTR_BIN_PATH_MAXLEN-1.

  @datatypes
  #qurt_process_attr_t

  @param[in] attr Pointer to the process attribute structure.
  @param[in] path Pointer to the binary path.
 
  @return
  None.

  @dependencies
  None.
*/
void qurt_process_attr_set_binary_path(qurt_process_attr_t *attr, char *path);

/**@ingroup func_qurt_process_attr_set_dtb_path
  Sets the DTB binary path for the process loading in the specified process attribute structure.

  Path specifies the DTB binary to load for this process.
  
  @note1hang Max length of path string is limited to QURT_PROCESS_ATTR_BIN_PATH_MAXLEN-1.

  @datatypes
  #qurt_process_attr_t

  @param[in] attr Pointer to the process attribute structure.
  @param[in] path Pointer to the binary path.
 
  @return
  None.

  @dependencies
  None.
*/
void qurt_process_attr_set_dtb_path(qurt_process_attr_t *attr, char *path);

/**@ingroup func_qurt_process_attr_set_flags
Sets the process properties in the specified process attribute structure.
Process properties are represented as defined symbols that map into bits 
0 through 31 of the 32-bit flag value. Multiple properties are specified by OR'ing 
together the individual property symbols.

@datatypes
#qurt_process_attr_t

@param[in] attr  Pointer to the process attribute structure.
@param[in] flags QURT_PROCESS_NON_SYSTEM_CRITICAL Process is considered as non system-critical.
                                                  This attribute will be used by error services,
                                                  to decide whether to kill user pd or whole subsystem.
                 QURT_PROCESS_ISLAND_RESIDENT     Process will be marked as island resident.
                 QURT_PROCESS_RESTARTABLE         Process will be marked as restartable.
                 QURT_PROCESS_UNTRUSTED           Process will be marked as unsigned process.
@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_flags (qurt_process_attr_t *attr, int flags)
{
    attr->flags = flags;
}
/** @endcond */
/** @cond internal_only*/
/**@ingroup func_qurt_process_attr_set_sid
Sets the process streamID in the specified process attribute structure.

@datatypes
#qurt_process_attr_t

@param[in] attr  Pointer to the process attribute structure.
@param[in] sid   streamID to set for this process.

@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_sid (qurt_process_attr_t *attr, unsigned sid)
{
    attr->sid = sid;
}
/** @endcond */
/** @cond rest_reg_dist */
/**@ingroup func_qurt_process_attr_set_max_threads
Sets the maximum number of threads allowed in the specified process attribute structure.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] max_threads   Maximum number of threads allowed for this process.

@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_max_threads (qurt_process_attr_t *attr, unsigned max_threads)
{
    attr->max_threads = max_threads;
}

/**@ingroup func_qurt_process_attr_set_sw_id
Sets the software ID of the process to load in the specified process attribute structure.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] sw_id         Software ID of the process, used in authentication.

@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_sw_id(qurt_process_attr_t *attr, unsigned int sw_id)
{
    attr->sw_id = sw_id;
}

/**@ingroup func_qurt_process_attr_set_ceiling_prio
Sets the highest thread priority allowed in the specified process attribute structure.
Refer qurt_thread.h for priority ranges.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] prio          Priority.

@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_ceiling_prio (qurt_process_attr_t *attr, unsigned short prio)
{
    attr->ceiling_prio = prio;
}
/** @endcond */

/** @cond internal_only*/
/**@ingroup func_qurt_process_attr_set_dump_status
Sets the process domain dump-enabled field in the process domain dump attributes.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] enabled       1 -- Process domain dump is collected \n
                         0 -- Process domain dump is not collected

@return
None.

@dependencies
None.
*/
static inline void qurt_process_attr_set_dump_status(qurt_process_attr_t *attr, unsigned int enabled)
{
    attr->dump_attr.enabled = enabled;
}

/**@ingroup func_qurt_process_attr_set_dump_path
Sets the process domain dump path and type.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] path          Path where the process domain dumps must be saved.
@param[in] path_len      Length of the path string.

@return
None. 

@dependencies
None.
*/
static inline void qurt_process_attr_set_dump_path(qurt_process_attr_t *attr, const char *path, int path_len)
{
    attr->dump_attr.path = path;
    attr->dump_attr.path_len = (unsigned int)path_len;
}

/**@ingroup func_qurt_process_attr_set_capabilities
Sets list of capabilities available to this process.

@datatypes
#qurt_process_attr_t

@param[in] attr          Pointer to the process attribute structure.
@param[in] capabilities  Pointer to array of structures of type qurt_capability_t defining 
                         resources and capabilites

@return
None. 

@dependencies
None.
*/
static inline void qurt_process_attr_set_capabilities(qurt_process_attr_t *attr, qurt_capability_t *capabilities)
{
    attr->capabilities = capabilities;
}

/** @endcond */
/** @cond rest_reg_dist */
/**@ingroup func_qurt_process_cmdline_get
Gets the command line string associated with the current process.
The Hexagon simulator command line arguments are retrieved using 
this function as long as the call is made
in the process of the QuRT installation, and with the 
requirement that the program runs in a simulation environment.

If the function modifies the provided buffer, it zero-terminates
the string. It is possible that the function does not modify the
provided buffer, so the caller must set buf[0] to a NULL
byte before making the call. A truncated command line is returned when
the command line is longer than the provided buffer.

@param[in] buf      Pointer to a character buffer that must be filled in.
@param[in] buf_siz  Size (in bytes) of the buffer pointed to by the buf argument.

@return
None.

@dependencies
None.
*/
void qurt_process_cmdline_get(char *buf, unsigned buf_siz);

/**@ingroup func_qurt_process_get_thread_count
Gets the number of threads present in the process indicated by the PID. 
 
@param[in] pid PID of the process for which the information is required.

@return
Number of threads in the process indicated by PID, if positive value is obtained
Negative error code if failed include:
   QURT_EFATAL - Invalid PID
   -QURT_ENOTALLOWED - Current process doesnt have access to target process indicated by PID

@dependencies
None.
*/
int qurt_process_get_thread_count(unsigned int pid);

/**@ingroup func_qurt_process_get_thread_ids
Gets the thread IDs for a process indicated by PID. 

@param[in] pid      PID of the process for which the information is required.
@param[in] ptr         Pointer to a user passed buffer that must be filled in with thread IDs.
@param[in] thread_num  Number of thread IDs requested.

@return
#QURT_EOK - Success
#QURT_EFATAL - Failed, ptr is NULL

@dependencies
None.
 */
int qurt_process_get_thread_ids(unsigned int pid, unsigned int *ptr, unsigned thread_num);
/** @endcond */
/** @cond internal_only*/
/**@ingroup func_qurt_process_dump_get_mem_mappings_count
Gets the number of mappings present in the process indicated by the PID. 
 
@param[in] pid PID of the process for which the information is required.

@return
Number of mappings for the process indicated by the PID.

@dependencies
None.
*/
int qurt_process_dump_get_mem_mappings_count(unsigned int pid);

/**@ingroup func_qurt_process_dump_get_mappings
Gets the mappings for a specified PID.

@note1hang This API skips device type mappings or mappings created by setting the #QURT_PERM_NODUMP attribute.

@param[in] pid      PID of the process for which the information is required.
@param[in] ptr      Pointer to a buffer that must be filled in with mappings.
@param[in] count    Count of mappings requested.

@return
Number of mappings filled in the buffer passed by the user.

@dependencies
None.
*/
int qurt_process_dump_get_mappings(unsigned int pid, unsigned int *ptr, unsigned count);
/** @endcond */
/** @cond rest_reg_dist */
/**@ingroup func_qurt_process_attr_get
Gets the attributes of the process with which it was created. 
 
@datatypes
#qurt_process_attr_t

@param[in]     pid  PID of the process for which the information is required.
@param[in,out] attr Pointer to the user allocated attribute structure.

@return
#QURT_EOK     - Success
#QURT_INVALID - Invalid PID
#QURT_EFATAL  - attr is NULL

@dependencies
None.
*/
int qurt_process_attr_get(unsigned int pid, qurt_process_attr_t *attr);

/**@ingroup func_qurt_process_dump_register_cb
Registers the process domain dump callback. 
 
@datatypes
#qurt_cb_data_t \n
#qurt_process_dump_cb_type_t

@param[in] cb_data Pointer to the callback information.
@param[in] type Callback type; these callbacks are called in the context of the user process domain: \n
   #QURT_PROCESS_DUMP_CB_PRESTM -- Before threads of the exiting process are frozen. \n
   #QURT_PROCESS_DUMP_CB_ERROR  -- After threads are frozen and captured. \n
   #QURT_PROCESS_DUMP_CB_ROOT   -- After threads are frozen and captured, and CB_ERROR type of callbacks
                                   are called.
@param[in] priority Priority.

@return
#QURT_EOK -- Success \n
Other values -- Failure
    QURT_EFATAL if cb_data is NULL
    QURT_EINVALID If invalid cb_type
    QURT_EFAILED If invalid cb_data 
 
@dependencies
None.
*/
int qurt_process_dump_register_cb(qurt_cb_data_t *cb_data, qurt_process_dump_cb_type_t type, unsigned short priority);

/**@ingroup func_qurt_process_dump_deregister_cb
Deregisters the process domain dump callback.

@datatypes
#qurt_cb_data_t \n
#qurt_process_dump_cb_type_t

@param[in] cb_data Pointer to the callback information to deregister.
@param[in] type    Callback type.

@return
#QURT_EOK -- Success.\n
Other values -- Failure.
    QURT_EFATAL if cb_data is NULL
    QURT_EINVALID If invalid cb_type
    QURT_EFAILED If invalid cb_data 

@dependencies
None.
*/
int qurt_process_dump_deregister_cb(qurt_cb_data_t *cb_data,qurt_process_dump_cb_type_t type);

/** @endcond */
/** @cond internal_only*/
/**@ingroup func_qurt_process_set_rtld_debug
Sets rtld_debug for a process. 
 
@param[in] pid     PID of the process for which rtld_debug must be set.
@param[in] address rtld_debug address.

@return
#QURT_EOK      - Success
#QURT_EINVALID - Invalid PID
#QURT_EFATAL   - Invalid address
 
@dependencies
None.
*/
int qurt_process_set_rtld_debug(unsigned int pid,unsigned int address);

/**@ingroup func_qurt_process_get_rtld_debug
Gets rtld_debug for a process.

@param[in] pid         PID of the process for which rtld_debug must be set.
@param[in,out] address Pointer to the user passed address in which the rtld_debug address must be returned.

@return
#QURT_EOK      - Success
#QURT_EINVALID - Invalid PID
#QURT_EFATAL   - Invalid address

@dependencies
None.
*/
int qurt_process_get_rtld_debug(unsigned int pid,unsigned int *address);
/** @endcond */
/**@ingroup func_qurt_process_exit
Exits the current user process with an exit code.

@param[in] exitcode Exit code.
 
@return
#QURT_EFATAL -- No client found with the specified PID value \n
#QURT_EINVALID -- Invalid client \n
#QURT_ENOTALLOWED -- User does not have permission to perform this operation \n
#QURT_EOK -- Success

@dependencies
None.
*/
int qurt_process_exit(int exitcode);

/**@ingroup func_qurt_process_kill
Kills the process represented by the PID with the exit code.

@param[in] pid       PID of the process to kill.
@param[in] exitcode  Exit code.
 
@return
#QURT_EFATAL -- No client found with the specified PID value \n
#QURT_EINVALID -- Invalid client \n
#QURT_ENOTALLOWED -- User does not have permission to perform this operation \n
#QURT_EOK -- Success

@dependencies
None.
*/
int qurt_process_kill(int pid, int exitcode);
 
 
/**@ingroup func_qurt_debugger_register_process
Registers the process indicated by the PID with the debug monitor. 

@param[in] pid  PID of the process.
@param[in] adr  Address.
 
@return
#QURT_EOK -- Success 

@dependencies
None.
*/
int qurt_debugger_register_process(int pid, unsigned int adr);
 
 
/**@ingroup func_qurt_debugger_deregister_process
Deregister the process indicated by the PID with the debug monitor.

@param[in] pid  PID of the process.
 
@return
#QURT_EOK -- Success

@dependencies
None.
*/
int qurt_debugger_deregister_process(int pid);
 
/**@ingroup func_qurt_process_exec_callback
Executes callbacks in the user process as indicated by the client_handle argument.

@param[in] client_handle  Client handle obtained from the current invocation function (Section 3.4.1).
@param[in] callback_fn    Callback function to execute.
@param[in] stack_base     Stack address to use.
@param[in] stack_size     Stack size.
 
@return
#QURT_EOK -- Success

@dependencies
None.
*/
int qurt_process_exec_callback(int client_handle,
                                     unsigned callback_fn,
                                     unsigned stack_base,
                                     unsigned stack_size);
 
/**@ingroup func_qurt_process_get_pid
Gets the process ID of the process that the client_handle argument represents.

@note1hang This API is not supported for unsigned PD, For unsigned PD use qurt_process_get_id()

@param[in] client_handle    Client handle obtained from the current invocation function (Section 3.4.1).
@param[in] pid              Pointer to the address to store the PID.
 
@return
#QURT_EOK -- Success
#QURT_EFATAL -- pid pointer passed as NULL 

@dependencies
None.
*/
int qurt_process_get_pid(int client_handle, int * pid);

/**@ingroup func_qurt_process_get_dm_status
Gets the debugging session status on the process represented by the pid argument.

@param[in]     pid      Process ID  
@param[in,out] status   Address to store the status: \n
                        #QURT_DEBUG_NOT_START \n        
                        #QURT_DEBUG_START         
 
@return
#QURT_EOK - Success \n
#QURT_EINVALID - Error

@dependencies
None.
*/
int qurt_process_get_dm_status( unsigned int pid, unsigned int *status);


/**@ingroup func_qurt_process_suspend_threads 
  Suspends user threads in a user process with its process identifier.
  The target user process can be a signed user process or an unsigned user process.
  The caller is from a thread in GuestOS/root process.
  After the user threads in the target user process are suspended, they cannot be scheduled to run by the kernel 
  until they resume later.

  This function has one optional argument with one default option.
  #QURT_PROCESS_SUSPEND_DEFAULT suspends user threads in the target user process.

  This function call is a synchronous call, the function returns after the relevant threads are 
  completely suspended. 
  
  If some user threads in the target user process are set as non-suspendable, this function call does
  not suspend these threads.

  If the target user process is already suspended, this function call returns success as the 
  confirmation on the user process suspending.

  QuRT debugger monitor threads in the target user process are non-suspendable, this function call does
  not suspend the threads.

  If the target user process is a secure user process, or a CPZ process, this function call returns error 
  without suspending the target user process.                                          

  If a user thread in the target user process runs in the guest OS/root process via a QDI call, this function call 
  does not suspend the thread in the guest OS, but instead marks the thread as pending-suspend. The thread is suspended 
  when it exits the guest OS, before executing the first instruction in the user process.
  In this case, the function returns success while the user thread can be running in GuestOS, and is suspended 
  when exiting the guest OS. 
 
  @param[in] process_id  Process identifier.
  @param[in] option      Dfault option #QURT_PROCESS_SUSPEND_DEFAULT suspends user threads in the target user process.
 
  @return
  #QURT_EOK         -- Success  \n
  #QURT_EINVALID    -- Failure because of invalid process_id input \n
  #QURT_ENOTALLOWED -- Failure because the operation is not allowed, for example, on a secure process/CPZ process.
 
  @dependencies
  None.
 */
int qurt_process_suspend_threads (unsigned int process_id, unsigned int option);


/**@ingroup func_qurt_process_resume_threads 
  Resumes a user process with its process identifier.
  The target user process can be a signed user process or an unsigned user process.
  The caller is from a thread in the guest OS/root process.
  After the user threads in the target user process resume, the kernel scheduler
  can schedule the user threads to run based on their thread priorities.

  This function has an optional argument, #QURT_PROCESS_RESUME_DEFAULT, which 
  resumes user threads in the target user process.

  This is an asynchronous function, it returns after the kernel moves the user thread from 
  suspended state to runnable state. The threads are scheduled to run based on their thread priorities.
  
  This function call does not resume threads in the target user process that have been set as non-resumable.

  If the target user process have already resumed, this function call confirms that the user process resumes
  by returning success.

  If the target user process is a secure user process or a CPZ process, this function call returns an error without 
  resuming operation.                                          

  If user threads in the target user process run in the guest OS/root process via QDI call, this function 
  call clears the mark of suspend-pending on these threads, so that the threads are be suspended when it exits 
  the guest OS. 
 
  @param[in] process_id Process identifier.
  @param[in] option     Default option #QURT_PROCESS_RESUME_DEFAULT resumes user threads in the target user process.
 
  @return
  #QURT_EOK         -- Success  
  #QURT_EINVALID    -- Failure because of invalid process_id input.
  #QURT_ENOTALLOWED -- Failure because of the operation is not allowed, for example, on a secure process/CPZ process.
 
  @dependencies
  None.
 */
int qurt_process_resume_threads (unsigned int process_id, unsigned int option);

/**@ingroup func_qurt_process_vtcm_window_set
  Set a VTCM access window for a process.
  The caller thread needs to be in SRM process.
  
  This is an synchronous function, it ensures all running threads of the process have the requested 
  window in effect.The requested view for all non-running thread will take in effect when they get 
  scheduled.  

  @param[in] pid Process identifier.
  @param[in] enable  QURT_VTCM_WINDOW_ENABLE    enforces VTCM access window defined by high and low offset.
                     QURT_VTCM_WINDOW_DISABLE   high and low offset is ignored and VTCM access is fully 
                                                disabled for the process.
  @param[in] high_offset  Specifies the high window offset, in 4K increments, from the base address of the VTCM.
                          QURT_VTCM_WINDOW_HI_OFFSET_DEFAULT  restore high offset to reset value.
  @param[in] low_offset   Specifies the low window offset, in 4K increments, from the base address of the VTCM.
                          QURT_VTCM_WINDOW_LO_OFFSET_DEFAULT restore low offset to reset value.
           
  @note1hang
  when high_offset is set to QURT_VTCM_WINDOW_HI_OFFSET_DEFAULT  and low offset is set as 
  QURT_VTCM_WINDOW_LO_OFFSET_DEFAULT full VTCM range is accessible. Access to VTCM is controlled 
  via MMU mapping for the process. 
  
  @return
  #QURT_EOK            -- Success  
  #QURT_EVAL           -- Failure because of invalid inputs.
  #QURT_EPRIVILEGE     -- Failure because caller does not have enough privilege for this operation.
  #QURT_ENOTSUPPORTED  -- Failure because of the operation is not supported due to limitation in HW capabilities 
 
  @dependencies
  None.
 */
int qurt_process_vtcm_window_set(int pid, unsigned int enable, unsigned int high_offset, unsigned int low_offset);

/**@ingroup func_qurt_process_vtcm_window_get
  Get the VTCM window for a process.
  The caller thread needs to be in SRM process.
  

  @param[in] pid Process identifier.
  @param[out] enable  address to store enable status if set
  @param[out] high_offset address to return high window offset, in 4K increments, from the base address of the VTCM
  @param[out] low_offset  address to return low window offset, in 4K increments, from the base address of the VTCM.
  
  @note1hang
  User must first check the value of enable returned before checking high and low offset.
 
  @return
  #QURT_EOK            -- Success  
  #QURT_EVAL           -- Failure because of invalid inputs.
  #QURT_EPRIVILEGE     -- Failure because caller does not have enough privilege for this operation.
  #QURT_ENOTSUPPORTED  -- Failure because of the operation is not supported due to limitation in HW capabilities 
 
  @dependencies
  None.
 */
int qurt_process_vtcm_window_get(int pid, unsigned int *enable, unsigned int *high_offset, unsigned int *low_offset);

/**@ingroup func_qurt_process_set_group_config
  Enable thread groups in the process with the ceiling priorities setup

  @param[in] process_id Process identifier.
  @param[in] group_bitmask 64-bit mask of active thread groups
  @param[in] ceiling_priorities array of ceiling priorities for thread group

  @note1hang
  This API can only be called by root PD and can only be called once for each process, otherwise it will be
  rejected. Group 0 must be enabled in group_bitmask, otherwise QuRT will return error. After this API, all
  exisiting threads will be moved to group 0, and if there is any thread's priority higher than ceiling
  priority of group 0, it will be lowered to the ceiling value.
  Examples 1:
  group_bitmask = 0xD7; //'b11010111
  ceiling_priorities[] = {100, 128, 200, 0, 196, 0, 240, 20}; // 0 - does not care
  Exmaples 2:
  group_mask = 0x5;     //'b101
  ceiling_priorities[] = {240, 0, 20}; // 0 - does not care


  @return
  #QURT_EOK            -- Success.
  #QURT_EVAL           -- Failure because of invalid inputs.
  #QURT_ENOTALLOWED    -- The group has been configured already.

  @dependencies
  None.
 */
int qurt_process_set_group_config(unsigned int process_id, unsigned long long group_bitmask,
    unsigned char *ceiling_priorities);


/**@ingroup func_qurt_process_stid_set
  Set the specified stid for a process or for a thread group within a process. 

  @param[in] pid Process identifier.
  @param[in] group_id  group identifier
  @param[in] stid stid to be set 
  
  @note1hang 
  User can pass default group_id (QURT_THREAD_DEFAULT_GROUP_ID) if stid needs to set at a process level.
  All threads within a process that has default stid (QURT_STID_DEFAULT) will inherit the stid set for a process.
  When a non-default group_id is specified, the stid is set only for a thread group.
  
  @return
  #QURT_EOK            -- Success
  #QURT_EFATAL         -- Invalid PID
  #QURT_EVAL           -- Failure because of invalid inputs.
  #QURT_EPRIVILEGE     -- Failure because caller does not have enough privilege for this operation.
 
  @dependencies
  None.
 */
int qurt_process_stid_set(unsigned int pid, unsigned int group_id , unsigned int stid);

/**@ingroup func_qurt_process_stid_get
  Get the stid for a process or for a thread group within a process. 

  @param[in]  pid Process identifier.
  @param[in]  group_id  group identifier
  @param[out] Pointer to a variable to return  stid
  
  @note1hang 
  User can pass default group_id (QURT_THREAD_DEFAULT_GROUP_ID) to return process-level stid.
  When a non-default group_id is specified, the stid is returned only for a thread group.
  
  @return
  #QURT_EOK            -- Success
  #QURT_EFATAL         -- Invalid PID
  #QURT_EVAL           -- Failure because of invalid inputs.
  #QURT_EPRIVILEGE     -- Failure because caller does not have enough privilege for this operation.
 
  @dependencies
  None.
 */
int qurt_process_stid_get(unsigned int pid, unsigned int group_id , unsigned int *stid);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
