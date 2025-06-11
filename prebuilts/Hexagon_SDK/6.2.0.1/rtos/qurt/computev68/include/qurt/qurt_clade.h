#ifndef QURT_CLADE_H
#define QURT_CLADE_H
/**
  @file qurt_clade.h 
  @brief  Prototypes of Cache Line Accelerated Decompression Engine (CLADE) API.
  CLADE is a cache line level memory compression system that is used to
  decrease DRAM usage.

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

 Copyright (c) 2019-2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                                    FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_clade2_get
  Reads the value of the clade2 register.
 
  @param[in] offset Offset from the clade2 cfg base.
  @param[out] *value  Pointer to the register value read from the offset.
 
  @return
  #QURT_EOK - Successfully read the value from the register at offset \n
  #QURT_EINVALID - Offset passed is incorrect
   
  @dependencies
  None.
 */
int qurt_clade2_get(unsigned short offset, unsigned int *value);
 
/**@ingroup func_qurt_clade2_set
  Sets the PMU register; only PMU_SEL register can be set.
  
  @param[in] offset Offset from the QURTK_clade2_cfg_base.          
  @param[in] value  Value to set at offset.  
 
  @return
  #QURT_EOK -- Successfully set the value at offset. \n
  #QURT_ENOTALLOWED -- Set operation performed at an offset other than CLADE2_PMU_SELECTION_REG.

  @dependencies
  None.
 */
int qurt_clade2_set(unsigned short offset, unsigned int value);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_CLADE_H */
