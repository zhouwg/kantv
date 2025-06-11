/*=============================================================================

                                    qurt_ptrace.h

GENERAL DESCRIPTION

EXTERNAL FUNCTIONS
        None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
        None.

             Copyright (c) 2013  by Qualcomm Technologies, Inc.  All Rights Reserved.
=============================================================================*/
#ifndef __SYS_PTRACE_H__
#define __SYS_PTRACE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum __ptrace_request
{
   /**
     Indicates that the process making this request is requesting to be traced.
   */
   PTRACE_TRACEME = 0,
   PTRACE_EXT_IS_DEBUG_PERMITTED = 500
};

long ptrace(enum __ptrace_request request, unsigned int pid, void*addr, void *data);

#ifdef __cplusplus
}
#endif

#endif //__SYS_PTRACE_H__
