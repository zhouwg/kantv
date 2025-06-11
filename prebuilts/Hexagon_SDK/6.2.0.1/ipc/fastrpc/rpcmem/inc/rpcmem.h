/*==============================================================================
  Copyright (c) 2012-2013, 2020 Qualcomm Technologies, Inc.
  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors
  may be used to endorse or promote products derived from this software without
  specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
==============================================================================*/

#ifndef RPCMEM_H
#define RPCMEM_H

#include "AEEStdDef.h"
#include "stddef.h"

/**
 *  @file rpcmem.h
 *  @brief APIs used to manage memory allocated by the application processor and shared with the DSP.
 */

/** @defgroup rpcmem_const RPCMEM API macros and enumerations
 *  @{
 */

/**
 * Allocate memory with the same properties as the ION_FLAG_CACHED flag.
 */
#ifdef ION_FLAG_CACHED
#define RPCMEM_DEFAULT_FLAGS ION_FLAG_CACHED
#else
#define RPCMEM_DEFAULT_FLAGS 1
#endif

/**
 * The FastRPC library tries to map buffers allocated with this flag to the remote process of all current and new
 * FastRPC sessions. In case of failure to map, the FastRPC library ignores the error and continues to open the session
 * without pre-mapping the buffer. In case of success, buffers allocated with this flag will be pre-mapped to reduce
 * the latency of upcoming FastRPC calls. This flag is recommended only for buffers that are used with latency-critical
 * FastRPC methods. Pre-mapped buffers will be unmapped during either buffer free or session close.
 */
#define RPCMEM_TRY_MAP_STATIC   0x04000000

/**
 *  Supported RPCMEM heap IDs.
 *
 * If you are not using any of the RPCMEM-defined heap IDs,
 * you are responsible for ensuring that you are passing
 * a valid ION heap ID.
 */
enum rpc_heap_ids {
/**
 *  Memory for secure use cases only.
 *  * Secure heap is to be used only by clients migrating to CPZ
 */
       RPCMEM_HEAP_ID_SECURE   = 9,
/**
 *  Contiguous physical memory:
 *  * Very limited memory is available (< 8 MB)
 *  * Recommended for subsystems without SMMU (sDSP and mDSP)
 *  * Contiguous heap memory will be deprecated from archs after v73
 */
       RPCMEM_HEAP_ID_CONTIG   = 22,
/**
 *  Non-contiguous system physical memory.
 *  * Recommended for all use cases that do not require using a specific heap
 *  * Used with subsystems with SMMU (cDSP and aDSP)
 */
       RPCMEM_HEAP_ID_SYSTEM   = 25,
 };

/**
 * Use uncached memory.
 */
#define RPCMEM_FLAG_UNCACHED 0

/**
 * Use cached memory.
 */
#define RPCMEM_FLAG_CACHED RPCMEM_DEFAULT_FLAGS

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup rpcmem_api RPCMEM API functions
 *  @{
 */

/**
 * Initialize the RPCMEM Library.
 *
 * Only call this function once before using the RPCMEM Library.
 *
 * This API is mandatory on pre-Lahaina targets IF the client has linked to the
 * rpcmem.a static library. If the client has only linked libadsprpc.so,
 * libcdsprpc.so, or libsdsprpc.so, then the rpcmem_init call is not required
 * on any target and other rpcmem APIs such as rpcmem_alloc can be called
 * directly.
 *
 * NOTE: This function is not thread safe.
 */
void rpcmem_init(void);

/**
 * Deinitialize the RPCMEM Library.
 *
 * Only call this function once when the RPCMEM Library is no longer required.
 *
 * This API is mandatory on pre-Lahaina targets IF the client has linked to the
 * rpcmem.a static library. If the client has only linked libadsprpc.so,
 * libcdsprpc.so, or libsdsprpc.so, then the rpcmem_deinit call is not required
 * on any target.
 *
 * NOTE: This function is not thread safe.
 */
void rpcmem_deinit(void);

/**
 * Allocate a zero-copy buffer for size upto 2 GB with the FastRPC framework.
 * Buffers larger than 2 GB must be allocated with rpcmem_alloc2
 * @param[in] heapid  Heap ID to use for memory allocation.
 * @param[in] flags   ION flags to use for memory allocation.
 * @param[in] size    Buffer size to allocate.
 * @return            Pointer to the buffer on success; NULL on failure.
 *
 * Examples:
 *
 * * Default memory attributes, 2 KB
 * @code
 *    rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, 2048);
 * @endcode
 * Or
 * @code
 *    rpcmem_alloc_def(2048);
 * @endcode
 *
 * * Heap 22, uncached, 1 KB
 * @code
 *    rpcmem_alloc(22, 0, 1024);
 * @endcode
 * Or
 * @code
 *    rpcmem_alloc(22, RPCMEM_FLAG_UNCACHED, 1024);
 * @endcode
 *
 * * Heap 21, cached, 2 KB
 * @code
 *    rpcmem_alloc(21, RPCMEM_FLAG_CACHED, 2048);
 * @endcode
 * Or
 * @code
 *    #include <ion.h>
 *    rpcmem_alloc(21, ION_FLAG_CACHED, 2048);
 * @endcode
 *
 * * Default memory attributes but from heap 18, 4 KB
 * @code
 *    rpcmem_alloc(18, RPCMEM_DEFAULT_FLAGS, 4096);
 * @endcode
 */
void* rpcmem_alloc(int heapid, uint32 flags, int size);

/**
 * Allocate a zero-copy buffer with the FastRPC framework.
 * @param[in] heapid  Heap ID to use for memory allocation.
 * @param[in] flags   ION flags to use for memory allocation.
 * @param[in] size    Buffer size to allocate.
 * @return            Pointer to the buffer on success; NULL on failure.
 *
 * Examples:
 *
 * * The usage examples are same as rpcmem_alloc.
 */
void* rpcmem_alloc2(int heapid, uint32 flags, size_t size);

/**
 * Allocate a buffer with default settings.
 * @param[in] size  Size of the buffer to be allocated.
 * @return          Pointer to the allocated memory buffer.
 */
 #if !defined(WINNT) && !defined (_WIN32_WINNT)
__attribute__((unused))
#endif
static __inline void* rpcmem_alloc_def(int size) {
   return rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, size);
}

/**
 * Free a buffer and ignore invalid buffers.
 */
void rpcmem_free(void* po);

/**
 * Return an associated file descriptor.
 * @param[in] po  Data pointer for an RPCMEM-allocated buffer.
 * @return        Buffer file descriptor.
 */
int rpcmem_to_fd(void* po);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

//! @cond Doxygen_Suppress
/** These macros are deprecated.
 */
#define RPCMEM_DEFAULT_HEAP     -1
#define RPCMEM_HEAP_DEFAULT     0x80000000
#define RPCMEM_HEAP_NOREG       0x40000000
#define RPCMEM_HEAP_UNCACHED    0x20000000
#define RPCMEM_HEAP_NOVA        0x10000000
#define RPCMEM_HEAP_NONCOHERENT 0x08000000
#define RPCMEM_FORCE_NOFLUSH    0x01000000
#define RPCMEM_FORCE_NOINVALIDATE    0x02000000
// Use macros from libion instead
#define ION_SECURE_FLAGS    ((1 << 31) | (1 << 19))
//! @endcond

#endif //RPCMEM_H
