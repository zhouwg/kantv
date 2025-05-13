#ifndef QURT_TRACE_H
#define QURT_TRACE_H
/**
  @file qurt_trace.h 
  @brief  Prototypes of system call tracing helpers API  

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2021-2023 by Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                            GLOBAL VARIABLES
=============================================================================*/
/** @cond internal_only */
/** @addtogroup etm_macros
@{ */
/* ETM trace types. */
#define QURT_ETM_TYPE_PC_ADDR                           (1U<<0) /**< PC address.*/
#define QURT_ETM_TYPE_MEMORY_ADDR                       (1U<<1) /**< Memory address. */
#define QURT_ETM_TYPE_TESTBUS                           (1U<<2) /**< Test bus. */
#define QURT_ETM_TYPE_CYCLE_ACCURATE                    (1U<<3) /**< Cycle accurate. */
#define QURT_ETM_TYPE_CYCLE_COARSE                      (1U<<4) /**< Cycle coarse. */
#define QURT_ETM_TYPE_PC_AND_MEMORY_ADDR                (QURT_ETM_TYPE_PC_ADDR|QURT_ETM_TYPE_MEMORY_ADDR) /**< PC and memory address. */
#define QURT_ETM_TYPE_PC_ADDR_AND_TESTBUS               (QURT_ETM_TYPE_PC_ADDR|QURT_ETM_TYPE_TESTBUS) /**< PC address and test bus. */
#define QURT_ETM_TYPE_MEMORY_ADDR_AND_TESTBUS           (QURT_ETM_TYPE_MEMORY_ADDR|QURT_ETM_TYPE_TESTBUS) /**< Memory address and test bus.*/
#define QURT_ETM_TYPE_PC_AND_MEMORY_ADDR_AND_TESTBUS    (QURT_ETM_TYPE_PC_ADDR|QURT_ETM_TYPE_MEMORY_ADDR|QURT_ETM_TYPE_TESTBUS) /**< PC, memory address, and test bus. */

/* ETM routes. */
#define QURT_ETM_ROUTE_TO_QDSS      0U /**< ETM route to QDSS. */
#define QURT_ETM_ROUTE_TO_Q6ETB     1U /**< ETM route to Q6ETB. */

/* ETM filters. */
#define QURT_ETM_TRACE_FILTER_ALL_DEFAULT   0U       /*< Filter all as default. */
#define QURT_ETM_TRACE_FILTER_HNUM0         (1U<<0)  /*< Filter HNUM0. */    
#define QURT_ETM_TRACE_FILTER_HNUM1         (1U<<1)  /*< Filter HNUM1. */     
#define QURT_ETM_TRACE_FILTER_HNUM2         (1U<<2)  /*< Filter HNUM2. */     
#define QURT_ETM_TRACE_FILTER_HNUM3         (1U<<3)  /*< Filter HNUM3. */  
#define QURT_ETM_TRACE_FILTER_HNUM4         (1U<<4)  /*< Filter HNUM4. */  
#define QURT_ETM_TRACE_FILTER_HNUM5         (1U<<5)  /*< Filter HNUM5. */  
#define QURT_ETM_TRACE_FILTER_HNUM6         (1U<<6)  /*< Filter HNUM6. */  
#define QURT_ETM_TRACE_FILTER_HNUM7         (1U<<7)  /*< Filter HNUM7. */  
#define QURT_ETM_TRACE_FILTER_HNUM8         (1U<<8)  /*< Filter HNUM8. */    
#define QURT_ETM_TRACE_FILTER_HNUM9         (1U<<9)  /*< Filter HNUM9. */     
#define QURT_ETM_TRACE_FILTER_HNUM10        (1U<<10) /*< Filter HNUM10. */     
#define QURT_ETM_TRACE_FILTER_HNUM11        (1U<<11) /*< Filter HNUM11. */
#define QURT_ETM_TRACE_FILTER_HNUM12        (1U<<12) /*< Filter HNUM12. */    
#define QURT_ETM_TRACE_FILTER_HNUM13        (1U<<13) /*< Filter HNUM13. */     
#define QURT_ETM_TRACE_FILTER_HNUM14        (1U<<14) /*< Filter HNUM14. */     
#define QURT_ETM_TRACE_FILTER_HNUM15        (1U<<15) /*< Filter HNUM15. */
#define QURT_ETM_TRACE_FILTER_ALL           QURT_ETM_TRACE_FILTER_ALL_DEFAULT

#define QURT_ETM_TRACE_FILTER_CLUSTER0      (1<<16)  /*< Filter trace cluster0 address. */  
#define QURT_ETM_TRACE_FILTER_CLUSTER1      (1<<17)  /*< Filter trace cluster1 address. */  
#define QURT_ETM_TRACE_FILTER_PC_RANGE      (1<<19)  /*< Filter PC address range. */  

/* ETM memory source - PC or data access */
#define QURT_ETM_SOURCE_PC                  0U  /**< ETM memory source of SAC* is PC. */
#define QURT_ETM_SOURCE_DATA                1U  /**< ETM memory source of SAC* is data. */

/* Period between synchronization traces */
#define QURT_ETM_ASYNC_PERIOD               0  /**< Async.*/
#define QURT_ETM_ISYNC_PERIOD               1  /**< Isync.*/
#define QURT_ETM_GSYNC_PERIOD               2  /**< Gsync. */

/* ETM enable flags */
#define QURT_ETM_OFF                0U  /**< ETM off. */
#define QURT_ETM_ON                 1U  /**< ETM on. */
/** @endcond */
/** @} */ /* end_addtogroup etm_macros */

/** @addtogroup function_tracing_macro
@{ */
/* ETM setup return values */
#define QURT_ETM_SETUP_OK                   0 /**< ETM setup OK. */
#define QURT_ETM_SETUP_ERR                  1 /**< ETM setup error. */
/** @} */ /* end_addtogroup function_tracing_macro */
/* ETM breakpoint types */
#define QURT_ETM_READWRITE_BRKPT            0U /**< ETM read/write breakpoint. */
#define QURT_ETM_READ_BRKPT                 1U /**< ETM read breakpoint. */
#define QURT_ETM_WRITE_BRKPT                2U /**< ETM write breakpoint. */
#define QURT_ETM_BRKPT_INVALIDATE           3U /**< Invalidate breakpoint. */
/** @addtogroup function_tracing_macro
@{ */
/* ATB status flags */
#define QURT_ATB_OFF                        0  /**< ATB off. */
#define QURT_ATB_ON                         1  /**< ATB on. */
/** @} */ /* end_addtogroup function_tracing_macro */
/* DTM enable flags */
#define QURT_DTM_OFF                0  /**< DTM off. */
#define QURT_DTM_ON                 1  /**< DTM on. */

/** @addtogroup function_tracing_datatypes
@{ */
/**STM trace information. */
typedef struct qurt_stm_trace_info {
   /** @cond */
   unsigned int stm_port_addr[6];   /* STM port address to which trace data must be written.*/
   unsigned int thread_event_id; /* Event ID for context switches.*/
   unsigned int interrupt_event_id; /* Event ID for interrupts. */
   unsigned int marker; /* Marker value that must be written at the beginning of the trace. */
   /** @endcond */
} qurt_stm_trace_info_t;
/** @} */ /* end_addtogroup function_tracing_datatypes */
/*=============================================================================
                            GLOBAL FUNCTIONS
=============================================================================*/


/**@ingroup func_qurt_trace_get_marker
  Gets the kernel trace marker.\n
  Returns the current value of the kernel trace marker.
  The marker consists of a hardware thread identifier and an index into the kernel trace
  buffer. The trace buffer records kernel events.

  @note1hang Using this function with qurt_trace_changed() 
             determines whether certain kernel events occurred in a block of code.

  @return
  Integer -- Kernel trace marker.

  @dependencies
  None.
*/
unsigned int qurt_trace_get_marker(void);

