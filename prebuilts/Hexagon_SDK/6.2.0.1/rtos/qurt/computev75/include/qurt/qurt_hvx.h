#ifndef QURT_HVX_H
#define QURT_HVX_H
/**
  @file qurt_hvx.h 
  @brief   Prototypes of QuRT HVX API.  

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2021-2022  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        TYPEDEFS
=============================================================================*/
/** @cond */

typedef enum {
    QURT_HVX_MODE_64B = 0,      /**< HVX mode of 64 bytes */
    QURT_HVX_MODE_128B = 1      /**< HVX mode of 128 bytes */
} qurt_hvx_mode_t;
/** @endcond */
/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @cond internal_only*/
/** @addtogroup hvx_macros
@{ */
#define QURT_HVX_HW_UNITS_2X128B_4X64B        0x00000204       /**< Bits 15 through 8 are for the number of 128B units.   */
                                                               /**< Bits 7 through 0 are for the number of 64B units.     */
#define QURT_HVX_HW_UNITS_4X128B_0X64B        0x00000400   
#define QURT_HVX_HW_UNITS_6X128B_0X64B        0x00000600   

/* HVX locking status */

#define QURT_HVX_UNLOCKED                     (0)              /* Has not locked HVX unit */
#define QURT_HVX_LOCKED                       (1)              /* Has locked HVX unit */
#define QURT_HVX_ERROR                        (-1)             /* Error, no HVX support */

/* Input value for HVX reservation */

#define QURT_HVX_RESERVE_ALL                  (4)              /* All the HVX units in terms of 64B_MODE are requested to be reserved */
#define QURT_HVX_RESERVE_ALL_AVAILABLE        (0xff)           /* All remaining unlocked HVX units in terms of 64B_MODE are requested to be reserved */

/* Return values for HVX reservation */

#define QURT_HVX_RESERVE_NOT_SUPPORTED        (-1)             /* There is no HVX hardware, or less units in the hardware than requested */
#define QURT_HVX_RESERVE_NOT_SUCCESSFUL       (-2)             /* Some HVX units are already locked/reserved by other PD, thus not enough units left for the reservation. */
#define QURT_HVX_RESERVE_ALREADY_MADE         (-3)             /* There is already a HVX reservation made. */
#define QURT_HVX_RESERVE_CANCEL_ERR           (-4)             /* The action of cancling the reservation fails because this protection domain has no reservation made before. */

// HVX set requests

#define QURT_HVX_64B                    0  /**< */
#define QURT_HVX_128B                   1  /**< */
#define QURT_HVX_NO_USE                 2  /**< */
#define QURT_HVX_RELEASE_CONTEXT        3  /**< */
#define QURT_HVX_IMMEDIATE_USE          4  /**< */

// HVX set masks

#define QURT_HVX_64B_PREFERRED          (1<<(QURT_HVX_64B  + 8))/**< */
#define QURT_HVX_128B_PREFERRED         (1<<(QURT_HVX_128B + 8))/**< */
#define QURT_HVX_64B_ACCEPTABLE         (1<<(QURT_HVX_64B  + 12))/**< */
#define QURT_HVX_128B_ACCEPTABLE        (1<<(QURT_HVX_128B + 12))/**< */

// HVX set return "result"

#define QURT_EOK                        0     /**< */
#define QURT_HVX_SET_ERROR              0xFF  /**< */

// hvx_mode_assigned for QURT_HVX_IMMEDIATE_USE 
#define QURT_HVX_64B_ASSIGNED          (1<<(QURT_HVX_64B  + 8)) /**< */
#define QURT_HVX_128B_ASSIGNED         (1<<(QURT_HVX_128B + 8)) /**< */

// Sizes of HVX dump buffer

#define   QURT_HVX_V65_64B_VSIZE           2084U      /**<  64 x 32 +  8 x 4 + 4 (version). */
#define   QURT_HVX_V65_128B_VSIZE          4164U      /**<  128 x 32 + 16 x 4 + 4 (version). */
#define   QURT_HVX_V66_128B_VSIZE          4420U      /**<  128 x (32 +2) + 16 x 4 + 4 (version). */
#define   QURT_HVX_V68_128B_VSIZE          4164U      /**<  128 x 32 + 16 x 4 + 4 (version). */
#define   QURT_HVX_V79_128B_VSIZE          4740U      /**<  128 x (32+4+1) + 4 (version). */
#define   QURT_HVX_VREG_BUF_SIZE           QURT_HVX_V79_128B_VSIZE /**< */

