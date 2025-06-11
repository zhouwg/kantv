#ifndef QURT_CONSTS_H
#define QURT_CONSTS_H

/**
  @file qurt_consts.h
  @brief  QuRT constants and definitions

  EXTERNAL FUNCTIONS
   None.

 INITIALIZATION AND SEQUENCING REQUIREMENTS
   None

 Copyright (c) 2013-2023 by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====================================================================
 Constants and macros
 ======================================================================*/

/* Definitions of system events. System events suspend
   a thread and put it into suspending_list.
   The system event number is saved in CONTEXT::error::cause field
   of the suspended thread. An event handler thread such as
   page fault handler or system error handler can wake up the suspended
   thread.
 */
#define QURT_EVENT_PAGEFAULT      0x1 /* Page fault event. */
#define QURT_EVENT_SYSTEM_ERR     0x2 /* System error event. */
#define QURT_EVENT_SUSPEND        0x3
#define QURT_EVENT_PROCESS_EXIT   0x4 /* Process termination event.*/

#define QURT_SYSENV_MAX_THREADS_TYPE           1 /* Maximum threads object. */
#define QURT_SYSENV_PROCNAME_TYPE              2 /* Process name object. */
#define QURT_SYSENV_MAX_PI_PRIO_TYPE           3 /* Maximum pi priority object. */
#define QURT_SYSENV_ARCH_REV_TYPE              4 /* Architecture version object. */
#define QURT_SYSENV_APP_HEAP_TYPE              5 /* Application heap object. */
#define QURT_SYSENV_REGION_ATTR_DEFAULT        7 /* Default region attributes. */
#define QURT_SYSENV_STACK_PROFILE_COUNT_TYPE   8 /* Stack profile count type. */
#define QURT_SYSENV_ISLAND_CONFIG_TYPE         9 /*island configuration check*/
#define QURT_SYSENV_HTHREADS_TYPE              10 /* Active threads objec */
#define QURT_SYSENV_CONFIG_IMAGE_START_LO      11 /* Config image start address for DTB parsing */
#define QURT_SYSENV_CONFIG_IMAGE_START_HI      12 /* Config Image start address for DTB parsing */
#define QURT_SYSENV_CHIPPARAMS_LO              13 /* ChipParams for DTB parsing */
#define QURT_SYSENV_CHIPPARAMS_HI              14 /* ChipParams for DTB parsing */
#define QURT_SYSENV_PLATPARAMS                 15 /* Platformparams for DTB parsing */
#define QURT_SYSENV_CONFIG_IMAGE_SIZE          16 /* Config image Size for DTB parsing */
#define QURT_SYSENV_L2_CACHE_LINE_SIZE         17 /*L2 cache line size*/

/* Get q6 regs */
#define QURT_GET_SSR         1
#define QURT_GET_CCR         2
#define QURT_GET_CFGBASE     3
#define QURT_GET_SYSCFG      4
#define QURT_GET_REV         5


/** @cond rest_reg_dist */
/** @addtogroup performance_monitor_macros
@{ */

/* PMU */
#define QURT_PMUCNT0    0  /**< */
#define QURT_PMUCNT1    1  /**< */
#define QURT_PMUCNT2    2  /**< */
#define QURT_PMUCNT3    3  /**< */
#define QURT_PMUCFG     4  /**< */
#define QURT_PMUEVTCFG  5  /**< */

/* new since V55 */
#define QURT_PMUCNT4    6  /**< */
#define QURT_PMUCNT5    7  /**< */
#define QURT_PMUCNT6    8  /**< */
#define QURT_PMUCNT7    9  /**< */
#define QURT_PMUEVTCFG1 10  /**< */

/* new since V61 */
#define QURT_PMUSTID0   11  /**< */
#define QURT_PMUSTID1   12  /**< */

#define QURT_PMUCNTSTID0   13  /**< */
#define QURT_PMUCNTSTID1   14  /**< */
#define QURT_PMUCNTSTID2   15  /**< */
#define QURT_PMUCNTSTID3   16  /**< */
#define QURT_PMUCNTSTID4   17  /**< */
#define QURT_PMUCNTSTID5   18  /**< */
#define QURT_PMUCNTSTID6   19  /**< */
#define QURT_PMUCNTSTID7   20  /**< */

/** @} */ /* end_addtogroup performance_monitor_macros */
/** @endcond */

