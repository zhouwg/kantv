#ifndef QURT_PMEM_MANAGER_H
#define QURT_PMEM_MANAGER_H
/**
  @file qurt_pmem_manager.h
  Prototypes of kernel physical memory manager APIs

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2023 by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====================================================================
 Constants and macros
 ======================================================================*/

/* physical memory API return error code */
#define QURT_PMEM_SUCCESS               0
#define QURT_PMEM_NO_PRIV               1
#define QURT_PMEM_RETRY                 2
#define QURT_PMEM_OVERLAP               3
#define QURT_PMEM_NOT_EXIST             4
#define QURT_PMEM_INIT_FAILURE          5
#define QURT_PMEM_OUTSTANDING_MAPPING   6
#define QURT_PMEM_GENERIC_FAILURE       7
#define QURT_PMEM_ENTRY_FOUND           8
#define QURT_PMEM_REACH_END             9
#define QURT_PMEM_UNCLAIMED             10
#define QURT_PMEM_ALREADY_CLAIMED       11

/*=====================================================================
 Functions
======================================================================*/

/**@ingroup func_qurt_pmem_acquire
  Acquire the ownership of a specific physical memory region.

  @note1hang The ownership will be the caller

  @param[in] ppage      Starting physical page number
  @param[in] pnum       Number of physical pages

  @return
  #QURT_PMEM_NO_PRIV -- Have no privilege to claim the ownership. \n
  #QURT_PMEM_OVERLAP -- The whole or part of the range has been owned \n
  #QURT_PMEM_SUCCESS -- Succeed to claim ownership.

  @dependencies
  None.
*/
int qurt_pmem_acquire(unsigned int ppage, unsigned int pnum);

/**@ingroup func_qurt_pmem_release
  Release the ownership of a specific physical memory region.

  @param[in] ppage      The start of physical page number
  @param[in] pnum       The numbers of physical pages

  @return
  #QURT_PMEM_NO_PRIV                -- Have no privilege to claim the ownership. \n
  #QURT_PMEM_NOT_EXIST              -- The physical memory range is not usable. \n
  #QURT_PMEM_OUTSTANDING_MAPPING    -- There is outstanding mapping in this range
  #QURT_PMEM_SUCCESS                -- Succeed to claim ownership.

  @dependencies
  None.
 */
int qurt_pmem_release(unsigned int ppage, unsigned int pnum);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_PMEM_MANAGER_H */