// HVX dump versions

#define QURT_HVX_DUMP_V65_64B           1U  /**< */
#define QURT_HVX_DUMP_V65_128B          2U  /**< */
#define QURT_HVX_DUMP_V66_128B          3U  /**< */
#define QURT_HVX_DUMP_V68_128B          4U  /**< */
#define QURT_HVX_DUMP_V79_128B          5U  /**< */
/** @} */ /* end_addtogroup hvx_macros */
/** @endcond */
/** @cond */
// Qurt data struct for hvx_set input
typedef struct qurt_hvx_set_struct_ {          
    unsigned char set_req;  // LSB
    struct {
        unsigned char preferred_mask:4;
        unsigned char acceptable_mask:4;
    };
    unsigned short resvd;   // MSB
} qurt_hvx_set_struct_t;  // 4 bytes


// Qurt data struct for hvx_set return
typedef struct qurt_hvx_set_return_str_ {          
    unsigned char result;  // LSB
    unsigned char hvx_mode_assigned;
    unsigned short resvd;   // MSB
} qurt_hvx_set_return_struct_t;  // 4 bytes
/** @endcond */


/*=============================================================================
                        FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_hvx_lock
  Locks one HVX unit specified by the HVX mode.
  
  @note1hang Input variable can be 128B_MODE or 64B_MODE. If an HVX unit in this mode 
             is available, this function locks the unit and returns right away.
             If the current HVX mode is different from the requested mode, the current 
             thread is blocked. When all HVX units become idle, QuRT changes 
             the mode, locks the HVX unit, and returns.

            Starting from Q6v65 with HVX context switch support, qurt_hvx_lock() is 
            mapped as qurt_hvx_set(64_BYTE or 128_BYTE).
  
  @datatypes
  #qurt_mode_t
  
  @param[in]  lock_mode #QURT_HVX_MODE_64B or #QURT_HVX_MODE_128B.

  @return
  #QURT_EOK -- Success \n
  Other value -- Failure

  @dependencies
  None.
  
 */
int qurt_hvx_lock(qurt_hvx_mode_t lock_mode);

/**@ingroup func_qurt_hvx_unlock
  Unlocks the HVX unit held by this software thread.
  
  @note1hang  Starting from Q6v65 with HVX context switch support, qurt_hvx_unlock()
              maps as qurt_hvx_set(QURT_HVX_RELEASE_CONTEXT).
  
  @return
  #QURT_EOK -- Successful return \n
  Other values -- Failure

  @dependencies
  None.
  
 */
int qurt_hvx_unlock(void);

/**@ingroup func_qurt_hvx_try_lock
  Tries to lock one HVX unit specified by the HVX mode.
  
  @note1hang Input variable can be 128B_MODE or 64B_MODE. If an HVX unit in this mode 
             is available, this function locks the unit and returns #QURT_EOK; Otherwise,
             the function returns a failure, but does not block the current software 
             thread to wait for the HVX unit.
            Starting from Q6v65 with HVX context switch support, qurt_hvx_try_lock()
             maps to qurt_hvx_set(FOR_IMMEDIATE_USE| preferred_mask | acceptable_mask);
  
  @datatypes
  #qurt_mode_t

  @return
  #QURT_EOK -- Successful return \n
  Other values -- Failure

  @dependencies
  None.
  
 */
int qurt_hvx_try_lock(qurt_hvx_mode_t lock_mode);

/**@ingroup func_qurt_hvx_get_mode
  Gets the current HVX mode configured by QuRT.
  
  @note1hang Returns #QURT_HVX_MODE_128B or #QURT_HVX_MODE_64B, based on 
             the current HVX configuration.
  
  @param[out] 
  None.

  @return
  #QURT_HVX_MODE_128B \n
  #QURT_HVX_MODE_64B \n
  -1 -- Not available.

  @dependencies
  None.
 */
int qurt_hvx_get_mode(void);


