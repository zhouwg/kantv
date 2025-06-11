#ifndef QURT_SRM_CONSTS_H
#define QURT_SRM_CONSTS_H
/**
  @file qurt_srm_consts.h 
  @brief  Type definitions for srm

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2020-2021, 2022  by Qualcomm Technologies, Inc.  All Rights Reserved
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */
#define QURT_SRM_WAKEUP_REQUEST       1U << 0          /**< Value = 1:  Send wakeup request to the SRM server. */
#define QURT_SRM_SET_HANDLE           1U << 1          /**< Value = 2:  Set the client handle for a new SRM client. */
#define QURT_SRM_ALLOC_KERNEL_PAGES   1U << 2          /**< Value = 4:  Allocate pages from the kernel VA space. */
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_SRM_CONSTS_H */
