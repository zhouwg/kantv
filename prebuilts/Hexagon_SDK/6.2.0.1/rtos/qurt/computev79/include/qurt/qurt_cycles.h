
#ifndef QURT_CYCLES_H
#define QURT_CYCLES_H 1
/**
  @file qurt_cycles.h
  Prototypes of kernel pcycle API functions.      

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/
 
#ifdef __cplusplus
extern "C" {
#endif

	/*=====================================================================
	 Functions
	======================================================================*/
	 
/*======================================================================*/

/**@ingroup func_qurt_profile_reset_idle_pcycles
  @xreflabel{hdr:qurt_profile_reset_idle_pcycles}
  Sets the per-hardware-thread idle cycle counts to zero. 

  @return 
  None. 
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_profile_reset_idle_pcycles (void);
	 
/*======================================================================*/
/**@ingroup func_qurt_profile_get_thread_pcycles
  @xreflabel{hdr:qurt_profile_get_thread_pcycles}
  Gets the count of the running processor cycles for the current thread.\n
  Returns the current running processor cycle count for the current QuRT thread.

  @note1hang  Profiling shall be enabled first to start the cycle counting. 
              The cycles are accumulated once the profiling is enabled and 
              resets on #qurt_profile_reset_threadid_pcycles

  @return 
  Integer -- Running processor cycle count for current thread.
		 
  @dependencies 
  None.
*/
/* ======================================================================*/
unsigned long long int qurt_profile_get_thread_pcycles(void);

	
/*======================================================================*/
/**@ingroup func_qurt_get_core_pcycles
  @xreflabel{hdr:qurt_get_core_pcycles}
  Gets the count of core processor cycles executed.\n
  Returns the current number of running processor cycles executed since the Hexagon
  processor was last reset.

  This value is based on the hardware core clock, which varies in speed according to the
  processor clock frequency.

  @note1hang Because the hardware core clock stops running when the processor shuts
             down (due to all of the hardware threads being idle), treat the cycle values returned
             by this operation as relative rather than absolute.

  @note1cont Thread cycle counts are valid only in the V4 Hexagon processor version.

  @return 
  Integer -- Current count of core processor cycles.
		 
  @dependencies
  None.
*/
/* ======================================================================*/
unsigned long long int qurt_get_core_pcycles(void);

