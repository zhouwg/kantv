/**=============================================================================
@file
    mem_utils.h

@brief
    Abstract operating system specific timing APIs.

Copyright (c) 2021 Qualcomm Technologies Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
=============================================================================**/

#ifndef MEM_UTILS_H_
#define MEM_UTILS_H_


#ifdef __cplusplus
  extern "C" {
#endif


#ifdef _WINDOWS
    #include <windows.h>
#else
#ifdef __hexagon__
#else
    #include <malloc.h>
    #include <sys/mman.h>
#endif
#endif


#ifndef MEMALIGN
  #ifdef _WINDOWS
    #define MEMALIGN(alignment,size) _aligned_malloc(size,alignment)
  #else
    #define MEMALIGN(alignment,size) memalign(alignment,size)
  #endif
#endif


#ifndef ALIGNED_FREE
  #ifdef _WINDOWS
    #define ALIGNED_FREE(ptr) _aligned_free(ptr)
  #else
    #define ALIGNED_FREE(ptr) free(ptr)
  #endif
#endif

#ifdef __cplusplus
}
#endif


#endif //MEM_UTILS_H_