/*
 Power collapse operation
*/
#define QURT_POWER_SHUTDOWN       0 /**< */
#define QURT_TCXO_SHUTDOWN        1 /**< */
#define QURT_POWER_CMD_PREPARE    0 /**< */
#define QURT_POWER_CMD_PERFORM    1 /**< */
#define QURT_POWER_CMD_EXIT       2 /**< */
#define QURT_POWER_CMD_FAIL_EXIT  3 /**< */
#define QURT_POWER_CMD_PERFORM_L2_RETENTION 4 /**< */
#define QURT_POWER_CMD_PERFORM_SAVE_TCM     5 /**< */
#define QURT_POWER_CMD_DEEP_SLEEP 6           /**< */


/** @addtogroup thread_macros
@{ */
#define QURT_MAX_HTHREAD_LIMIT    8U /**< Limit on the maximum number of hardware threads supported by QuRT for any
 Hexagon version. Use this definition to define arrays, and so on, in
 target independent code. */
/** @} */ /* end_addtogroup thread_macros */

/** @cond internal_only */
/** @addtogroup power_management_macros
@{ */
/**
  L2 cache retention mode
*/
#define QURT_POWER_SHUTDOWN_TYPE_L2NORET QURT_POWER_CMD_PERFORM /**< */
#define QURT_POWER_SHUTDOWN_TYPE_L2RET   QURT_POWER_CMD_PERFORM_L2_RETENTION /**< */
#define QURT_POWER_SHUTDOWN_TYPE_SAVETCM QURT_POWER_CMD_PERFORM_SAVE_TCM /**< */
/** @} */ /* end_addtogroup power_management_macros */
/** @endcond */

/*
  QURT_system_state
  Use for debugging the shutdown/startup process.

  State transition for cold boot:
  QURT_BOOT_SETUP_ISDB --> QURT_CBOOT_BSP_INIT -->
  QURT_CBOOT_END_CLEAN_INIT --> QURT_CBOOT_END_OS_INIT -->
  QURT_CBOOT_KERNEL_INIT_DONE --> QURT_CBOOT_PLAT_CONFIG_DONE -->
  QURT_CBOOT_ROOT_TASK_STARTED

  State transition for power collapse:
  QURT_PREPARE_SINGLE_MODE --> QURT_PERFORM_IPEND -->
  QURT_PERFORM_SAVE_TLB --> QURT_PERFORM_SWITCH_PC -->
  cache flush states (dependent on L2 retention config)

  State transition for warm boot:
  QURT_BOOT_SETUP_ISDB --> QURT_WBOOT_INIT_TLB -->
  QURT_WBOOT_SET_1TO1_MAP --> QURT_WBOOT_REMOVE_1TO1_MAP -->
  QURT_CBOOT_END_CLEAN_INIT --> QURT_CBOOT_END_OS_INIT
*/
#define QURT_PREPARE_SINGLE_MODE 1 /**< */
#define QURT_PREPARE_END 2 /**< */
#define QURT_PERFORM_IPEND 3 /**< */
#define QURT_PERFORM_SAVE_ISDP 4 /**< */
#define QURT_PERFORM_SAVE_PMU 5 /**< */
#define QURT_PERFORM_SAVE_TLB 6 /**< */
#define QURT_PERFORM_SWITCH_PC 7 /**< */
#define QURT_PERFORM_EXIT 8 /**< */
#define QURT_FLUSH_L1CACHE 9 /**< */
#define QURT_FLUSH_L2CACHE 0xA /**< */
#define QURT_FLUSH_CACHE_DONE 0xB /**< */
#define QURT_SWITCH_PC_DONE 0xC /**< */
#define QURT_BOOT_SETUP_ISDB 0xD /**< */
#define QURT_WBOOT_INIT_TLB 0xE /**< */
#define QURT_WBOOT_SET_1TO1_MAP 0xF /**< */
#define QURT_WBOOT_CFG_ADV_SYSCFG 0x10 /**< */
#define QURT_WBOOT_REMOVE_1TO1_MAP 0x11 /**< */
#define QURT_CBOOT_BSP_INIT 0x12 /**< */
#define QURT_CBOOT_END_CLEAN_L1CACHE 0x13 /**< */
#define QURT_CBOOT_END_CLEAN_INIT 0x14 /**< */
#define QURT_CBOOT_END_OS_INIT 0x15 /**< */
#define QURT_CBOOT_TLB_DUMP_LOAD 0x16 /**< */
#define QURT_CBOOT_TLB_STATIC_LOAD 0x17 /**< */
#define QURT_CBOOT_KERNEL_INIT_DONE 0x18 /**< */
#define QURT_CBOOT_PLAT_CONFIG_DONE 0x19 /**< */
#define QURT_CBOOT_ROOT_TASK_STARTED 0x1A /**< */
#define QURT_IMPRECISE_EXCEPTION 0x1B /**< */
#define QURT_WBOOT_DEBUG_L2_START 0x1C /**< */
#define QURT_WBOOT_DEBUG_L2_END   0x1D /**< */
#define QURT_NMI_SAVE_L2VIC_COMPLETE   0x1E /**< */
#define QURT_NMI_HANDLER_COMPLETE   0x1F /**< */
#define QURT_NMI_AFTER_SAVE_GLOBAL 0x20 /**< */
#define QURT_WBOOT_START 0x21 /**< */
#define QURT_ENTER_ISLAND 0x22 /**< */
#define QURT_EXIT_ISLAND 0x23 /**< */
#define QURT_LOAD_NOTIFIER_TCB 0x24 /**< */
#define QURT_ABNORMAL_RESET 0x25 /**< */
/*
  Thread attributes
*/

