#ifndef QURT_ECC_H
#define QURT_ECC_H


/*=====================================================================
 
  @file  qurt_ecc.h
  @brief  Prototypes of QuRT memory ECC API functions      

 Copyright (c) 2018, 2020-2021 by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.

 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        TYPEDEFS
=============================================================================*/
/** @addtogroup exception_handling_types
@{ */
// ECC memory definition
typedef enum {
    QURT_ECC_MEM_L1_ICACHE = 0, /**< ECC memory L1 ICache. */
    QURT_ECC_MEM_L1_DCACHE = 1, /**< ECC memory L1 DCache.*/
    QURT_ECC_MEM_L2_CACHE  = 2, /**< ECC memory L2 Cache.*/
    QURT_ECC_MEM_VTCM      = 3  /**< ECC memory VTCM.*/
} qurt_ecc_memory_t;
/** @} */ /* end_addtogroup exception_handling_types */

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @addtogroup exception_handling_macros
@{ */

#define   QURT_ECC_ERR_DETECTED_STATUS        0 /**< ECC error detected. */
#define   QURT_ECC_ERR_TYPE                   1 /**< ECC error type.*/
// ECC status type

#define  QURT_ECC_CORRECTABLE_COUNT           (1<<0) /**< ECC correctable count.*/
#define  QURT_ECC_UNCORRECTABLE_COUNT         (1<<1) /**< ECC uncorrectable count.*/
#define  QURT_ECC_REGION_LOGGING              (1<<2) /**< ECC region logging.*/
// ECC enable/disable definition

#define QURT_ECC_PROTECTION_DISABLE  (0<<0)    /**< Bit 0. */
#define QURT_ECC_PROTECTION_ENABLE   (1<<0)    /**< Bit 0. */
/** @} */ /* end_addtogroup exception_handling_macros */
/*=============================================================================
                        FUNCTIONS
=============================================================================*/


/**@ingroup func_qurt_ecc_enable
  Enables or disables ECC protection on a specified memory.
  
  @datatypes
  #qurt_ecc_memory_t
  
  @param[in]  memory Set to one of the following values:
                     - #QURT_ECC_MEM_L1_ICACHE
                     - #QURT_ECC_MEM_L1_DCACHE
                     - #QURT_ECC_MEM_L2_CACHE
                     - #QURT_ECC_MEM_VTCM   @tablebulletend

  @param[in]  enable Set to one of the following values:
                     - #QURT_ECC_PROTECTION_ENABLE
                     - #QURT_ECC_PROTECTION_DISABLE  @tablebulletend

  @return
  - #QURT_EOK --   ECC enabling or disabling setup is performed successfully
  - Others  --    Failure

  @dependencies
  None.
 */
int qurt_ecc_enable( qurt_ecc_memory_t memory, unsigned int enable );


/**@ingroup func_qurt_ecc_get_error_status
  Gets ECC error status for a specified memory.
  
  @datatypes
  #qurt_ecc_memory_t
  
  @param[in]  memory  Set to one of the following:
                      - #QURT_ECC_MEM_L1_ICACHE
                      - #QURT_ECC_MEM_L1_DCACHE
                      - #QURT_ECC_MEM_L2_CACHE
                      - #QURT_ECC_MEM_VTCM    @tablebulletend

  @param[in]  type  Set to one of the following:
                     - #QURT_ECC_ERR_DETECTED_STATUS
                     - #QURT_ECC_ERR_TYPE  @tablebulletend

  @return
  Returns the following when the type is #QURT_ECC_ERR_DETECTED_STATUS:
       - 0 -- No error detected \n
       - 1 -- At least one error detected \n
  Returns the following when the type is #QURT_ECC_ERR_TYPE: \n
       - 0 through 1 -- Correctable error \n
       - 2 --   Uncorrectable error

  @dependencies
  None.
 */
int qurt_ecc_get_error_status( qurt_ecc_memory_t memory, unsigned int type );


/**@ingroup func_qurt_ecc_get_error_count
  Gets the ECC error count for a specified memory.
  
  @datatypes
  #qurt_ecc_memory_t
  
  @param[in]  memory  Set to one of the following values:\n
                      - #QURT_ECC_MEM_L1_ICACHE \n
                      - #QURT_ECC_MEM_L1_DCACHE \n
                      - #QURT_ECC_MEM_L2_CACHE \n
                      - #QURT_ECC_MEM_VTCM    @tablebulletend

  @param[in]  type  Set to one of the following values: \n
                     - #QURT_ECC_CORRECTABLE_COUNT \n
                     - #QURT_ECC_UNCORRECTABLE_COUNT  @tablebulletend

  @return
  Error count for the specified error type.

  @dependencies
  None.
 */
int qurt_ecc_get_error_count( qurt_ecc_memory_t memory, unsigned int type );


/**@ingroup func_qurt_ecc_clear_error_count
  Clears ECC error count or region logging for a specified memory.
  
  @datatypes
  #qurt_ecc_memory_t
  
  @param[in]  memory Set to one of the following values: \n
                     - #QURT_ECC_MEM_L1_ICACHE \n
                     - #QURT_ECC_MEM_L1_DCACHE \n
                     - #QURT_ECC_MEM_L2_CACHE \n
                     - #QURT_ECC_MEM_VTCM    @tablebulletend

  @param[in]  type Set to one or multiple OR'ed of the following values: \n
                  - #QURT_ECC_CORRECTABLE_COUNT  \n
                  - #QURT_ECC_UNCORRECTABLE_COUNT \n
                  - #QURT_ECC_REGION_LOGGING  @tablebulletend
     
  @return
  #QURT_EOK -- Error count successfully cleared \n
  Others --   Failure at clearing the error count

  @dependencies
  None.
 */
int qurt_ecc_clear_error_count( qurt_ecc_memory_t memory, unsigned int type );

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ECC_H */

