#ifndef QURT_HMX_H
#define QURT_HMX_H
/**
  @file qurt_hmx.h 
  @brief   Prototypes of Qurt HMX API.  

Copyright (c) 2019-2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        TYPEDEFS
=============================================================================*/


/** @addtogroup hmx_types
@{ */
/* HMX locking type */
#define  QURT_HMX_NON_SHARED_LOCK           0U /**< HMX locking type.*/
#define  QURT_HMX_SHARED_LOCK               1U /**< HMX locking type.*/

/* HMX unlocking type */
#define  QURT_HMX_NON_SHARED_UNLOCK         0U /**< HMX unlocking type.*/
#define  QURT_HMX_SHARED_UNLOCK             1U /**< HMX unlocking type.*/

/* HMX hardware context */
#define  QURT_HMX_UNIT_0                    0U /**< HMX hardware context #0 */
#define  QURT_HMX_UNIT_1                    1U /**< HMX hardware context #1 */
	/** @} */ /* end_addtogroup hmx_types */


/*=============================================================================
                        FUNCTIONS
=============================================================================*/


/**@ingroup func_qurt_hmx_lock2
  Locks a HMX unit with the specified locking type.

    #QURT_HMX_NON_SHARED_LOCK:
   - If a HMX unit is available, lock the unit and return success of #QURT_EOK.
   - If the HMX unit is already locked by another thread, the caller thread is suspended 
     until the HMX is available and gets locked by this function.
   - If there is no HMX hardware supported, returns #QURT_EVAL;

    #QURT_HMX_SHARED_LOCK:
   - If a HMX unit is available, enables HMX access for the caller thread, and returns 
     success of #QURT_EOK.
   - If the HMX is enabled on the caller thread, return #QURT_EFAILED.
   - If the HMX is locked by another thread in the same user process of the caller 
     thread with locking type of #QURT_HMX_SHARED_LOCK, enable HMX access for the caller 
     thread, and return success of #QURT_EOK.
   - If the HMX is locked by another thread in the same user process of the caller 
     thread with locking type of #QURT_HMX_NON_SHARED_LOCK, return #QURT_EFAILED.
   - If the HMX is locked by a thread from another user process different from the 
     user process of the caller thread, return #QURT_EFAILED.
   - If there is no HMX hardware supported, return #QURT_EVAL.

  @param[in]  type  Locking type.
    
  @return
  #QURT_EOK     -- HMX lock successful.\n
  #QURT_EFAILED -- Failure due to wrong locking condition.\n
  #QURT_EVAL    -- Failure because no HMX hardware is supported.

  @dependencies
  None.
  
 */
int qurt_hmx_lock2(unsigned int type);


/**@ingroup func_qurt_hmx_unlock2
  Unlocks a HMX unit with the unlocking type.

    #QURT_HMX_NON_SHARED_UNLOCK:
  - If there is a HMX unit locked by the caller thread, unlock the HMX unit and clear the 
    HMX accumulators (assuming a fixed point type). 
  - If there is no HMX unit locked by the caller thread, return #QURT_EFAILED. 
  - If there is no HMX hardware supported, return #QURT_EVAL.

  #QURT_HMX_SHARED_UNLOCK:
   - If the caller thread has locked HMX with type #QURT_HMX_SHARED_LOCK, disable the 
     HMX access on the caller thread, and return success of #QURT_EOK.
     Note: If the caller thread is the last thread that unlocks for #QURT_HMX_SHARED_LOCK 
           in its user process, the unlock function clears the HMX accumulators. 
   - If the caller thread has locked HMX with type #QURT_HMX_NON_SHARED_LOCK, return 
     failure of #QURT_EFAILED.
   - If the caller thread has not locked HMX, return failure of #QURT_EFAILED.
   - If there is no HMX hardware supported, returns #QURT_EVAL.

  @param[in]  type  Locking type.
    
  @return
  #QURT_EOK     -- HMX is unlocked successful. \n
  #QURT_EFAILED -- Failure due to wrong unlocking condition. \n
  #QURT_EVAL    -- Failure because no HMX hardware is supported.

  @dependencies
  None.
  
 */
int qurt_hmx_unlock2(unsigned int type);


