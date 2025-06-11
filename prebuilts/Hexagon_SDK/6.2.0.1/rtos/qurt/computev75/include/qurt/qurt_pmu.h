#ifndef QURT_PMU_H
#define QURT_PMU_H
/**
  @file qurt_pmu.h 
  Prototypes of pipe interface API.  
	 A pipe or message queue blocks when too full (send) or empty (receive).
	 Unless using a nonblocking option, all datagrams are 64 bits.

  EXTERNAL FUNCTIONS
   None.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

  Copyright (c) 2021 Qualcomm Technologies, Inc.
  All rights reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
												FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_pmu_set
  Sets the value of the specified PMU register.

  @note1hang Setting PMUEVTCFG automatically clears the PMU registers PMUCNT0
             through PMUCNT3.
 
  @param[in] reg_id   PMU register. Values: 
            - #QURT_PMUCNT0
            - #QURT_PMUCNT1    
            - #QURT_PMUCNT2    
            - #QURT_PMUCNT3    
            - #QURT_PMUCFG     
            - #QURT_PMUEVTCFG
            - #QURT_PMUCNT4    
            - #QURT_PMUCNT5    
            - #QURT_PMUCNT6    
            - #QURT_PMUCNT7    
            - #QURT_PMUEVTCFG1   @tablebulletend 

  @param[in] reg_value  Register value.
 
  @return
  None.
   
  @dependencies
  None.
 */
void qurt_pmu_set (int reg_id, unsigned int reg_value);
 
/**@ingroup func_qurt_pmu_get
  Gets the PMU register.\n
  Returns the current value of the specified PMU register.

  @param[in] reg_id   PMU register. Values: 			   
            - #QURT_PMUCNT0
            - #QURT_PMUCNT1    
            - #QURT_PMUCNT2    
            - #QURT_PMUCNT3    
            - #QURT_PMUCFG     
            - #QURT_PMUEVTCFG
            - #QURT_PMUCNT4    
            - #QURT_PMUCNT5    
            - #QURT_PMUCNT6    
            - #QURT_PMUCNT7    
            - #QURT_PMUEVTCFG1  @tablebulletend           
 
  @return
   Integer -- Current value of the specified PMU register.

  @dependencies
  None.
 */
unsigned int  qurt_pmu_get (int reg_id);
 
/**@ingroup func_qurt_pmu_enable
  Enables or disables the Hexagon processor PMU.
  Profiling is disabled by default. 

  @note1hang Enabling profiling does not automatically reset the count registers -- this must
            be done explicitly before starting event counting.
 
  @param[in] enable Performance monitor. Values: \n
                    - 0 -- Disable performance monitor \n
                    - 1 -- Enable performance monitor @tablebulletend
 
  @return 
  None.

  @dependencies
  None.
 */
void qurt_pmu_enable (int enable);

/**@ingroup func_qurt_pmu_get_pmucnt
  Reads PMU counters in a single trap.
 
  @param[out] buf   Pointer to a buffer to save values read from PMU counters.
                    buffer size should be at least 32 bytes to read all eight PMU counters.
 
  @return 
  #QURT_EOK    -- Successful read.\n
  #QURT_EFATAL -- Failure.

  @dependencies
  None.
 */
int qurt_pmu_get_pmucnt (void * buf);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_PMU_H */