#define QURT_THREAD_ATTR_GP                    0x00000002 /*< */
#define QURT_THREAD_ATTR_UGP                   0x00000003 /*< User general pointer (UGP)*/
#define QURT_THREAD_ATTR_PREFETCH              0x00000004 /*< */
#define QURT_THREAD_ATTR_TID                   0x00000005 /*< */
#define QURT_THREAD_ATTR_CACHE_PART            0x00000007 /*< */
#define QURT_THREAD_ATTR_COPROCESSOR           0x00000008 /*< */
#define QURT_THREAD_ATTR_GET_L2CACHE_PART      0x00000009 /*< */
#define QURT_THREAD_ATTR_SET_FRML              0x0000000A /*< */
#define QURT_THREAD_ATTR_STID_GET              0x0000000B /*< */
#define QURT_THREAD_ATTR_STID_SET              0x0000000C /*< */
#define QURT_THREAD_ATTR_AUTOSTACK             0x0000000D /*< */
#define QURT_THREAD_ATTR_SYSTEM_THREAD         0x0000000E /*< */
#define QURT_THREAD_ATTR_STID_SET2             0x0000000F /*< */
#define QURT_THREAD_ATTR_STID_SET2_ACKNOWLEDGE 0x00000010 /*< */
#define QURT_THREAD_ATTR_STID_GET2             0x00000011 /*< */

/**  Cache operations*/
#define QURT_DCCLEAN                0U   /* Clean Dcache. */
#define QURT_DCINV                  1U   /* Invalidate Dcache. */
#define QURT_DCCLEANINV             2U   /* Clean and invalidate Dcache. */
#define QURT_ICINV                  3U   /* Invalidate Icache. */
#define QURT_DUMP_DCTAGS            4U  /* For testing purpose. */
#define QURT_FLUSH_ALL              5U  /* Flush entire L1 and L2 cache. */
#define QURT_TABLE_FLUSH            6U  /* Flush based on table of physical pages */
#define QURT_CLEAN_INVALIDATE_ALL   7U  /* Flush and invalidate entire L1 and L2 cache. */
#define QURT_L2CACHE_LOCK_LINES     8U  /* l2 cache lock lines */
#define QURT_L2CACHE_UNLOCK_LINES   9U  /* l2 cache unlock lines */
#define QURT_CLEAN                  10U  /* Flush L1 and L2 cache */
#define QURT_CLEAN_INVALIDATE       11U  /* Flush and invalidate L1 and L2 cache. */
#define QURT_CLEAN_INVALIDATE_L2    12U  /* Flush and invalidate entire L2 cache. */

/**@ingroup chapter_prefined_symbols */
/**@xreflabel{hdr:QURT_API_VERSION}*/


/* Process state. */
#define QURT_UPDATE_PROCESS_STATE   0 /**< */
#define QURT_MP_INIT        1 /*< */
#define QURT_MP_RUNNING     2 /*< */
#define QURT_MP_STOPPED     3 /*< */

