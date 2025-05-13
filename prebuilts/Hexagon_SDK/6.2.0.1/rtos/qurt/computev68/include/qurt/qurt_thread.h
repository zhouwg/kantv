#ifndef QURT_THREAD_H
#define QURT_THREAD_H
/**
  @file qurt_thread.h 
  @brief Prototypes of Thread API

  EXTERNAL FUNCTIONS
   None.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2018, 2020-2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/


/* The followings are for C code only */
#ifndef __ASSEMBLER__ 
#include <string.h>
#include "qurt_pmu.h"
#include "qurt_api_version.h"
#endif /* __ASSEMBLER__ */
#include "qurt_consts.h"
#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                            CONSTANTS AND MACROS
=============================================================================*/


/*
  Bitmask configuration to select DSP hardware threads. 
  To select all the hardware threads, use #QURT_THREAD_CFG_BITMASK_ALL 
  and the following: \n
  - For QDSP6 V2/V3, all six hardware threads are selected \n
  - For QDSP6 V3L, all four hardware threads are selected \n
  - For QDSP6 V4, all three hardware threads are selected
 */  

#define QURT_THREAD_CFG_BITMASK_HT0      0x00000001   /**< HTO. */
#define QURT_THREAD_CFG_BITMASK_HT1      0x00000002   /**< HT1. */
#define QURT_THREAD_CFG_BITMASK_HT2      0x00000004   /**< HT2. */ 
#define QURT_THREAD_CFG_BITMASK_HT3      0x00000008   /**< HT3. */
#define QURT_THREAD_CFG_BITMASK_HT4      0x00000010   /**< HT4. */
#define QURT_THREAD_CFG_BITMASK_HT5      0x00000020   /**< HT5. */
/** @cond rest_reg_dist */
/** @addtogroup thread_macros
@{ */
/**   @xreflabel{sec:qurt_thread_cfg} */  

#define QURT_THREAD_CFG_BITMASK_ALL      0x000000ffU   /**< Select all the hardware threads. */
/** @} */ /* end_addtogroup thread_macros */
/** @endcond */

#define QURT_THREAD_CFG_USE_RAM          0x00000000   /**< Use RAM. */
#define QURT_THREAD_CFG_USE_TCM          0x00000100   /**< Use TCM. */
/** @cond rest_reg_dist */
/** @addtogroup thread_macros
@{ */
#define QURT_THREAD_BUS_PRIO_DISABLED    0   /**< Thread internal bus priority disabled. */
#define QURT_THREAD_BUS_PRIO_ENABLED     1   /**< Thread internal bus priority enabled.  */
/** @} */ /* end_addtogroup thread_macros */
/** @endcond */

#define QURT_THREAD_AUTOSTACK_DISABLED    0   /**< Thread has autostack v2 feature disabled. */
#define QURT_THREAD_AUTOSTACK_ENABLED     1   /**< Thread has autostack v2 feature enabled.  */

/*
   Macros for QuRT thread attributes.   
 */
#define QURT_HTHREAD_L1I_PREFETCH      0x1     /**< Enables hardware L1 instruction cache prefetching. */
#define QURT_HTHREAD_L1D_PREFETCH      0x2     /**< Enables hardware L1 data cache prefetching. */
#define QURT_HTHREAD_L2I_PREFETCH      0x4     /**< Enables hardware L2 instruction cache prefetching. */
#define QURT_HTHREAD_L2D_PREFETCH      0x8     /**< Enables hardware L2 data cache prefetching. */
#define QURT_HTHREAD_DCFETCH           0x10    /**< Enables DC fetch to the provided virtual address. 
                                                    DC fetch indicates the hardware that a data memory access is likely. 
                                                    Instructions are dropped when there is high bus utilization. */
/** @addtogroup thread_macros
@{ */
/** @xreflabel{hdr:partition_tcm} */
/*
   Below value is used to create legacy QuRT threads by default.
   If a thread has this as the detach_state, the thread can be joined
   on until it exits. When we are able to change default behavior of all
   QuRT threads to JOINABLE (posix default), we can remove this legacy
   behavior.
*/
#define QURT_THREAD_ATTR_CREATE_LEGACY               0U /**< Create a legacy QuRT thread by default. If a thread has this as a detach state, the thread can be joined on until it exits. */
#define QURT_THREAD_ATTR_CREATE_JOINABLE             1U /**< Create a joinable thread. */
#define QURT_THREAD_ATTR_CREATE_DETACHED             2U /**< Create a detached thread. */
/** @} */ /* end_addtogroup thread_macros */


#define QURT_THREAD_ATTR_NAME_MAXLEN            16  /**< Maximum name length. */
#define QURT_THREAD_ATTR_TCB_PARTITION_RAM      0  /**< Creates threads in RAM/DDR. */
#define QURT_THREAD_ATTR_TCB_PARTITION_TCM      1  /**< Creates threads in TCM. */
/** @cond rest_reg_dist */
/** @addtogroup thread_macros
@{ */
#define QURT_THREAD_ATTR_TCB_PARTITION_DEFAULT  QURT_THREAD_ATTR_TCB_PARTITION_RAM  /**< Backward compatibility. */
#define QURT_THREAD_ATTR_PRIORITY_DEFAULT       254   /**< Priority.*/
#define QURT_THREAD_ATTR_ASID_DEFAULT           0    /**< ASID. */
#define QURT_THREAD_ATTR_AFFINITY_DEFAULT      (-1)  /**< Affinity. */
#define QURT_THREAD_ATTR_BUS_PRIO_DEFAULT       255  /**< Bus priority. */
#define QURT_THREAD_ATTR_AUTOSTACK_DEFAULT      0    /**< Default autostack v2 disabled thread. */
#define QURT_THREAD_ATTR_TIMETEST_ID_DEFAULT   (-2)  /**< Timetest ID. */
#define QURT_THREAD_ATTR_STID_DEFAULT           QURT_STID_DEFAULT  /**< STID. */
#define QURT_THREAD_ATTR_STID_ENABLE            1  /**< Indicate to allocate STID during thread creation. */