/*======================================================================*/
/**@ingroup func_qurt_profile_get_idle_pcycles

  @deprecated use #qurt_profile_get_idle_pcycles2 instead

  Gets the current idle processor cycle counts for a maximum of 6 hardware threads. Use
  #qurt_profile_get_idle_pcycles2 for reading pcycles without limitation on maximum hardware threads. 

  This operation accepts a pointer to a user-defined array, and writes to the array the current
  idle cycle count for each hardware thread.

  Each count value represents the number of processor cycles that have elapsed on the
  corresponding hardware thread while that thread has been in Wait mode.\n


  @note1hang This operation does not return the idle cycles that occur when the Hexagon
             processor shuts down (due to all of the hardware threads being idle). 
             Idle cycle counts gets accumulated irrespective of profiling is enabled or not, 
	           and resets on #qurt_profile_reset_idle_pcycles
	
  @param[out] pcycles  User array where the function stores the current idle cycle count values.
                        Array size should be a minimum of the number of hardware threads intended. 

  @return
  None.
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_profile_get_idle_pcycles (unsigned long long *pcycles);

/*======================================================================*/
/**@ingroup func_qurt_profile_get_idle_pcycles2
  Gets the current idle processor cycle counts for maximum available hardware threads.

  This operation accepts a pointer to a user-defined array with length in bytes, and writes 
  to the array the current idle cycle count for each hardware thread.

  Each count value represents the number of processor cycles that have elapsed on the
  corresponding hardware thread while that thread has been in Wait mode.\n

  @note1hang This operation does not return the idle cycles that occur when the Hexagon
             processor shuts down (due to all of the hardware threads being idle). 
             Idle cycle counts gets accumulated irrespective of profiling enable status, and 
             resets on #qurt_profile_reset_idle_pcycles
	
  @param[out] pcycles  User array where the function stores the current idle cycle count values. 
                        Array size should be equivalent to the number of hardware threads intended. 
                        Call #qurt_sysenv_get_max_hw_threads to determine the array size required.
  
  @param[in] length_in_bytes Length of pcycles array in bytes. If the array size is smaller
                              than the required for the maximum available hardware threads, 
                              it returns error code. 

  @return
  #QURT_EOK -- Successful operation. Stored all the data to the destination array
  #QURT_EFAILED -- Operation failed due to smaller #pcycles array
		 
  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_profile_get_idle_pcycles2 (unsigned long long *pcycles, unsigned int length_in_bytes);

/*======================================================================*/
/**@ingroup func_qurt_profile_get_threadid_pcycles
  
  @deprecated use #qurt_profile_get_threadid_pcycles2 instead
  
  Gets the current per-hardware-thread running cycle counts for the specified QuRT
  thread for a maximum of 6 hardware threads.

  Each count value represents the number of processor cycles that have elapsed on the
  corresponding hardware thread while that thread has been scheduled for the specified
  QuRT thread.

  @note1hang  Profiling shall be enabled first to start the cycle counting. 
              The cycles are accumulated once the profiling is enabled and 
              resets on #qurt_profile_reset_threadid_pcycles

  @param[in]   thread_id  Valid thread identifier.
  @param[out]  pcycles    Pointer to a user array where the function stores the current running 
                          cycle count values. Array size should be a minimum of the number of
                          hardware threads intended. 
	
  @return 				
  None. 
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_profile_get_threadid_pcycles (int thread_id, unsigned long long  *pcycles);

/*======================================================================*/
/**@ingroup func_qurt_profile_get_threadid_pcycles2
    
  Gets the current per-hardware-thread running cycle counts for the specified QuRT
  thread for maximum available hardware threads.

  Each count value represents the number of processor cycles that have elapsed on the
  corresponding hardware thread while that thread has been scheduled for the specified
  QuRT thread.

  @note1hang  Profiling shall be enabled first to start the cycle counting. 
              The cycles are accumulated once the profiling is enabled and 
              resets on #qurt_profile_reset_threadid_pcycles

  @param[in]  thread_id  Thread identifier.
  @param[out] pcycles    Pointer to a user array where the function stores the current running 
                          cycle count values. Array size should be equivalent to the number of
                          hardware threads intended. 
                          Call #qurt_sysenv_get_max_hw_threads to determine the array size required.
  @param[in]  length_in_bytes Length of pcycles array in bytes. If the array size is smaller
                              than the required for the maximum available hardware threads, it 
                              returns error code. 
  
  @return
  #QURT_EOK -- Successful operation. Stored all the data to the destination array
  #QURT_EFAILED -- Operation failed due to smaller #pcycles array
  #QURT_ENOTHREAD -- Operation failed due to invalid #thread_id
		 
  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_profile_get_threadid_pcycles2 (int thread_id, unsigned long long  *pcycles, unsigned int length_in_bytes);


/*======================================================================*/
/**@ingroup func_qurt_profile_reset_threadid_pcycles
  @xreflabel{hdr:qurt_profile_reset_threadid_pcycles}
  Sets the per-hardware-thread running cycle counts to zero for the specified QuRT thread.

  @param[in]  thread_id Thread identifier.
	
  @return 
  None. 
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_profile_reset_threadid_pcycles (int thread_id);

/*======================================================================*/
/**@ingroup func_qurt_profile_enable
  @xreflabel{hdr:qurt_profile_enable}
  Enables profiling.\n
  Enables or disables cycle counting of the running and idle processor cycles.
  Profiling is disabled by default. \n

  @note1hang Enabling profiling does not automatically reset the cycle counts -- this must be
             done explicitly by calling the reset operations before starting cycle counting.
             Cycle counting starts from the instant of it was enabled using this API, and  
             halts on profiling disable.
	
  @param[in] enable  Profiling. Values: \n
                     - 0 -- Disable profiling \n
                     - 1 -- Enable profiling @tablebulletend
	
  @return 
  None. 
		 
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_profile_enable (int enable);

/*======================================================================*/
/**@ingroup func_qurt_get_hthread_pcycles
  @xreflabel{hdr:qurt_get_hthread_pcycles}
  Reads the GCYCLE_nT register to allow performance measurement when N threads are in run mode.\n

  @note1hang Returns 0 when architecture is earlier than v67 or for invalid HW thread id.
  
  @param[in] n Threads in run mode. Valid values are 1 through <maximum HW threads>.
                     
  
  @return 
  Value read from GCYCLE_nT register. This value indicates the total number of pcycles that got executed
  from reset to current point of execution when n threads are in run mode
     
  @dependencies
  PMU must be enabled.
*/
/* ======================================================================*/
unsigned int qurt_get_hthread_pcycles(int n);

/*======================================================================*/
/**@ingroup func_qurt_get_hthread_commits
  @xreflabel{hdr:qurt_get_hthread_commits}
  Reads the GCOMMIT_nT register to allow performance measurement when N threads are in run mode.\n

  @note1hang Returns 0 when architecture is earlier than v67 or for invalid HW thread id.
  
  @param[in] n Threads in run mode. Valid values: 1 through <maximum HW threads>.
  
  @return 
  Value read from the GCOMMIT_nT register. This value indicates the total number of packets 
  committed from reset to current point of execution when n threads are in run mode.
     
  @dependencies
  PMU must be enabled.
*/
/* ======================================================================*/
unsigned int qurt_get_hthread_commits(int n);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif

