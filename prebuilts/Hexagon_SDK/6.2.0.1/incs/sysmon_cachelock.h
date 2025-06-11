/*-----------------------------------------------------------------------------
   Copyright (c) 2017-2020 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------------*/

#ifndef SYSMON_CACHELOCK_H_
#define SYSMON_CACHELOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @file sysmon_cachelock.h
 *  @brief CDSP L2 cache locking manager API
 */

/**
 * Allocates a memory buffer, locks it in L2 cache, and returns the locked
 * virtual address.
 *
 * @param[in] size Memory size (in bytes) to lock. 
 * @param[out] paddr_ptr Pointer to @c unsigned @c long @c long
 *                       variable to get the locked 64-bit physical address upon
 *                       success. NULL if the allocation and cache lock failed.
 *
 * @return
 * @c void* Virtual address of the locked memory region. \n
 * 0 if the requested buffer size could not be allocated and locked in the L2 cache.
 */
void* HAP_cache_lock(unsigned int size, unsigned long long *paddr_ptr);


/**
 * Unlocks cache and deallocates memory for a virtual address returned by 
 * the corresponding HAP_cache_lock() call.
 *
 * @param[in] vaddr_ptr Virtual address of the memory block to unlock.
 *
 * @return 
 * 0 upon success. \n
 * Other values upon failure.
 */
int HAP_cache_unlock(void* vaddr_ptr);

/**
 * Locks the cache for a given virtual address and memory size (in bytes).
 *
 * Align the address and size to 32 bytes. The size should not be more 
 * than 64 KB, and at any point of time, only one such request is honored
 * (this restriction has been removed from SM8250 onwards).
 * 
 * Use this function to lock an existing memory block, for example, 
 * to lock a code segment or data buffer. Note that whenever possible, it is
 * preferable to let the driver allocate the memory to be locked in L2 via the
 * HAP_cache_lock API, as it can often avoid the fragmentation likely to occur
 * when the user provides the memory ranges to be locked. 
 *
 * @param[in] vaddr_ptr Virtual address of the memory block to lock; should be
 *                      32-byte aligned.
 * @param[in] size Memory size (in bytes) to lock; should be 32-byte aligned. 
 *                 The maximum size limit is 64 KB. From SM8250, this size limit is
 *                 the same as HAP_cache_lock().
 *
 * @return
 * 0 upon success. \n
 * Others values upon failure.
 */
int HAP_cache_lock_addr(void* vaddr_ptr, unsigned int size);

/**
 * Unlocks the cache for a given virtual address.
 *
 * Use this function together with HAP_cache_lock_addr().
 *
 * @param[in] vaddr_ptr Virtual address of the memory block to unlock.
 *
 * @return 
 * 0 upon success. \n
 * Other values upon failure.
 */
int HAP_cache_unlock_addr(void* vaddr_ptr);

/**
 * Queries the API to get the size of largest contiguous memory block available for 
 * cache locking.
 *
 * @return 
 * Available size in bytes upon success. \n
 * -1 upon failure.
 */
int HAP_query_avail_cachelock(void);

/**
 * Queries the API to get the total locked cache size. 
 *
 * @return 
 * Total locked cache size in bytes upon success. \n
 * -1 upon failure.
 */
int HAP_query_total_cachelock(void); 


#ifdef __cplusplus
}
#endif

#endif /* SYSMON_CACHELOCK_H_ */