#define  QURT_PRIORITY_FLOOR_DEFAULT            255U  /**< Default floor. */
/** @} */ /* end_addtogroup thread_macros */

// Option for suspending thread
#define  QURT_THREAD_SUSPEND_SYNCHRONOUS   0x0U  // bit#0
#define  QURT_THREAD_SUSPEND_ASYNCHRONOUS  0x1U  // bit#0
#define  QURT_THREAD_SUSPEND_KEEP_HMX      0x0U  // bit#1
#define  QURT_THREAD_SUSPEND_DETACH_HMX    0x2U  // bit#1
 
// Option for resuming thread
#define  QURT_THREAD_RESUME_DEFAULT        0x0

// Thread property IDs
#define  QURT_THREAD_PROPERTY_SUSPENDABLE  0x0U 
#define  QURT_THREAD_PROPERTY_RESUMABLE    0x1

// Thread group
#define  QURT_THREAD_DEFAULT_GROUP_ID      0x0U
#define  QURT_THREAD_GROUP_ID_MASK         0x3FU

/** @endcond*/


/* The followings are for C code only */
#ifndef __ASSEMBLER__ 
/*=============================================================================
                                TYPEDEFS
=============================================================================*/
/** @addtogroup thread_types
@{ */
/** @cond rest_reg_dist  */
typedef unsigned int qurt_cache_partition_t; /**< QuRT cache partition type. */

#define CCCC_PARTITION      0U     /**< Use the CCCC page attribute bits to determine the main or auxiliary partition. */
#define MAIN_PARTITION      1U     /**< Use the main partition. */
#define AUX_PARTITION       2U     /**< Use the auxiliary partition. */
#define MINIMUM_PARTITION   3U     /**< Use the minimum. Allocates the least amount of cache (no-allocate policy possible) for this thread. */
/** @endcond */

/** Thread ID type. */
typedef unsigned int qurt_thread_t;

/** @cond rest_reg_dist  */
/** Thread attributes. */
typedef struct _qurt_thread_attr {
    
    char name[QURT_THREAD_ATTR_NAME_MAXLEN]; /**< Thread name. */
    unsigned char tcb_partition;  /**< Indicates whether the thread TCB resides in RAM or
                                       on chip memory (TCM). */
    unsigned char  stid;          /**< Software thread ID used to configure the stid register
                                       for profiling purposes. */
    unsigned short priority;      /**< Thread priority. */
    unsigned char  autostack:1;   /**< Autostack v2 enabled thread. */
    unsigned char  group_id:6;    /**< Group ID. */
    unsigned char  reserved:1;    /**< Reserved bits. */
    unsigned char  bus_priority;  /**< Internal bus priority. */
    unsigned short timetest_id;   /**< Timetest ID. */
    unsigned int   stack_size;    /**< Thread stack size. */
    void *stack_addr;             /**< Pointer to the stack address base. The range of the stack is
                                       (stack_addr, stack_addr+stack_size-1). */
    unsigned short detach_state;  /**< Detach state of the thread. */

} qurt_thread_attr_t;
/** @endcond */

/** @cond rest_reg_dist */
/** Dynamic TLS attributes. */
typedef struct qurt_tls_info {
  unsigned int module_id;        /**< Module ID of the loaded dynamic linked library. */
  unsigned int tls_start;        /**< Start address of the TLS data. */
  unsigned int tls_data_end;     /**< End address of the TLS RW data. */
  unsigned int tls_end;          /**< End address of the TLS data. */
}qurt_tls_info;
/** @endcond */

/** @} */ /* end_addtogroup thread_types */

/*=============================================================================
                       FUNCTIONS
=============================================================================*/
/**@ingroup func_qurt_thread_attr_init
  Initializes the structure used to set the thread attributes when a thread is created.
  After an attribute structure is initialized, Explicity set the individual attributes in the structure 
  using the thread attribute operations.

  The initialize operation sets the following default attribute values: \n
  - Name -- NULL string \n
  - TCB partition -- QURT_THREAD_ATTR_TCB_PARTITION_DEFAULT
  - Priority -- QURT_THREAD_ATTR_PRIORITY_DEFAULT \n
  - Autostack -- QURT_THREAD_ATTR_AUTOSTACK_DEFAULT \n
  - Bus priority -- QURT_THREAD_ATTR_BUS_PRIO_DEFAULT \n
  - Timetest ID -- QURT_THREAD_ATTR_TIMETEST_ID_DEFAULT \n
  - stack_size -- 0 \n
  - stack_addr -- NULL \n
  - detach state -- #QURT_THREAD_ATTR_CREATE_LEGACY \n
  - STID -- #QURT_THREAD_ATTR_STID_DEFAULT

  @datatypes
  #qurt_thread_attr_t
  
  @param[in,out] attr Pointer to the thread attribute structure.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_init (qurt_thread_attr_t *attr)
{

    attr->name[0] = '\0';
    attr->tcb_partition = QURT_THREAD_ATTR_TCB_PARTITION_DEFAULT;
    attr->priority = QURT_THREAD_ATTR_PRIORITY_DEFAULT;
    attr->autostack = QURT_THREAD_ATTR_AUTOSTACK_DEFAULT; /* Default attribute for autostack v2*/
    attr->bus_priority = QURT_THREAD_ATTR_BUS_PRIO_DEFAULT;
    attr->timetest_id = (unsigned short)QURT_THREAD_ATTR_TIMETEST_ID_DEFAULT;
    attr->stack_size = 0;
    attr->stack_addr = NULL;
    attr->detach_state = QURT_THREAD_ATTR_CREATE_LEGACY;
    attr->stid = QURT_THREAD_ATTR_STID_DEFAULT;
    attr->group_id = QURT_THREAD_DEFAULT_GROUP_ID;
}