/**@ingroup func_qurt_hmx_lock
  Locks a HMX unit.
  If a HMX unit is available, this function locks the unit and returns right away.
  If there is no HMX unit available, the caller is blocked until a HMX is available 
  and is locked by the function.

  @return
  #QURT_EOK -- HMX lock successful. \n
  #QURT_EFAILED -- Failure due to wrong locking condition. \n
  #QURT_EVAL    -- Failure because no HMX hardware is supported.

  @dependencies
  None.
 */
int qurt_hmx_lock(void);


/**@ingroup func_qurt_hmx_unlock
  Unlocks a HMX unit.
  If a HMX unit is locked by the caller thread, unlock the HMX unit and clear its 
  accumulators(assuming fixed point type). 
  If there is no HMX unit locked by the caller thread, return failure. 
  
  @return
  #QURT_EOK -- HMX unlock successful. \n
  #QURT_EFAILED -- Failure due to wrong unlocking condition. \n
  #QURT_EVAL    -- Failure because no HMX hardware is supported.

  @dependencies
  None.
 */
int qurt_hmx_unlock(void);


/**@ingroup func_qurt_hmx_try_lock
  Tries to lock a HMX unit.
  If a HMX unit is available, this function locks the unit and returns right away;
  if there is no HMX unit available, the function returns failure without blocking the caller.
  
  @return
  #QURT_EOK -- HMX lock successful \n
  #QURT_EFAILED -- Failure due to wrong locking condition.\n
  #QURT_EVAL    -- Failure because no HMX hardware is supported.

  @dependencies
  None.
 */
int qurt_hmx_try_lock(void);


/**@ingroup func_qurt_hmx_assign
  Assign a HMX unit to a target thread specified by its thread identifier. 
  The HMX unit (HMX hardware context) is specified by hmx_unit.
  The caller of this function is limited to the SRM process.
  If the requested hmx_unit is already assigned to another thread with QURT_HMX_NON_SHARED_LOCK, 
  kernel will detach it from the thread, and re-assign it to the target thread. 
  If the target thread has HVX enabled, it cannot have HMX enabled.  

  Locking type 
  #QURT_HMX_NON_SHARED_LOCK:
   - If the HMX unit is available, lock the HMX unit and return success of #QURT_EOK.
   - If the HMX unit is already enabled on the target thread, return #QURT_EOK.
   - If the HMX unit is already locked by another thread, detach the HMX from the thread.
     Re-assign the HMX unit to the target thread, and return #QURT_EOK.
     
  @param[in]  thread_id    Thread identifier
  @param[in]  type         Locking type  
                             #QURT_HMX_NON_SHARED_LOCK -- non-shared lock
  @param[in]  hmx_unit     HMX hardware context number  
                             #QURT_HMX_UNIT_0
                             #QURT_HMX_UNIT_1 
    
  @return
  #QURT_EOK       -- The HMX is assigned successfully. This includes the case that \n
                     the target thread already has HMX assigned. \n
  #QURT_EFAILED   -- Failure due to wrong assigning conditions. \n
  #QURT_EINVALID  -- Failure because no HMX hardware is supported.

  @dependencies
  None.
 */
int qurt_hmx_assign ( unsigned int thread_id, unsigned int type, unsigned int hmx_unit );


/**@ingroup func_qurt_hmx_release
  Release a HMX unit from a target thread specified by its thread identifier. 
  The HMX unit (HMX hardware context) is specified by hmx_unit.
  The caller of this function is limited to the SRM process.

  Qurt detaches the specified HMX unit from the target thread, and return success of 
  #QURT_EOK. If the HMX unit is already released from the target thread, return #QURT_EOK.
     
  @param[in]  thread_id    Thread identifier
  @param[in]  hmx_unit     HMX hardware context number  
                             #QURT_HMX_UNIT_0
                             #QURT_HMX_UNIT_1 
    
  @return
  #QURT_EOK       -- The HMX is released successfully. This includes the case that \n
                     the target thread already has the HMX released. \n
  #QURT_EFAILED   -- Failure due to wrong assigning condition. \n
  #QURT_EINVALID  -- Failure because no HMX hardware is supported.

  @dependencies
  None.
 */
int qurt_hmx_release ( unsigned int thread_id, unsigned int hmx_unit );



#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_HMX_H */