/**@ingroup func_qurt_hvx_get_units
  Gets the HVX hardware configuration that the chipset supports.
  
  @note1hang The function returns the HVX hardware configuration supported by the chipset.
  
  @return
  Bitmask of the units: 1X64, 2X64, 4X64, 1X128, 2X128, and so on.\n
  - QURT_HVX_HW_UNITS_2X126B_4X64B -- V60, V62, or V65 HVX \n
  - QURT_HVX_HW_UNITS_4X128B_0X64B -- V66 CDSP or newer \n
  - 0 --  not available

  @dependencies
  None.
  
 */
int qurt_hvx_get_units(void);


/**@ingroup func_qurt_hvx_reserve
  Reserves HVX units in terms of 64-byte mode for the protection domain (PD) of the caller.
  
  @note1hang Only one HVX reservation in the system is supported.
             If one HVX unit is already locked by the application in the same PD, the unit is 
             added to the returned count as one reserved unit for the PD.
            Starting from Q6v65 with HVX context switch support, qurt_hvx_reserve()
            only does basic sanity checks on HVX units.
  
  @datatypes
  None.

  @param[in]  num_units  Number of HVX units in terms of 64B_MODE to reserve for the PD.
                         QURT_HVX_RESERVE_ALL to reserve all the HVX units.
                         QURT_HVX_RESERVE_ALL_AVAILABLE to reserve the remaining unlocked units.

  @return
    Number of units successfully reserved, including the units already locked in the same PD. \n
    #QURT_HVX_RESERVE_NOT_SUPPORTED \n     
    #QURT_HVX_RESERVE_NOT_SUCCESSFUL \n    
  #QURT_HVX_RESERVE_ALREADY_MADE    


  @dependencies
  None.
  
 */
int qurt_hvx_reserve(int num_units);


/**@ingroup func_qurt_hvx_cancel_reserve
  Cancels the HVX reservation in the protection domain (PD) of the caller.
  
  @note1hang Only one HVX reservation in the system is supported.
  
  @return
    0 -- Success \n
    #QURT_HVX_RESERVE_CANCEL_ERR -- Failure      

  @dependencies
  None.
  
 */
int qurt_hvx_cancel_reserve(void);


/**@ingroup func_qurt_hvx_get_lock_val
  Gets the HVX locking status value of the thread of the caller. 
  
  @note1hang Returns the status of whether the thread of the caller already locks a HVX unit or not.
  
  @datatypes
  None.

  @return
    #QURT_HVX_UNLOCKED \n  
    #QURT_HVX_LOCKED \n   
    #QURT_HVX_ERROR    

  @dependencies
  None.
 */
int qurt_hvx_get_lock_val(void);

/** @cond internal_only*/
/**@ingroup func_qurt_hvx_set
  Sets the HVX configuration for the software thread of the caller. 
  
  @datatypes
  None.

  @param[in] input_arg Composed of set_request | hvx_preferred_mode_mask 
                       | hvx_acceptable_mode_mask where set_request can be set to: \n
                       - #QURT_HVX_64B  \n         
                       - #QURT_HVX_128B  \n       
                       - #QURT_HVX_NO_USE  \n    
                       - #QURT_HVX_RELEASE_CONTEXT \n
                       - #QURT_HVX_IMMEDIATE_USE \n
                       When set_request is QURT_HVX_IMMEDIATE_USE,  
    hvx_preferred_mode_mask can be set to: \n
                       - #QURT_HVX_64B_PREFERRED \n    
                       - #QURT_HVX_128B_PREFERRED   
                       When set_request is QURT_HVX_IMMEDIATE_USE,  
    hvx_acceptable_mode_mask can be set to: \n
                       - #QURT_HVX_64B_ACCEPTABLE  \n
                       - #QURT_HVX_128B_ACCEPTABLE @tablebulletend

  @return 
     Result of the HVX setting in the least significant 8 bits of the returned data. \n
  #QURT_EOK -- 0  \n
  #QURT_HVX_SET_ERROR -- 0xFF \n     
  When #QURT_HVX_IMMEDIATE_USE has a result of #QURT_EOK, 
  bit 8 to bit 15 of the returned data contain hvx_mode_assigned:\n
  - #QURT_HVX_64B_ASSIGNED      \n
  - #QURT_HVX_128B_ASSIGNED   

  @dependencies
  None.
 */