/**@ingroup func_qurt_thread_attr_set_name
  Sets the thread name attribute.\n
  This function specifies the name to use by a thread.
  Thread names identify a thread during debugging or profiling.
  Maximum name length is 16 charactes  \n
  @note1hang Thread names differ from the kernel-generated thread identifiers used to
  specify threads in the API thread operations.

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] name     Pointer to the character string containing the thread name.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_name (qurt_thread_attr_t *attr, const char *name)
{
    strlcpy (attr->name, name, QURT_THREAD_ATTR_NAME_MAXLEN);
    attr->name[QURT_THREAD_ATTR_NAME_MAXLEN - 1] = '\0';
}


/**@ingroup func_qurt_thread_attr_set_tcb_partition
  Sets the thread TCB partition attribute.
  Specifies the memory type where a TCB of a thread is allocated.
  Allocates TCBs in RAM or TCM/LPM.

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr  Pointer to the thread attribute structure.
  @param[in] tcb_partition TCB partition. Values:\n
                     - 0 -- TCB resides in RAM \n
                     - 1 -- TCB resides in TCM/LCM @tablebulletend

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_tcb_partition (qurt_thread_attr_t *attr, unsigned char tcb_partition)
{
    attr->tcb_partition = tcb_partition;
}

/**@ingroup func_qurt_thread_attr_set_priority
  Sets the thread priority to assign to a thread.
  Thread priorities are specified as numeric values in the range 1 to 254, with 1 representing
  the highest priority.
  Priority 0 and 255  are internally used by the kernel for special purposes.

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] priority Thread priority.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_priority (qurt_thread_attr_t *attr, unsigned short priority)
{
    attr->priority = priority;
}

/**@ingroup func_qurt_thread_attr_set_detachstate
  Sets the thread detach state with which thread is created.
  Thread detach state is either joinable or detached; specified by the following values:
  - #QURT_THREAD_ATTR_CREATE_JOINABLE  \n           
  - #QURT_THREAD_ATTR_CREATE_DETACHED  \n   

  When a detached thread is created (QURT_THREAD_ATTR_CREATE_DETACHED), its thread
  ID and other resources are reclaimed as soon as the thread exits. When a joinable thread 
  is created (QURT_THREAD_ATTR_CREATE_JOINABLE), it is assumed that some
  thread waits to join on it using a qurt_thread_join() call. 
  By default, detached state is QURT_THREAD_ATTR_CREATE_LEGACY
  If detached state is QURT_THREAD_ATTR_CREATE_LEGACY then other
  thread can join before thread exits but it will not wait other thread to join.
  
  @note1hang For a joinable thread (QURT_THREAD_ATTR_CREATE_JOINABLE), it is very
             important that some thread joins on it after it terminates, otherwise
			 the resources of that thread are not reclaimed, causing memory leaks.      

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] detachstate Thread detach state.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_detachstate (qurt_thread_attr_t *attr, unsigned short detachstate)
{	
    if(detachstate == QURT_THREAD_ATTR_CREATE_JOINABLE  || detachstate == QURT_THREAD_ATTR_CREATE_DETACHED){
		attr->detach_state = detachstate;
	}
}


/**@ingroup func_qurt_thread_attr_set_timetest_id
  Sets the thread timetest attribute.\n
  Specifies the timetest identifier to use by a thread.

  Timetest identifiers are used to identify a thread during debugging or profiling. \n
  @note1hang Timetest identifiers differ from the kernel-generated thread identifiers used to
             specify threads in the API thread operations.

  @datatypes
  #qurt_thread_attr_t
  
  @param[in,out] attr   Pointer to the thread attribute structure.
  @param[in] timetest_id Timetest identifier value.

  @return
  None.

  @dependencies
  None.
  */
static inline void qurt_thread_attr_set_timetest_id (qurt_thread_attr_t *attr, unsigned short timetest_id)
{
    attr->timetest_id = timetest_id;
}

/**@ingroup func_qurt_thread_attr_set_stack_size
  @xreflabel{sec:set_stack_size}
  Sets the thread stack size attribute.\n
  Specifies the size of the memory area to use for a call stack of a thread.

  The thread stack address (Section @xref{sec:set_stack_addr}) and stack size specify the memory area used as a
  call stack for the thread. The user is responsible for allocating the memory area used for
  the stack.

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] stack_size Size (in bytes) of the thread stack.

  @return
  None.

  @dependencies
  None.
*/

static inline void qurt_thread_attr_set_stack_size (qurt_thread_attr_t *attr, unsigned int stack_size)
{
    attr->stack_size = stack_size;
}

