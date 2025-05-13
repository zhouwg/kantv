#ifndef QURT_QDI_IMACROS_H
#define QURT_QDI_IMACROS_H

/**
  @file  qurt_qdi_imacros.h 
  @brief  Internal macros used for QDI. Mostly consists of tricky (and ugly)
  preprocessor hacks that permit us to do varargs function invocations
  where we pass optional arguments in registers and where we can do
  type casting and checking automatically.

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#define _QDMPASTE(a,b) _QDMPASTE_(a,b)
#define _QDMPASTE_(a,b) a##b
#define _QDMCNT(...) _QDMCNT_(__VA_ARGS__,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define _QDMCNT_(a,b,c,d,e,f,g,h,i,j,k,l,cnt,...) cnt

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