/* QuRT reset reason. */
#define QURT_NORMAL_BOOT               0  /* Normal boot. */
#define QURT_WARM_BOOT                 1  /* Power collapse warm boot. */
#define QURT_WARM_BOOT_L2_RETENTION    2  /* Power collapse with L2 retention warm boot. */
#define QURT_WARM_BOOT_SAVE_TCM        3  /* Power collapse with saving TCM. */
#define QURT_QUICK_BOOT                4  /* Deep sleep. */

/* QuRT Wait for Idle command */
#define QURT_WAIT_FOR_IDLE_DISABLE  0 /*< */
#define QURT_WAIT_FOR_IDLE_ENABLE   1 /*< */
#define QURT_WAIT_FOR_IDLE     2 /*< */
#define QURT_WAIT_FOR_IDLE_CANCEL 3 /*< */

/*QuRT island exit stages */
#define QURT_ISLAND_EXIT_STAGE1 1 /*< */
#define QURT_ISLAND_EXIT_STAGE2 2 /*< */

#define QURT_MAX_NAME_LEN   64 /*< */

#define MAX_POOL_RANGES     16 /*< */

/* key definitions for debug thread info */
//#define MAX_TCB_KEY           40    //whatever is a good number or makes debug thread structure be 1K
#define KEY_SCHDULER_STATE      1   /*< */
#define KEY_PRIORITY            2   /*< */
#define KEY_PRIORITY_ORIG       3   /*< */
#define KEY_STACK_BOTTOM        4    // Currently not populated
#define KEY_STACK_TOP           5    // Currently not populated
#define KEY_HVX_STATE           6    /*< */
#define KEY_FUTEX_OBJECT        7    /*< */
#define KEY_THREAD_ID           8    /*< */
#define KEY_PROFILE_CYCLE_LO    9    // Currently not populated
#define KEY_PROFILE_CYCLE_HI    10   // Currently not populated
#define KEY_ERROR_ADDRESS       11   // This holds the BADVA
#define KEY_ERROR_CAUSE         12   // This is the same as QURT_error_info.cause
#define KEY_ERROR_CAUSE2        13   // This is the same as QURT_error_info.cause2
#define KEY_ERROR_SSR           14   /*< Holds the SSR value */
#define QURT_RESERVED           -1

/* VTLB method IDs. */
#define QURT_VTLB_ENTRY_CREATE          0U
#define QURT_VTLB_ENTRY_DELETE          1U
#define QURT_VTLB_ENTRY_READ            2U
#define QURT_VTLB_ENTRY_WRITE           3U
#define QURT_VTLB_ENTRY_PROBE           4U
#define QURT_VTLB_ENTRY_SPLIT           5U
#define QURT_VTLB_ENTRY_MERGE           6U
#define QURT_VTLB_ENTRY_STATISTICS      7U
#define QURT_VTLB_ENTRY_SET_SPECIAL     8U
#define QURT_VTLB_QUEUE_PPAGE           9U
#define QURT_VTLB_RECLAIM_STACK_PAGES   10U
#define QURT_VTLB_ASID_SET_STATE_FAST   11U
#define QURT_VTLB_ASID_SET_STATE        12U
#define QURT_VTLB_ENTRY_SET_EXTENSION   13U
#define QURT_VTLB_ENTRY_CLEAR_EXTENSION 14U

/* VTCM window access control HWIO programming. */
#define QURT_VTCM_WINDOW_ENABLE             1U
#define QURT_VTCM_WINDOW_DISABLE            0U
#define QURT_VTCM_WINDOW_HI_OFFSET_DEFAULT  0xFFFU
#define QURT_VTCM_WINDOW_LO_OFFSET_DEFAULT  0U

/** @cond */
/* ETM source - PC or data access */
#define QURT_ETM_SOURCE_PC          0U  /**< Memory source of SAC* is PC. */
#define QURT_ETM_SOURCE_DATA        1U  /**< Memory source of SAC* is data. */

/* ETM PID status flags */
#define QURT_ETM_NO_PID             0xFFFFFFFF /**< No PID is selected. */
/** @endcond */

/* execution context */
#define QURT_CTX_USER       1
#define QURT_CTX_GUEST      2

/* Profiling STID */
#define QURT_STID_DEFAULT   0U

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_CONSTS_H */