/**@ingroup func_qurt_thread_attr_set_stack_size2
  @xreflabel{sec:set_stack_size}
  Sets the thread stack size attribute for island threads that require a higher guest OS stack size than the stack size
  defined in the configuration XML.\n
  Specifies the size of the memory area to use for a call stack of an island thread in User and Guest mode.

  The thread stack address (Section @xref{sec:set_stack_addr}) and stack size specify the memory area used as a
  call stack for the thread. The user is responsible for allocating the memory area used for
  the stack.

  @datatypes
  #qurt_thread_attr_t

  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] user_stack_size Size (in bytes) of the stack usage in User mode.
  @param[in] root_stack_size Size (in bytes) of the stack usage in Guest mode.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_stack_size2 (qurt_thread_attr_t *attr, unsigned short user_stack_size, unsigned short root_stack_size)
{
	union qurt_thread_stack_info{
		unsigned int raw_size;
		struct{
			unsigned short user_stack;
			unsigned short root_stack;
		};
	}user_root_stack_size;
	user_root_stack_size.user_stack = user_stack_size;
	user_root_stack_size.root_stack = root_stack_size;
	
    attr->stack_size = user_root_stack_size.raw_size;
}

/**@ingroup func_qurt_thread_attr_set_stack_addr
  @xreflabel{sec:set_stack_addr}
  Sets the thread stack address attribute. \n
  Specifies the base address of the memory area to use for a call stack of a thread.

  stack_addr must contain an address value that is 8-byte aligned.

  The thread stack address and stack size (Section @xref{sec:set_stack_size}) specify the memory area used as a
  call stack for the thread. \n
  @note1hang The user is responsible for allocating the memory area used for the thread
             stack. The memory area must be large enough to contain the stack that the thread
			 creates.

  @datatypes
  #qurt_thread_attr_t
  
  @param[in,out] attr Pointer to the thread attribute structure.
  @param[in] stack_addr  Pointer to the 8-byte aligned address of the thread stack.

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_stack_addr (qurt_thread_attr_t *attr, void *stack_addr)
{
    attr->stack_addr = stack_addr;
}

/**@ingroup func_qurt_thread_attr_set_bus_priority
   Sets the internal bus priority state in the Hexagon core for this software thread attribute. 
   Memory requests generated by the thread with bus priority enabled are
   given priority over requests generated by the thread with bus priority disabled. 
   The default value of bus priority is disabled.

   @note1hang Sets the internal bus priority for Hexagon processor version V60 or greater. 
              The priority is not propagated to the bus fabric.
  
   @datatypes
   #qurt_thread_attr_t

   @param[in] attr Pointer to the thread attribute structure.

   @param[in] bus_priority Enabling flag. Values: \n 
         - #QURT_THREAD_BUS_PRIO_DISABLED \n
         - #QURT_THREAD_BUS_PRIO_ENABLED @tablebulletend

   @return
   None

   @dependencies
   None.
*/
static inline void qurt_thread_attr_set_bus_priority ( qurt_thread_attr_t *attr, unsigned short bus_priority)
{
    attr->bus_priority = (unsigned char)bus_priority;
}

/**@ingroup func_qurt_thread_attr_set_autostack
   Enables autostack v2 feature in the thread attributes.
   
   When autostack is enabled by the subsystem, in the case that
   an autostack enabled thread gets framelimit exception, kernel will
   allocate more stack for thread and return to normal execution. 

   If autostack is not enabled by the subsystem, or it is not enabled
   for the thread, the framelimit exception will be fatal.
  
   @datatypes
   #qurt_thread_attr_t

   @param[in] attr Pointer to the thread attribute structure.
   @param[in] autostack  Autostack enable or disable flag. Values: \n 
         - #QURT_THREAD_AUTOSTACK_DISABLED \n
         - #QURT_THREAD_AUTOSTACK_ENABLED @tablebulletend

   @return
   None.

   @dependencies
   None.
*/
static inline void qurt_thread_attr_set_autostack ( qurt_thread_attr_t *attr, unsigned short autostack)
{
    attr->autostack = (unsigned char)autostack;  
}
/**@ingroup qurt_thread_attr_enable_stid
   Set STID in the thread attributes.
  
   @datatypes
   #qurt_thread_attr_t

   @param[in] attr Pointer to the thread attribute structure.
   @param[in] enable_stid  STID to be set. Values: \n 
         - #QURT_THREAD_ATTR_STID_DEFAULT (0): Default STID. \n
         - #QURT_THREAD_ATTR_STID_ENABLE (1):  QuRT assigns an STID that is not already in use \n
         - #2 through #255 : User provided STID.  @tablebulletend

   @return
   None.

   @dependencies
   None.
*/
static inline void qurt_thread_attr_enable_stid ( qurt_thread_attr_t *attr, char enable_stid)
{
    if (enable_stid != '\0') {
        attr->stid = enable_stid;
    }
    else
    {
        attr->stid = QURT_THREAD_ATTR_STID_DEFAULT;
    }
}

/**@ingroup func_qurt_thread_attr_set_stid
   Sets the stid thread attribute.
   The default stid value is QURT_THREAD_ATTR_STID_DEFAULT

   @note1hang When a thread is created with non default stid , 
   the stid set in thread attribute  will be assigned to a thread.
  
   @datatypes
   #qurt_thread_attr_t

   @param[in] attr Pointer to the thread attribute structure.
   @param[in] stid Stid to be set for a thread.

   @return
   None

   @dependencies
   None.
*/
static inline void qurt_thread_attr_set_stid( qurt_thread_attr_t *attr, unsigned int stid){
    attr->stid = stid;
}