unsigned int qurt_hvx_set(unsigned int input_arg);


/**@ingroup func_qurt_system_hvx_regs_get_maxsize
  Returns the maximum buffer size for saving HVX registers.
  
  @datatypes
  None.

  @return
  0 -- No HVX supported in the target. \n
  #QURT_HVX_VREG_BUF_SIZE -- Maximum buffer size for saving HVX registers.

  @dependencies
  None.
 */
unsigned int qurt_system_hvx_regs_get_maxsize(void);


/**@ingroup func_qurt_system_hvx_regs_get_size
  Returns the buffer size for saving HVX registers for a specified thread.
  
  @param[in]  thread_id    Thread ID of the target thread.

  @return
  0 -- No HVX assgined to the thread. \n
    size -- Size of the buffer in bytes for saving HVX registers for the specified thread: \n 
  - #QURT_HVX_V65_64B_VSIZE  -- 64 x 32 +  8 x 4 + 4 (version) \n
  - #QURT_HVX_V65_128B_VSIZE -- 128 x 32 + 16 x 4 + 4 (version) \n
  - #QURT_HVX_V66_128B_VSIZE -- 128 x (32 +2) + 16 x 4 + 4 (version) \n
  - #QURT_HVX_V68_128B_VSIZE -- 128 x 32 + 16 x 4 + 4 (version) \n
  - #QURT_HVX_V79_128B_VSIZE -- 128 x (32+4+1) + 4 (version)


  @dependencies
  None.
  
 */
unsigned int qurt_system_hvx_regs_get_size(unsigned int thread_id);



/**@ingroup func_qurt_system_hvx_regs_get
  Saves the HVX registers into the specified buffer.
  Returns the size of the data saved into the buffer.
  After calling this function for the first time on a specified thread_id, the QuRT kernel removes the internal HVX saving buffer 
  from the specified thread. When calling the function on the same thread_id for the second time, this function returns 0.
  
  @param[in] thread_id    Thread ID of the target thread.
  @param[in] pBuf         Pointer to the buffer for HVX register saving.
                          The first four bytes of the buffer are for saving the HVX version. HVX registers are saved from 
                          the fifth byte of the buffer. The address of the fifth byte should be 256 bytes aligned. 
                          For example, a buffer can be declared at first as: \n
                          unsigned char vbuf[QURT_HVX_VREG_BUF_SIZE+256];\n
                          unsigned char *pBuf; \n
                          then align the buffer pointer to: \n
                          pBuf = vbuf; \n
                    pBuf += (256 - 4 - (unsigned)pBuf%256);
  @param[in] size         Size of the buffer provided, which is pointed by *pBuf. The buffer size should not be smaller than that 
                          returned from qurt_system_hvx_regs_get_size(), and pBuf should be aligned as described above.
  @param[out] pBuf        Buffer returned with the saved HVx registers (unsigned char hvx_regs[];), which are saved from the fith 
                          byte of the buffer, and the HVX version (unsigned int hvx_version;), which in the first four bytes 
                          contain one of the HVX dump versions:\n
                          - #QURT_HVX_DUMP_V65_64B \n   
                          - #QURT_HVX_DUMP_V65_128B \n   
                          - #QURT_HVX_DUMP_V66_128B  \n  
                          - #QURT_HVX_DUMP_V68_128B  \n  
                          - #QURT_HVX_DUMP_V79_128B  \n  
                           @tablebulletend

  @return
    Total bytes of the data saved in the provided buffer. \n
  0  -- No HVX assigned to the thread \n
  #QURT_HVX_V65_64B_VSIZE   --  64 x 32 +  8 x 4 + 4 (version) \n
  #QURT_HVX_V65_128B_VSIZE  -- 128 x 32 + 16 x 4 + 4 (version) \n
  #QURT_HVX_V66_128B_VSIZE  -- 128 x (32 +2) + 16 x 4 + 4 (version) \n
  #QURT_HVX_V68_128B_VSIZE  -- 128 x 32 + 16 x 4 + 4 (version)  \n
  #QURT_HVX_V79_128B_VSIZE  -- 128 x (32+4+1) + 4 (version)

  @dependencies
  None.
 */
unsigned int qurt_system_hvx_regs_get(unsigned int thread_id, void *pBuf, size_t size);
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_HVX_H */

