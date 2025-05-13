#ifndef QURT_L2CFG_H
#define QURT_L2CFG_H
/**
  @file qurt_l2cfg.h 
  @brief QuRT APIs for L2 configuration and system configuration

EXTERNAL FUNCTIONS
   qurt_l2cfg_set 
   qurt_l2cfg_get 
   qurt_system_config_get

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2019-2021 by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/

/* Definition for system configuration */
/** @addtogroup l2cfg_macros
@{ */
#define QURT_CORE_CFG_HMX_INT8_SPATIAL  0x78   /**< HMX fixed-point spatial size */
#define QURT_CORE_CFG_HMX_INT8_DEPTH    0x7C   /**< HMX fixed-point output depth */
/** @} */ /* end_addtogroup l2cfg_macros */
/*=============================================================================
                        FUNCTIONS
=============================================================================*/
/**@ingroup func_qurt_l2cfg_set
  Sets the value of a L2 configuration register. A register can be set *IFF* its 
  initial value is configured.
   
  @param[in] offset Offset of L2 configuration register; must be multiple of 4.
  @param[in] value  Value to set the register to. 
   
  @return 
  #QURT_EOK -- Success. \n 
  #QURT_EFAILED -- Internal mapping that covers L2CFG register file absent; likely 
                    a configuration problem. \n
  #QURT_EINVALID -- Argument error. \n
  #QURT_ENOTALLOWED -- Setting this register is prohibited.
   
  @dependencies 
  None.  
 */
int qurt_l2cfg_set (unsigned short offset, unsigned int value);

/**@ingroup func_qurt_l2cfg_get 
  Gets the value of a L2 configuration register. 
   
  @param[in]  offset Offset of L2 configuration register; must be multiple of 4.
  @param[out] value  Pointer to value of the register. 
   
  @return 
  #QURT_EOK -- Success. \n 
  #QURT_EFAILED -- Internal mapping that covers L2CFG register file absent;  
                   likely a configuration problem. \n 
  #QURT_EINVALID -- Argument error. 
   
  @dependencies 
  None. 
  
 */
int qurt_l2cfg_get (unsigned short offset, unsigned int * value);


/**@ingroup func_qurt_system_config_get
  Gets the system configuration information.

  @param[in] index Index to system configuration. Values:\n
                   - #QURT_CORE_CFG_HMX_INT8_SPATIAL \n
                   - #QURT_CORE_CFG_HMX_INT8_DEPTH @tablebulletend

  @param[out] data   Pointer to a word for returned data.

  @return
  #QURT_EOK -- Get the configuration data successful. \n
  Other values -- Failure (no such configuration available).

  @dependencies
  None.
  
 */
int qurt_system_config_get(int index, unsigned int *data);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_L2CFG_H */