/**@ingroup func_qurt_thread_attr_set_group_id
  Sets group id in the thread attributes.
  Primordial/first thread has group ID 0.
  If a new thread is created without assigning group_id, it
  inherits the group ID from its parent thread.

  @note1hang
  1) Group ID can only be set before creating a thread. It cannot be
  changed after the thread is created.
  2) If a non-activated group_id is passed, thread creation will fail.
  3) Only a thread with Group ID #0 can set Group ID for its child threads.
  4) If thread with non-zero group ID set the group ID for its child threads,
  QuRT will ingore this parameter and child threads will inherit the parent
  thread's group ID. But if passed group ID is not activated, thread creation
  will still fail.

  @datatypes
  #qurt_thread_attr_t

  @param[in] attr Pointer to the thread attribute structure.
  @param[in] group_id Group identifier. Its valid range is 0 ~ 63

  @return
  None.

  @dependencies
  None.
*/
static inline void qurt_thread_attr_set_group_id(qurt_thread_attr_t *attr, unsigned int group_id)
{
    attr->group_id = group_id & QURT_THREAD_GROUP_ID_MASK;
}

/**@ingroup func_qurt_thread_set_autostack
  Sets autostack enable in the TCB.

  @param[in] Pointer to UGP

  @return
  None.

  @dependencies
  None.
*/

void qurt_thread_set_autostack(void *);


/**@ingroup func_qurt_thread_get_name
  Gets the thread name of current thread.\n
  Returns the thread name of the current thread. 
  Thread names are assigned to threads as thread attributes, see qurt_thread_attr_set_name(). Thread names 
  identify a thread during debugging or profiling.

  @param[out] name Pointer to a character string, which specifies the address where the returned thread name is stored.
  @param[in] max_len Maximum length of the character string that can be returned.

  @return
  None.

  @dependencies
  None.
*/
void qurt_thread_get_name (char *name, unsigned char max_len);

/**@ingroup func_qurt_thread_create
  @xreflabel{hdr:qurt_thread_create}
  Creates a thread with the specified attributes, and makes it executable.

  @datatypes
  #qurt_thread_t \n
  #qurt_thread_attr_t
  
  @param[out]  thread_id    Returns a pointer to the thread identifier if the thread was 
                             successfully created.
  @param[in]   attr 	    Pointer to the initialized thread attribute structure that specifies 
                             the attributes of the created thread.
  @param[in]   entrypoint   C function pointer, which specifies the main function of a thread.
  @param[in]   arg  	     Pointer to a thread-specific argument structure
  
   
  @return 
  #QURT_EOK -- Thread created. \n
  #QURT_EFAILED -- Thread not created. 

  @dependencies
  None.
 */
int qurt_thread_create (qurt_thread_t *thread_id, qurt_thread_attr_t *attr, void (*entrypoint) (void *), void *arg);

/**@ingroup func_qurt_thread_stop
   Stops the current thread, frees the kernel TCB, and yields to the next highest ready thread. 
  
   @return
   void 

   @dependencies
   None.
 */
void qurt_thread_stop(void);

/** @cond internal_only */
/**@ingroup func_qurt_thread_resume
   When a demand-loading paging solution is enabled, this function
   will resumes the execution of a thread that was suspended due to
   a page miss.
  
   @param[in]  thread_id Thread identifier.

   @return 
   #QURT_EOK -- Thread successfully resumed. \n
   #QURT_EFATAL -- Resume operation failed.

   @dependencies
   None.
 */
int qurt_thread_resume(unsigned int thread_id);
/** @endcond */

/**@ingroup func_qurt_thread_get_id
   Gets the identifier of the current thread.\n
   Returns the thread identifier for the current thread.
     
   @return 
   Thread identifier -- Identifier of the current thread. 

   @dependencies
   None.
 */
qurt_thread_t qurt_thread_get_id (void);


/**@ingroup func_qurt_thread_get_l2cache_partition
   Returns the current value of the L2 cache partition assigned to the caller thread.\n
     
   @return 
   Value of the #qurt_cache_partition_t data type.

   @dependencies
   None.
 */
qurt_cache_partition_t qurt_thread_get_l2cache_partition (void);

/**@ingroup func_qurt_thread_set_timetest_id
   Sets the timetest identifier of the current thread.
   Timetest identifiers are used to identify a thread during debugging or profiling.\n
   @note1hang Timetest identifiers differ from the kernel-generated thread identifiers used to
              specify threads in the API thread operations.

   @param[in]  tid  Timetest identifier.

   @return
   None.

   @dependencies
   None.
 */
void qurt_thread_set_timetest_id (unsigned short tid);

/**@ingroup func_qurt_thread_set_cache_partition
   Sets the cache partition for the current thread. This function uses the qurt_cache_partition_t type 
   to select the cache partition of the current thread for the L1 Icache, L1 Dcache, and L2 cache.
  
   @datatypes
   #qurt_cache_partition_t 

   @param[in] l1_icache L1 I cache partition.
   @param[in] l1_dcache L1 D cache partition.
   @param[in] l2_cache L2 cache partition.
    
   @return
   None.

   @dependencies
   None.
 */
void qurt_thread_set_cache_partition(qurt_cache_partition_t l1_icache, qurt_cache_partition_t l1_dcache, qurt_cache_partition_t l2_cache);


/**@ingroup func_qurt_thread_get_timetest_id
   Gets the timetest identifier of the current thread.\n
   Returns the timetest identifier of the current thread.\n
   Timetest identifiers are used to identify a thread during debugging or profiling. \n
   @note1hang Timetest identifiers differ from the kernel-generated thread identifiers used to
              specify threads in the API thread operations.

   @return 
   Integer -- Timetest identifier. 

   @dependencies
   None.
 */
unsigned short qurt_thread_get_timetest_id (void);

