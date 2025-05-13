/*
 * Copyright (c) 2012-2013,2021 QUALCOMM Technologies Inc. All Rights Reserved.
 * Qualcomm Technologies Confidential and Proprietary
 *
 */
#ifndef QTEST_STDLIB_H
#define QTEST_STDLIB_H

#include <assert.h>
#include "rpcmem.h"

#define WHILE(a) \
__pragma(warning(suppress:4127)) while(a)

#define FREEIF(pv) \
   do {\
      if(pv) { \
         void* tmp = (void*)pv;\
         pv = 0;\
         FREE(tmp);\
      } \
   } WHILE(0)

#define ALIGNED_FREEIF(pv) \
   do {\
      if(pv) { \
         void* tmp = (void*)pv;\
         pv = 0;\
         ALIGNED_FREE(tmp);\
      } \
   } WHILE(0)

#ifndef FASTRPC_DMA_FREE
#define FASTRPC_DMA_FREE(pv) rpcmem_free(pv)
#endif

#define FASTRPC_DMA_FREEIF(pv) \
   do {\
      if(pv) { \
         void* tmp = (void*)pv;\
         pv = 0;\
         FASTRPC_DMA_FREE(tmp);\
      } \
   } WHILE(0)

#ifndef QASSERT
#define QASSERT(st) assert(st)
#endif

//had to copy this so not to bring in a1qtest headers
#if (((defined __linux__) && !(defined ANDROID)) || (defined __APPLE__))
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

static __inline char* stacktrace(void) {
   int bufsz = 0, sz = 0;
   char* buf = 0;
   void* callstack[256];
   int i, frames = backtrace(callstack, 256);
   char** strs = backtrace_symbols(callstack, frames);
   bufsz += snprintf(0, 0, "\n");
   for (i = 0; i < frames; ++i) {
      bufsz += snprintf(0, 0, "%s\n", strs[i]);
   }
   buf = malloc(bufsz);
   assert(buf != 0);
   sz += snprintf(buf + sz, bufsz, "\n");
   bufsz -= sz;
   for (i = 0; i < frames && bufsz > 0; ++i) {
      sz += snprintf(buf + sz, bufsz, "%s\n", strs[i]);
      bufsz -= sz;
   }
   free(strs);
   return buf;
}

#else

static __inline char* stacktrace(void) {
   return 0;
}


#endif //ANDROID


#ifndef QTEST
//default implementation for stdlib
#include <stdlib.h>


#define IF_QTEST(vv) (void)0

#ifndef QASSERT
#define QASSERT(st) (void)0
#endif

#ifndef MALLOC
#define MALLOC malloc
#endif

#ifndef FASTRPC_DMA_MALLOC
#define FASTRPC_DMA_MALLOC(heapid, flags, size) rpcmem_alloc(heapid, flags, size)
#endif

#ifndef CALLOC
#define CALLOC calloc
#endif

#ifndef FREE
#define FREE free
#endif

#ifndef REALLOC
#define REALLOC(pv, nsz, osz) realloc(pv, nsz)
#endif

#ifndef ALIGNED_REALLOC
#define ALIGNED_REALLOC(pv, nsz, osz, aln) _aligned_realloc(pv, nsz, aln)
#endif

#ifndef FASTRPC_DMA_REALLOC
#define FASTRPC_DMA_REALLOC(pv, nsz, osz, aln) fastrpc_dma_realloc(pv, nsz, osz)
#endif

#ifndef ALIGNED_FREE
#define ALIGNED_FREE(pv) _aligned_free(pv)
#endif

#define qtest_set_failure_mask(mask) (void)mask
#define qtest_get_failure_mask(mask) (void)mask
#define qtest_set_pass_count(cnt)    (void)cnt
#define qtest_done()                 (void)0
#define qtest_test_failure()         0
#define qtest_atexit(pfn,ctx)        (void)pfn; (void)ctx

#else // QTEST

#include "AEEStdDef.h"

#define IF_QTEST(vv) do {\
   vv \
} while (0)

//causes alloc to fail when mask & 0x1 is true
//each test shifts the mask to the right
void qtest_set_failure_mask(uint32 mask);
void qtest_get_failure_mask(uint32* mask);

//causes alloc to fail when count == 0
//each test decrements the count
void qtest_set_pass_count(int count);

//returns 0 if succeeds and shifts the mask
//usefull for generating controlled failures in functions
int qtest_test_failure(void);

void qtest_atexit(void (*pfnAtExit)(void* pCtx), void* pvCxt);

void qtest_done(void);

void* qtest_malloc(const char* name, char* stack, int sz);

void* qtest_calloc(const char* name, char* stack, int cnt, int sz);

void* qtest_realloc(const char* name, char* stack, void* ptr, int sz);

void qtest_free(const char* name, char* stack, void* rv);

#define MALLOC(sz)         qtest_malloc(__FILE_LINE__, stacktrace(), sz)
#define CALLOC(cnt, sz)    qtest_calloc(__FILE_LINE__, stacktrace(), cnt, sz)
#define REALLOC(ptr, sz)   qtest_realloc(__FILE_LINE__, stacktrace(), ptr, sz)
#define FREE(ptr)          qtest_free(__FILE_LINE__, stacktrace(), ptr)

#endif //QTEST
#endif //QTEST_STDLIB_H