/**@ingroup func_qurt_trace_changed  
  Determines whether specific kernel events have occurred. \n
  Returns a value that indicates whether the specified kernel events are recorded in the
  kernel trace buffer since the specified kernel trace marker was obtained.

  The prev_trace_marker parameter specifies a kernel trace marker that was obtained by calling 
  qurt_trace_get_marker().
  @cond rest_dist For more information on the mask value, see the description of the trace_mask element in 
  @xhyperref{80VB41992,80-VB419-92}. \n @endcond

  @note1hang Used with qurt_trace_get_marker(), this function determines whether
             certain kernel events occurred in a block of code.\n
  @note1cont This function cannot determine whether a specific kernel event type has
             occurred unless that event type has been enabled in the trace_mask element
             of the system configuration file. \n
  @note1cont QuRT supports the recording of interrupt and context switch events only (such as
             a trace_mask value of 0x3).

  @param[in] prev_trace_marker Previous kernel trace marker.
  @param[in] trace_mask        Mask value that indicates which kernel events to check for.

  @returns
  1 -- Kernel events of the specified type have occurred since the
       specified trace marker was obtained.\n
  0 -- No kernel events of the specified type have occurred since the
       specified trace marker was obtained.

  @dependencies
  None.
*/
int qurt_trace_changed(unsigned int prev_trace_marker, unsigned int trace_mask);

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @addtogroup function_tracing_macro
@{ */
#ifndef QURT_DEBUG 
#define QURT_TRACE(str, ...) __VA_ARGS__
  /**< Function tracing is implemented with the QURT_TRACE debug macro, which
       optionally generates printf statements both before and after every function call that is
       passed as a macro argument. 

       For example, in the following macro calls in the source code:
       @code
       QURT_TRACE(myfunc, my_func(33))
       
       @endcode
       generates the following debug output:
       @code
       myfile:nnn: my_func >>> calling my_func(33)
       myfile:nnn: my_func >>> returned my_func(33)
       @endcode
       The debug output includes the source file and line number of the function call, along with
       the text of the call. Compile the client source file with -D __FILENAME__
       defined for its file name.

       The library function qurt_printf() generates the debug output.
       The QURT_DEBUG symbol controls generation of the debug output. If this symbol is
       not defined, function tracing is not generated.\n
       @note1hang The debug macro is accessed through the QuRT API header file. 
        */
#else
#define QURT_TRACE(str, ...) \
	do { \
		qurt_printf("%s:%d: %s: >>> calling %s\n",__FILENAME__,__LINE__,(str),#__VA_ARGS__); \
		__VA_ARGS__; \
		qurt_printf("%s:%d: %s: <<< %s returned\n",__FILENAME__,__LINE__,(str),#__VA_ARGS__); \
	} while (0);
#endif
/** @} */ /* end_addtogroup function_tracing_macro */

/**@ingroup func_qurt_etm_set_pc_range
  Sets the PC address range for ETM filtering.
  Depending on the Hexagon core design, a maximum of four PC ranges are supported.

  @param[in] range_num  0 to 3. 
  @param[in] low_addr   Lower boundary of PC address range.
  @param[in] high_addr  Higher boundary of PC address range.

  @returns
  #QURT_ETM_SETUP_OK -- Success. \n
  #QURT_ETM_SETUP_ERR -- Failure.

  @dependencies
  None.
*/
unsigned int qurt_etm_set_pc_range(unsigned int range_num, unsigned int low_addr, unsigned int high_addr);

/**@ingroup func_qurt_etm_set_range
  Sets the address range for ETM filtering. 
  It allows the user to select the source type of addresses - QURT_ETM_SOURCE_PC and QURT_ETM_SOURCE_DATA.

  @param[in] addr_source_type   Type of the address source:\n
                                - #QURT_ETM_SOURCE_PC \n
                                - #QURT_ETM_SOURCE_DATA @tablebulletend
  @param[in] trig_block_num     0 to 3.
  @param[in] pid                pid of the process
                                1. Any valid PID number will enable the ASID based trace filtering.
                                2. QURT_ETM_NO_PID - Disable the ASID based trace filtering.
  @param[in] low_addr           Lower boundary of PC address range.
  @param[in] high_addr          Higher boundary of PC address range.

  @returns
  #QURT_ETM_SETUP_OK -- Success. \n
  #QURT_ETM_SETUP_ERR -- Failure.

  @dependencies
  None.
*/
unsigned int qurt_etm_set_range(unsigned int addr_source_type, unsigned int trig_block_num, unsigned int pid, unsigned int low_addr, unsigned int high_addr);

/**@ingroup func_qurt_etm_set_atb
  Sets the advanced trace bus (ATB) state to notify QuRT that the ATB is actively enabled or disabled.
  QuRT performs the corresponding actions at low power management.
  
  @param[in] flag Values: \n
                         #QURT_ATB_ON \n
						 #QURT_ATB_OFF  
      
  @returns
  #QURT_ETM_SETUP_OK  -- Success. \n
  #QURT_ETM_SETUP_ERR -- Failure

  @dependencies
  None.
*/
unsigned int qurt_etm_set_atb(unsigned int flag);

/**@ingroup func_qurt_etm_set_sync_period
  Sets the period for types of synchronization trace packets. \n
  ASYNC defines the period between alignment synchronization packets.
         Period is in terms of bytes in the packet stream. \n 
  ISYNC defines the period between instruction synchronization packets.
         Period is per thread and is defined as the bytes sent out for that thread. \n
  GSYNC is the defined period in thread cycles between GSYNC packets.

  @param[in]  sync_type Type of synchronization packets: \n
                          #QURT_ETM_ASYNC_PERIOD \n
                          #QURT_ETM_ISYNC_PERIOD \n
                          #QURT_ETM_GSYNC_PERIOD
  @param[in]  period    Period value. 

  @return
  #QURT_ETM_SETUP_OK -- Success. \n
  #QURT_ETM_SETUP_ERR -- Failure.

  @dependencies
  None.
 */
unsigned int qurt_etm_set_sync_period(unsigned int sync_type, unsigned int period);

/**@ingroup func_qurt_stm_trace_set_config
  Sets up a STM port for tracing events.

  @datatypes
  #qurt_stm_trace_info_t 

  @param[in]  stm_config_info Pointer to the STM trace information used to set up the trace
              in the kernel.
			  The strucure must have the following:\n
			  - One port address per hardware thread \n
			  - Event ID for context switches \n
			  - Event ID for interrupt tracing n
			  - Header or marker to identify the beginning of the trace. @tablebulletend

  @return
  #QURT_EOK -- Success. \n
  #QURT_EINVALID -- Failure; possibly because the passed port address is not in the page table.

  @dependencies
  None.
 */
unsigned int qurt_stm_trace_set_config(qurt_stm_trace_info_t *stm_config_info);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_TRACE_H */