/**@ingroup func_qurt_thread_exit
   @xreflabel{sec:qurt_thread_exit}
   Stops the current thread, awakens threads joined to it, then destroys the stopped
   thread.

   Threads that are suspended on the current thread (by performing a thread join 
   Section @xref{sec:thread_join}) are awakened and passed a user-defined status value 
   that indicates the status of the stopped thread.

   @note1hang Exit must be called in the context of the thread to stop.
  
   @param[in]   status User-defined thread exit status value.

   @return
   None.

   @dependencies
   None.
 */
void qurt_thread_exit(int status);

/**@ingroup func_qurt_thread_join
   @xreflabel{sec:thread_join}
   Waits for a specified thread to finish; the specified thread is another thread within
   the same process.
   The caller thread is suspended until the specified thread exits. When the unspecified thread
   exits, the caller thread is awakened. \n
   @note1hang If the specified thread has already exited, this function returns immediately
              with the result value #QURT_ENOTHREAD. \n
   @note1cont Two threads cannot call qurt_thread_join to wait for the same thread to finish.
              If this occurs, QuRT generates an exception (see Section @xref{sec:exceptionHandling}).
  
   @param[in]   tid     Thread identifier.
   @param[out]  status  Destination variable for thread exit status. Returns an application-defined 
                        value that indicates the termination status of the specified thread. 
  
   @return  
   #QURT_ENOTHREAD -- Thread has already exited. \n
   #QURT_EOK -- Thread successfully joined with valid status value. 

   @dependencies
   None.
 */
int qurt_thread_join(unsigned int tid, int *status);

/**@ingroup qurt_thread_detach
   @xreflabel{sec:thread_detach}
   Detaches a joinable thread. The specified thread is another thread within the 
   same process. Create the thread as a joinable thread; only joinable threads 
   can be detached.
   If a joinable thread is detached, it finishes execution and exits.
  
   @param[in]   tid     Thread identifier.
   
   @return  
   #QURT_ENOTHREAD -- Thread specifed by TID does not exist. \n
   #QURT_EOK -- Thread successfully detached.

   @dependencies
   None.
 */
int qurt_thread_detach(unsigned int tid);


/**@ingroup func_qurt_thread_get_priority 
   Gets the priority of the specified thread. \n 
   Returns the thread priority of the specified thread.\n
   Thread priorities are specified as numeric values in a range as large as 1 through 254, with lower
   values representing higher priorities. 1 represents the highest possible thread priority. \n
   Priority 0 and 255 are internally used by the kernel for special purposes.

   @note1hang QuRT can be configured to have different priority ranges.

   @datatypes
   #qurt_thread_t
  
   @param[in]  threadid	   Thread identifier.	

   @return
   -1 -- Invalid thread identifier. \n
   1 through 254 -- Thread priority value.

   @dependencies
   None.
 */
int qurt_thread_get_priority (qurt_thread_t threadid);

/**@ingroup func_qurt_thread_set_priority
   Sets the priority of the specified thread.\n
   Thread priorities are specified as numeric values in a range as large as 1 through 254, with lower
   values representing higher priorities. 1 represents the highest possible thread priority.
   Priority 0 and 255  are internally used by the kernel  for special purposes.

   @note1hang QuRT can be configured to have different priority ranges. For more
              information, see Section @xref{sec:AppDev}.
   
   @datatypes
   #qurt_thread_t

   @param[in] threadid	    Thread identifier.	
   @param[in] newprio 	    New thread priority value.

   @return
   0 -- Priority successfully set. \n
   -1 -- Invalid thread identifier. \n 
   
   @dependencies
   None.
 */
int qurt_thread_set_priority (qurt_thread_t threadid, unsigned short newprio);



/**@ingroup func_qurt_thread_attr_get
  Gets the attributes of the specified thread.

  @datatypes
  #qurt_thread_t \n
  #qurt_thread_attr_t

  @param[in]  thread_id	    Thread identifier.
  @param[out] attr 	    Pointer to the destination structure for thread attributes.
  
  @return
  #QURT_EOK -- Success. \n
  #QURT_EINVALID -- Invalid argument.

  @dependencies
  None.
 */
int qurt_thread_attr_get (qurt_thread_t thread_id, qurt_thread_attr_t *attr);



/**@ingroup func_qurt_thread_get_tls_base
  Gets the base address of thread local storage (TLS) of a dynamically loaded module
  for the current thread.
  
  @datatypes
  #qurt_tls_info 

  @param[in]  info	   Pointer to the TLS information for a module.
  
  @return
   Pointer to the TLS object for the dynamically loaded module.\n
   NULL -- TLS information is invalid.

  @dependencies
  None.
 */
void * qurt_thread_get_tls_base(qurt_tls_info* info);

/**@ingroup func_qurt_thread_pktcount_get
  Gets the PKTCOUNT of a specified thread.

  @datatypes
  #qurt_thread_t 

  @param[in]  thread_id	    Thread identifier.
  
  @return
  PKTCOUNT

  @dependencies
  None.
 */

long long int qurt_thread_pktcount_get (qurt_thread_t thread_id);

/**@ingroup func_qurt_thread_pktcount_set
  Sets the PKTCOUNT for the current QuRT thread.
  
  @return
  Value to which pktcount is set.

  @dependencies
  None.
 */

long long int qurt_thread_pktcount_set (long long int);

/**@ingroup func_qurt_thread_stid_get
  Gets the STID for a specified thread.

  @datatypes
  #qurt_thread_t 

  @param[in]  thread_id	    Thread identifier.
  
  @return
  STID

  @dependencies
  None.
 */

char qurt_thread_stid_get(qurt_thread_t thread_id);
 
