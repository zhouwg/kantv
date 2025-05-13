#ifndef QURT_H
#define QURT_H 

/**
  @file qurt.h 
  @brief  Contains kernel header files that provide kernel OS API functions, constants, and 
  definitions 

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013,2021,2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/
/*======================================================================
 *
 *											 EDIT HISTORY FOR FILE
 *
 *	 This section contains comments describing changes made to the
 *	 module. Notice that changes are listed in reverse chronological
 *	 order.
 *
 *	
 *
 *
 * when 				who 		what, where, why
 * ---------- 	--- 		------------------------------------------------
 * 2011-02-25 	op			Add Header file
   2012-12-16   cm          (Tech Pubs) Edited/added Doxygen comments and markup.
 ======================================================================*/
 

#ifdef __cplusplus
extern "C" {
#endif

#include "qurt_consts.h"
#include "qurt_api_version.h"
#include "qurt_alloc.h"
#include "qurt_futex.h"
#include "qurt_mutex.h"
#include "qurt_pipe.h"
#include "qurt_printf.h"
#include "qurt_assert.h"
#include "qurt_thread.h"
#include "qurt_trace.h"
#include "qurt_cycles.h"
#include "qurt_profile.h"
#include "qurt_sem.h"
#include "qurt_cond.h"
#include "qurt_barrier.h"
#include "qurt_fastint.h"
#include "qurt_allsignal.h"
#include "qurt_anysignal.h"
#include "qurt_signal.h"
#include "qurt_rmutex.h"
#include "qurt_pimutex.h"
#include "qurt_signal2.h"
#include "qurt_rmutex2.h"
#include "qurt_pimutex2.h"
#include "qurt_int.h"
#include "qurt_lifo.h"
#include "qurt_power.h"
#include "qurt_event.h"
#include "qurt_pmu.h"
#include "qurt_stid.h"
//#include "qurt_version.h"
#include "qurt_tlb.h"
#include "qurt_vtlb.h"
#include "qurt_memory.h"
#include "qurt_qdi.h"
#include "qurt_sclk.h"
#include "qurt_space.h"
#include "qurt_process.h"
#include "qurt_timer.h"
#include "qurt_tls.h"
#include "qurt_thread_context.h"
#include "qurt_hvx.h"
#include "qurt_hmx.h"
#include "qurt_mailbox.h"
#include "qurt_island.h"
#include "qurt_qdi_proxy.h"
#include "qurt_l2cfg.h"
#include "qurt_mmap.h"
#include "qurt_isr.h"
#include "qurt_busywait.h"
#include "qurt_ecc.h"
#include "qurt_callback.h"
#include "qurt_error.h"
#include "qurt_except.h"
#include "qurt_mq.h"
#include "qurt_user_dma.h"
#include "qurt_fs_hub.h"	
#include "qurt_os_services.h"	

#ifndef MAIN_ONLY
#define INCLUDE_ISLAND_CONTENTS
#endif
#ifndef ISLAND_ONLY
#define INCLUDE_MAIN_CONTENTS
#endif

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_H */

