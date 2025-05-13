#ifndef QURT_ERROR_H
#define QURT_ERROR_H

/**
  @file qurt_error.h 
  Error results- QURT defines a set of standard symbols for the error result values. This file lists the
  symbols and their corresponding values.

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021-2022 , 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc..
 ======================================================================*/
#include "qurt_except.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup chapter_error
@{ */

/*=====================================================================
Constants and macros
======================================================================*/
#define QURT_EOK                             0  /**< Operation successfully performed. */
#define QURT_EVAL                            1  /**< Wrong values for the parameters. The specified page does not exist. */
#define QURT_EMEM                            2  /**< Not enough memory to perform the operation.*/

#define QURT_EINVALID                        4  /**< Invalid argument value; invalid key. */ 
/** @cond  */
#define QURT_EUNKNOWN                        6  /**< Defined but never used in QuRT. */
#define QURT_ENOMSGS                         7  /**< Message queue is empty. */ 
#define QURT_EBADF                           9  /**< Bad message queue descriptor. */
/** @endcond */
#define QURT_EFAILED                        12  /**< Operation failed. */ 

#define QURT_ENOTALLOWED                    13  /**< Operation not allowed. */

/** @cond */
#define QURT_EDUPCLSID                      14  /*< Duplicate class ID. */
/** @endcond */
/** @cond rest_reg_dist   */
#define QURT_ENOREGISTERED                  20  /**< No registered interrupts.*/ 
/** @endcond */


/** @cond */
#define QURT_EISDB                          21  /*< Power collapse failed due to ISDB being enabled. */
#define QURT_ESTM                           22  /*< Power collapse failed in a Single-threaded mode check. */
/** @endcond */


/** @cond rest_reg_dist  */
#define QURT_ETLSAVAIL                      23  /**< No free TLS key is available. */
#define QURT_ETLSENTRY                      24  /**< TLS key is not already free. */ 
/** @endcond */

#define QURT_EINT                           26  /**< Invalid interrupt number (not registered). */  
/** @cond rest_reg_dist */
#define QURT_ESIG                           27  /**< Invalid signal bitmask (cannot set more than one signal at a time). */
/** @endcond */

/** @cond */
#define QURT_EHEAP                          28  /**< No heap space is available. */
#define QURT_ENOSPC                         28  /**< No space to create another queue in the system. */
#define QURT_EMEMMAP                        29  /**< Physical address layout is not supported by the kernel. */
/** @endcond */
/** @cond rest_reg_dist */
#define QURT_ENOTHREAD                      30  /**< Thread no longer exists. */
/** @endcond */
/** @cond */
#define QURT_EL2CACHE                       31  /**< L2cachable is not supported in kernel invalidate/cleaninv. */
/** @endcond */
/** @cond rest_reg_dist  */
#define QURT_EALIGN                         32  /**< Not aligned. */
#define QURT_EDEREGISTERED                  33  /**< Interrupt is already deregistered.*/
/** @endcond */

/** @cond internal_only */

#define QURT_ETLBCREATESIZE                 34  /**< TLB create error -- Incorrect size.*/
#define QURT_ETLBCREATEUNALIGNED            35  /**< TLB create error -- Unaligned address.*/
/** @endcond */
/** @cond rest_reg_dist*/
#define QURT_EEXISTS                        35  /**< File or message queue already exists. */
#define QURT_ENAMETOOLONG                   36  /**< Name too long for message queue creation. */
#define QURT_EPRIVILEGE                     36  /**< Caller does not have privilege for this operation.*/

#define QURT_ECANCEL                        37  /**< A cancellable request was canceled because the associated process was asked to exit.*/
/** @endcond */

/** @cond */
#define QURT_EISLANDTRAP                    38  /*< Unsupported TRAP is called in Island mode.*/ 

#define QURT_ERMUTEXUNLOCKNONHOLDER         39  /*< Rmutex unlock by a non-holder.*/
#define QURT_ERMUTEXUNLOCKFATAL             40  /*< Rmutex unlock error, all except the non-holder error.*/
#define QURT_EMUTEXUNLOCKNONHOLDER          41  /*< Mutex unlock by a non-holder.*/
#define QURT_EMUTEXUNLOCKFATAL              42  /*< Mutex unlock error, all except the non-holder error.*/
#define QURT_EINVALIDPOWERCOLLAPSE          43  /*< Invalid power collapse mode requested. */ 
/** @endcond */
#define QURT_EISLANDUSEREXIT                44  /**< User call has resulted in island exit.*/
#define QURT_ENOISLANDENTRY                 45  /**< Island mode had not yet been entered.*/
#define QURT_EISLANDINVALIDINT              46  /**< Exited Island mode due to an invalid island interrupt.*/
/** @cond rest_reg_dist */
#define QURT_ETIMEDOUT                      47  /**< Operation timed-out. */
#define QURT_EALREADY                       48  /**< Operation already in progress. */
/** @endcond */

#define QURT_ERETRY                         49  /*< Retry the operation. */
#define QURT_EDISABLED                      50  /*< Resource disabled. */
#define QURT_EDUPLICATE                     51  /*< Duplicate resource. */
#define QURT_EBADR                          53  /*< Invalid request descriptor. */
#define QURT_ETLB                           54  /*< Exceeded maximum allowed TLBs. */
#define QURT_ENOTSUPPORTED                  55  /*< Operation not supported. */
/** @cond rest_reg_dist */
#define QURT_ENORESOURCE                    56  /**< No resource. */
/** @endcond */

#define QURT_EDTINIT                        57  /**< Problem with device tree intialization. */
#define QURT_EBUFLOCK                       58  /*< Buffer lock failed because it was already locked many times. */
#define QURT_ELOCKED                        59  /**< Current operation failed as the buffer is locked. */
#define QURT_EMSGSIZE                       90  /*< Message queue msg_len is greater than mq_msgsize attribute of the message queue. */


#define QURT_ENOTCONFIGURED                 91  /*< Interrupt is NOT configured. */

#define QURT_EBANDWIDTHLIMIT                92  /*< Message queue send exceed the bandwidth limit. */

#define QURT_ECFIVIOLATION                  93  /*< CFI violation detected. */

#define QURT_EDESTROY                       94  /**< A destroy request was made to waiting threads.*/

#define QURT_EHMXNOTAVAIL                   95  /**< HMX is not available to target thread.*/
#define QURT_EHMXNOTDETACHABLE              96  /**< HMX is not detachable from target thread.*/

#define QURT_EFATAL                         -1  /**< Fatal error. */

/** @} */ /* end_addtogroup chapter_error */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ERROR_H */