/**@ingroup func_qurt_thread_stid_get2
  Returns the set stid for a thread
  
  @param[in]  thread_id   thread identifier
  @param[out] stid  Pointer to a variable to return  stid
   
  @return
  QURT_EOK - success
  QURT_ENOTALLOWED   - operation not allowed for a thread
  QURT_EINVALID - Invalid input

  @dependencies
  None.
 */
int qurt_thread_stid_get2(unsigned int thread_id, unsigned int *stid);

/**@ingroup func_qurt_thread_stid_set
  Sets the STID for a specified thread. 

  @datatypes
  #qurt_thread_t 

  @param[in]  stid	    Thread identifier.
  
  @return 
   #QURT_EOK -- STID set created. \n
   #QURT_EFAILED -- STID not set. 

  @dependencies
  None.
 */

int qurt_thread_stid_set(char stid);

/**@ingroup qurt_thread_stid_set2
   Sets the stid for a specified thread.

   @datatypes
   #qurt_thread_attr_t

   @param[in]  thread_id  Thread identifier.
   @param[in]  stid       Stid to be set for a thread.

   @return
   QURT_EOK -- Success
   #QURT_EPRIVILEGE -- Failure because caller does not have enough privilege for this operation.
   #QURT_EVAL -- Failure because of invalid inputs.

   @dependencies
   None.
*/
int qurt_thread_stid_set2(unsigned int thread_id, unsigned int stid); 

/** @cond internal_only */
/**@ingroup func_qurt_thread_get_running_ids
  Returns the thread IDs of the running threads in the system; use only during fatal error handling.
 
  @datatypes
  #qurt_thread_t 
 
  @param[in,out] * Array of thread identifier of size #QURT_MAX_HTHREAD_LIMIT + 1.
 
  @return
   #QURT_EINVALID -- Incorrect argument \n
   #QURT_ENOTALLOWED  -- API not called during error handling \n
   #QURT_EOK -- Success, returns a NULL-terminated array of thread_id
 
  @dependencies
  None.
 */
int qurt_thread_get_running_ids(qurt_thread_t *);
/** @endcond */


/**@ingroup func_qurt_thread_get_thread_id
  Gets the thread identifier of the thread with the matching name in the same process
  of the caller.
 
  @datatypes
  #qurt_thread_t 
 
  @param[out] thread_id Pointer to the thread identifier.
  @param[in]  name      Pointer to the name of the thread.
 
  @return
  #QURT_EINVALID -- No thread with matching name in the process of the caller \n
  #QURT_EOK      -- Success  
 
  @dependencies
  None.
 */
int qurt_thread_get_thread_id (qurt_thread_t *thread_id, char *name);

/**@ingroup func_qurt_sleep
  Suspends the current thread for the specified amount of time.

  @note1hang Because QuRT timers are deferrable, this call is guaranteed to block
             at least for the specified amount of time. If power-collapse is 
             enabled, the maximum amount of time this call can block depends on
             the earliest wakeup from power-collapse past the specified duration.

  @param[in] duration  Duration (in microseconds) for which the thread is suspended.

  @return 
  None.

  @dependencies
  None.
 */
void qurt_sleep (unsigned long long int duration);


/**@ingroup func_qurt_system_set_priority_floor
  Sets a priority floor to move threads with thread priority lower than the floor out of the running state.
  Running threads with thread priority lower than the priority floor are moved into the kernel ready queue, and they 
  are not scheduled to run when the thread priority is lower than the floor.
  Later the caller should reset the priority floor back to the default value of QURT_PRIORITY_FLOOR_DEFAULT. 
  Threads in the kernel ready queue are scheduled to run when the thread priority is higher than the floor.

  The priority floor is set and associated to the user process of the caller. When the caller gets into QuRTOS and
  sets a new floor, the new floor is associated to its original user process, not the QuRTOS process.
  The floor associated to the user process is reset when the user process exits or is killed, but not at the time 
  when the user thread of the caller exits.

  The priority floor cannot be set to a priority higher than the thread priority of the caller.

  The priority floor cannot be set to a priority lower than the default #QURT_PRIORITY_FLOOR_DEFAULT system floor.

  This function is not supported in Island mode.

  After the system floor is set above QURT_PRIORITY_FLOOR_DEFAULT, power collapse is skipped, and sleep task 
  is not scheduled to run.
 
  @param[in]  priority_floor Priority floor. 
 
  @return
  #QURT_EOK         -- Success \n  
  #QURT_ENOTALLOWED -- Floor setting is not allowed
 
  @dependencies
  None.
 */
int qurt_system_set_priority_floor (unsigned int priority_floor);


/**@ingroup func_qurt_thread_suspend_thread 
  Suspend a QuRT thread with its thread identifier.
  The target thread can be in a signed user process or an unsigned user process.
  The caller thread can be a thread from the same user process of the target thread, or from its parent process.
  After the target thread is suspended, the kernel will not schedule it to run until it is resumed later.

  If the target thread is set as non-suspendable, this function call returns an error without suspending 
  the target thread. 

  If the target thread is already suspended, this function call returns success to confirm 
  the target thread suspend.                                          

  If the target thread is in a secure user process, or CPZ process, this function call returns an error without
  suspending the target thread.                                          

  If the target thread is running in the guest OS/root process via a QDI call, this function call does not suspend 
  the target thread in guest OS, but marks the target thread as suspend-pending. The target thread is
  suspended when it exits the guest OS, before executing the first instruction in the user process.
  In this case, the function returns success even with the #QURT_THREAD_SUSPEND_SYNCHRONOUS option, while the target
  thread can runn in the guest OS, and is suspended when exiting the guest OS. 
 
  QuRT debug monitor threads that are in a user process are non-suspendable. This function does not suspend 
  those threads.

  @param[in] thread_id  Thread identifier.
  @param[in] option     Optional argument, multiple options can be ORed. \n
                        #QURT_THREAD_SUSPEND_SYNCHRONOUS (default) -- set to synchronous function call,
                        the function returns after the thread is completely suspended.\n
                        #QURT_THREAD_SUSPEND_ASYNCHRONOUS -- set to asynchronous function call, the function returns
                        after the kernel acts to suspend the target thread. The target thread
                        might still be running before it is completely suspended. \n
                        #QURT_THREAD_SUSPEND_KEEP_HMX (default) -- keep the HMX attachment on the target thread 
                        if it locks the HMX with qurt_hmx_lock(). In this case, the HMX cannot be re-used by other threads. \n
                        #QURT_THREAD_SUSPEND_DETACH_HMX -- detach HMX from the target thread if it locks the HMX with qurt_hmx_lock().
                        Later when the target thread resumes, the HMX is re-attached to the thread. Note that, this option is only 
                        supported for the caller from the same user process of the target thread, not for a caller from the parent 
                        process of the target thread, or other processes. With the HMX detach option, Qurt does not save the HMX 
                        context. Thus, the HMX context state will be lost. It is the responsibility of caller to ensure HMX operations
                        and its context state saving when calling qurt_thread_suspend_thread() with the HMX detach option.
                        If a thread from another process uses this detach option, QURT_EHMXNOTDETACHABLE will be returned; in this 
                        case, if the caller is qualified to suspend the target thread, the target thread will be moved to suspended 
                        state without HMX detached.
 
  @return
  #QURT_EOK         -- Success  \n
  #QURT_EINVALID    -- Failure because of invalid thread_id input \n
  #QURT_ENOTALLOWED -- Failure because of the operation is not allowed, for example, in secure process/CPZ process.
  #QURT_EHMXNOTDETACHABLE -- Failure because HMX is not detachable from the target thread.
 
  @dependencies
  None.
 */
int qurt_thread_suspend_thread (unsigned int thread_id, unsigned int option);


/**@ingroup func_qurt_thread_resume_thread 
  Resume a QuRT thread with its thread identifier.
  The target thread can be in a signed user process or an unsigned user process.
  The caller thread can be a thread from the same user process of the target thread, or from its parent 
  process. After the target thread resumes, the kernel scheduler can schedule the thread to run based on 
  the thread priority.

  There is an option argument in this function, with only one default option as of now,
     QURT_THREAD_RESUME_DEFAULT: resume the target thread in default way.

  By default, this is an asynchronous function. The function returns after kernel moves the 
  target thread from suspended state to runnable state. The thread is scheduled to run based on its 
  thread priority.
  
  If the target thread is set as non-resumable, this function call does not resume the target thread.                                          

  If the target thread has already resumed, this function confirms that the target thread resumes
  by returning success.  

  If the target thread is in a secure user process or CPZ process, this function call returns an error without 
  resuming the operation.  

  If the target thread runs in the guest OS/root process via a QDI call, this function call clears the mark of
  suspend-pending on the target thread, and the target thread is not suspended when it exits the 
  guest OS. 
 
  @param[in] thread_id  Thread identifier.
  @param[in] option     Optional argument, #QURT_THREAD_RESUME_DEFAULT, which resumes the target thread.
 
  @return
  #QURT_EOK           -- Success \n 
  #QURT_EINVALID      -- Failure because of invalid thread_id input \n
  #QURT_ENOTALLOWED   -- Failure because of the operation is not allowed, for example, in a secure process/CPZ process.
  #QURT_EHMXNOTAVAIL  -- Failure because when resume a HMX thread, the HMX is not available/free for the HMX thread resume.
 
  @dependencies
  None.
 */
int qurt_thread_resume_thread (unsigned int thread_id, unsigned int option);


/**@ingroup func_qurt_thread_set_thread_property 
  Set a QuRT thread property with its thread identifier.
  The target thread can be in a signed user process or an unsigned user process.
  The caller thread can be from the same user process of the target thread, or from its parent process.

  If the target thread is in a secure user process, or CPZ process, this function call returns an error without 
  changing the property of the target thread.

  @param[in] thread_id    Thread identifier \n
  @param[in] property_id  Thread property identifier \n
                          #QURT_THREAD_PROPERTY_SUSPENDABLE -- thread is suspendable. Default is TRUE. \n
                          #QURT_THREAD_PROPERTY_RESUMEABLE  -- thread is resumable. Default is TRUE
  @param[in] value        Proper value: \n
                          TRUE(1) -- TRUE for the property \n
                          FALSE(0) -- FALSE for the property
 
  @return
  #QURT_EOK         -- Success  \n
  #QURT_EINVALID    -- Failure because of invalid thread_id input \n
  #QURT_ENOTALLOWED -- Failure because of the operation is not allowed, for example, in a secure process/CPZ process.
 
  @dependencies
  None.
 */
int qurt_thread_set_thread_property( unsigned int thread_id, unsigned int property_id, unsigned int value );    

/**@ingroup func_qurt_thread_get_group_id
  Get the group id of the thread specified by thread_id.\n

  @param[in] thread_id Thread identifier
  @param[out] group_id Pointer to the variable of group identifier

  @return
  #QURT_EOK         -- Success  \n
  #QURT_EINVALID    -- Thread id is invalid, or the process has no groups enabled \n
  #QURT_ENOTALLOWED -- Operation is not allowed \n

  @dependencies
  None.
*/
int qurt_thread_get_group_id(qurt_thread_t thread_id, unsigned int* group_id);

#endif /* __ASSEMBLER__ */ 

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_THREAD_H */
