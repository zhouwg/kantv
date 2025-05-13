/*
 * Copyright (c) 2012-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc
 */

#ifndef HAP_MEM_H
#define HAP_MEM_H
#include <stdlib.h>
#include "AEEStdDef.h"
#include "AEEStdErr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file HAP_mem.h
 * @brief HAP Memory APIs
 */


/*
 * Protections are chosen from these bits, or-ed together
 */


 /*! @name HAP_PROT
     \brief These macros define the permissions on memory block described by the file descriptor.

     It is passed as input parameter 'prot' to HAP_mmap(). These can be ORed to set the required permissions.


*/

///@{

/*!     \def HAP_PROT_NONE
        \brief Passing HAP_PROT_NONE as input results in setting 'NO' permissions on the buffer.
*/
#define HAP_PROT_NONE   0x00    /* no permissions */

/*!      \def HAP_PROT_READ
         \brief Passing HAP_PROT_READ as input results in setting 'Read' permissions on the buffer.
*/
#define HAP_PROT_READ   0x01    /* pages can be read */
/*!      \def HAP_PROT_WRITE
     \brief Passing HAP_PROT_WRITE as input results in setting 'Write' permissions on the buffer.
*/

#define HAP_PROT_WRITE  0x02    /* pages can be written */

/*!
     \def HAP_PROT_EXEC
     \brief Passing HAP_PROT_EXEC as input results in setting 'Execute' permissions on the buffer. Currently not supported.
*/
#define HAP_PROT_EXEC   0x04    /* pages can be executed */


///@}

/*
 * Cache policy or-ed with protections parameter
 */

 /*! @name HAP_MEM_CACHE
     \brief These macros define the cache policies for mapping memory pages to DSP MMU. Default cache policy is cache writeback.

     It is passed as input parameter 'prot', or-ed with page protections to HAP_mmap().

*/

///@{

/*!     \def HAP_MEM_CACHE_WRITEBACK
        \brief Passing HAP_MEM_CACHE_WRITEBACK as input results in mapping memory as cache writeback
*/
#define HAP_MEM_CACHE_WRITEBACK    (0x10)    /* cache write back */

/*!      \def HAP_MEM_CACHE_NON_SHARED
         \brief Passing HAP_MEM_CACHE_NON_SHARED as input results in mapping memory as uncached
*/
#define HAP_MEM_CACHE_NON_SHARED   (0x20)    /* normal uncached memory */

/*!      \def HAP_MEM_CACHE_WRITETHROUGH
     \brief Passing HAP_MEM_CACHE_WRITETHROUGH as input results in mapping memory as cache write through
*/

#define HAP_MEM_CACHE_WRITETHROUGH  (0x40)    /* write through memory */

///@}

/*! @name HAP_MEM_FLAGS
     \brief These macros define the buffer attribute flags for allocating APPS memory from the DSP.

     It is passed as input parameter 'flags' to HAP_apps_mem_request().

*/

///@{

/*!     \def HAP_MEM_FLAGS_SKIP_DSP_MAP
        \brief Allocate memory on HLOS but skip DSP mapping
*/

#define HAP_MEM_FLAGS_SKIP_DSP_MAP    0

/*!     \def HAP_MEM_FLAGS_DSP_MAP
        \brief Allocate memory on HLOS and map on DSP
*/

#define HAP_MEM_FLAGS_DSP_MAP         1

/*!     \def HAP_MEM_FLAGS_EXTENDED_MAP
        \brief Allocate memory on HLOS and map beyond 4GB virtual address range on DSP.

        Unsupported currently. Reserved for future use.
*/

#define HAP_MEM_FLAGS_EXTENDED_MAP    2

/*!     \def HAP_MEM_FLAGS_MAX
        \brief Max number of flags supported by HAP_apps_mem_request
*/

#define HAP_MEM_FLAGS_MAX             (HAP_MEM_FLAGS_EXTENDED_MAP + 1)

///@}

/**
 * Allocate a block of memory.
 * @param[in] bytes size of memory block in bytes.
 * @param[out] pptr pointer to the memory block
 * @return int AEE_SUCCESS for success and AEE_ENOMEMORY for failure.
 */

static inline int HAP_malloc(uint32 bytes, void** pptr)
{
    *pptr = malloc(bytes);
    if (*pptr) {
        return AEE_SUCCESS;
    }
    return AEE_ENOMEMORY;
}

/**
 * Free the memory block allocated through HAP_malloc().
 * @param[in] ptr pointer to the memory block
 * @return int AEE_EBADCLASS if ptr is NULL
               AEE_SUCCESS if ptr is not NULL

 */

static inline int HAP_free(void* ptr)
{
    if(ptr == NULL)
        return AEE_EBADCLASS;
    free(ptr);
    return AEE_SUCCESS;
}

/** Statistics of user heap memory */
struct HAP_mem_stats {
   uint64 bytes_free; /**< number of bytes free from all the segments,
                        * may not be available for a single alloc
                        */
   uint64 bytes_used; /**< number of bytes used */
   uint64 seg_free;   /**< number of segments free */
   uint64 seg_used;   /**< number of segments used */
   uint64 min_grow_bytes; /**< minimum number of bytes to grow the heap by when creating a new segment */
};

/**
 * @brief Enum for reqID for HAP_mem_get_heap_info()
 */
enum HAP_mem_stats_request {
  USAGE_STATS = 1,
  MAX_USED
};

/**
 * @brief RequestID/Response for HAP_mem_get_heap_info
 */
typedef struct {
  enum HAP_mem_stats_request req_id;
  union {
    struct HAP_mem_stats usage_stats;
    unsigned long max_used; /* Peak heap usage */
  };
} HAP_mem_heap_info_t;

/**
 * Get the current statistics from the heap.
 *
 * @param[in,out] stats pointer to stats structure
 * @retval AEE_SUCCESS
 */
int HAP_mem_get_stats(struct HAP_mem_stats *stats);

/**
 * Get the heap info.
 *
 * @param payload, pointer to store the request/response
 * @retval, 0 on success
 */
int HAP_mem_get_heap_info(HAP_mem_heap_info_t *payload);

/**
 * Enum to hold the START and END marker values
 *
 */
typedef enum
{
  START = 0,
  END
} marker_t;

/**
 * Request types:
 * HAP_MEM_LOG_BLOCKS - to log all the blocks to csv
 *                      file named: hprt_block_info_<asid>.csv
 *
 * HAP_MEM_SET_MARKER - to set markers for different instances.
 *                      (2^16 instances are possible per application)
 *
 * HAP_MEM_MAP        - to map buffer at random VA or reserved VA
 *
 * HAP_MEM_UNMAP      - to unmap buffer
 *
 * HAP_RESERVE_VA     - to reserve VA space on DSP without mapping
 *
 * HAP_UNRESERVE_VA   - to unreserve VA space on DSP
 */
typedef enum
{
  HAP_MEM_LOG_BLOCKS = 1,
  HAP_MEM_SET_MARKER = 2,
  HAP_MEM_MAP        = 3,
  HAP_MEM_UNMAP      = 4,
  HAP_RESERVE_VA     = 5,
  HAP_UNRESERVE_VA   = 6
} HAP_mem_req_t;

/**
 * Payload structure for HAP_MEM_SET_MARKER request
 * marker_type, START or END marker
 * instance, incase of START - NOOP; if request is success, instance number.
 *           incase of END   - instance number to find leaks
 *
 */
typedef struct
{
  marker_t marker_type;
  uint16_t instance;
} HAP_mem_marker_payload_t;

/* Payload structure for HAP_MEM_MAP request */
typedef struct {
    uint64_t addr;   // [in] reserved va (optional). If 0, buffer mapped at random VA
    uint64_t len;    // [in] length of buffer to be mapped
    int prot;        // [in] permissions and cache-mode of mapping
    int flags;       // [in] buffer flags
    int fd;          // [in] file descriptor of buffer
    uint64_t dsp_pa; // [in] Offset
    uint64_t dsp_va; // [out] Mapped DSP virtual address
} HAP_mem_map_t;

/* Payload structure for HAP_MEM_UNMAP request */
typedef struct {
    uint64_t dsp_va;   // [in] DSP VA to be unmapped
    uint64_t len;      // [in] length of mapping
} HAP_mem_unmap_t;

/* Payload structure for HAP_RESERVE_VA request */
typedef struct {
  uint64_t len;     // [in] Length of VA space to be reserved
  int prot;         // [in] Permissions of the VA space
  int flags;         // [in] flags (unused for now)
  uint64_t dsp_va;  // [out] Reserved DSP virtual address
} HAP_mem_reserve_t;

/* Payload structure for HAP_UNRESERVE_VA request */
typedef struct {
   uint64_t dsp_va;   // [in] DSP VA to be unreserved
   uint64_t len;      // [in] Length of buffer to be unreserved
} HAP_mem_unreserve_t;

/**
 * Payload for different requests
 * New request payload structures should be
 * added to the union.
 */
typedef struct
{
  HAP_mem_req_t request_id;
  union {
    HAP_mem_marker_payload_t mem_marker_payload;
    HAP_mem_map_t mmap;
    HAP_mem_unmap_t munmap;
    HAP_mem_reserve_t reserve;
    HAP_mem_unreserve_t unreserve;
  };
} HAP_mem_req_payload_t;

/**
 * Generic request API, which will decode request type
 * and use the payload to parse the input and output
 * for the request
 * @param mem_payload- input and output payload for the request
 * @retval 0 on success.
 */
int HAP_mem_request(HAP_mem_req_payload_t *mem_payload);

/**
 * Set the minimum and maximum grow size.
 *
 * This API allows to configure the minimum and maximum size that should
 * be added to the DSP user heap when an allocation fails and more memory
 * needs to be obtained from the HLOS. Using this API is optional. If not
 * used, the runtime will try to choose reasonable growth sizes based on
 * allocation history.
 *

 * @param[in] min minimum bytes to grow the heap by when requesting a new segment
 * @param[in] max maximum bytes to grow the heap by when requesting a new segment
 * @retval AEE_SUCCESS
 *
 */
int HAP_mem_set_grow_size(uint64 min, uint64 max);

/**
 * Set low and high memory thresholds for heap
 *
 * Thresholds must be tuned according to the memory requirements
 *
 * Improper thresholds might led to heap failure
 *
 * @param[in] low_largest_block_size (in bytes) - the heap will grow if size of the largest free block is less than this threshold.
 *                                Currently, setting this parameter will have no impact on the heap.
 * @param[in] high_largest_block_size (in bytes) - the heap manager will release all unused sections if size of the largest free block is greater than this threshold.
 *                                 The recommended value for this, is the size of largest single allocation possible in your application.
 * @return  AEE_SUCCESS on success
 *         AEE_EBADPARM on failure
 */
int HAP_mem_set_heap_thresholds(unsigned int low_largest_block_size, unsigned int high_largest_block_size);


/**
 * Map buffer associated with the file descriptor to DSP memory. The reference
 * count gets incremented if the file descriptor is already mapped. This API is
 * limited to buffer size less then 2 GB. Recommendation is to use HAP_mmap2 for
 * buffer of size > 2 power(8*sizeof(size_t))
 *
 * @param[in] addr mapping at fixed address, not supported currently. This has to be set to NULL
 * @param[in] len size of the buffer to be mapped
 * @param[in] prot protection flags - supported are only HAP_PROT_READ and HAP_PROT_WRITE. HAP_PROT_EXEC is not supported
 * @param[in] flags HAP_MAP_NO_MAP - Increment reference count with no mapping
 *               0 - map the buffer and increment the reference count
 * @param[in] fd file descriptor for the buffer
 * @param[in] offset offset into the buffer
 * @retval  mapped address
 *          -1 on failure
 */
void* HAP_mmap(void *addr, int len, int prot, int flags, int fd, long offset);

/**
 * Map buffer associated with the file descriptor to DSP memory. The reference
 * count gets incremented if the file descriptor is already mapped.
 *
 * @param[in] addr mapping at fixed address, not supported currently. This has to be set to NULL
 * @param[in] len size of the buffer to be mapped
 * @param[in] prot protection flags - supported are only HAP_PROT_READ and HAP_PROT_WRITE. HAP_PROT_EXEC is not supported
 * @param[in] flags HAP_MAP_NO_MAP - Increment reference count with no mapping
 *               0 - map the buffer and increment the reference count
 * @param[in] fd file descriptor for the buffer
 * @param[in] offset offset into the buffer
 * @retval  mapped address
 *          -1 on failure
 */
void* HAP_mmap2(void *addr, size_t len, int prot, int flags, int fd, long offset);

/**
 * Decrements the reference count and unmaps the buffer from memory if the reference count goes to 0.
 * This API is used for buffer size less then 2 GB. Recommendation is to use HAP_munmap2 for buffer of
 * size > 2 power(8*sizeof(size_t)).
 *
 * @param[in] addr mapped address
 * @param[in] len size of the mapped buffer
 * @return  0 on success
 *         AEE_NOSUCHMAP in input addr is invalid
 */
int HAP_munmap(void *addr, int len);

/**
 * Decrements the reference count and unmaps the buffer from memory if the reference count goes to 0.
 *
 * @param[in] addr mapped address
 * @param[in] len size of the mapped buffer
 * @return  0 on success
 *         AEE_NOSUCHMAP in input addr is invalid
 */
int HAP_munmap2(void *addr, size_t len);

/**
 * Get virtual and physical address associated with the buffer and increments
 * the reference count.
 *
 * @param[in] fd file descriptor for the buffer
 * @param[out] vaddr virtual address associated with the buffer
 * @param[out] paddr physical address associated with the buffer
 * @retval 0 on success
 *          AEE_ENOSUCHMAP if fd is invalid
 */
int HAP_mmap_get(int fd, void **vaddr, uint64 *paddr);

/**
 * Decrements the reference count of the file descriptor.
 *
 *@param[in] fd file descriptor of the buffer
 *@retval 0 on success
 *         AEE_ENOSUCHMAP if fd is invalid
 *         AEE_EBADMAPREFCNT if map refcount is <=0
 */
int HAP_mmap_put(int fd);

/**
 * Get the stack size (in bytes) available for current thread
 * Supported only on Lahaina and Cedros
 * @return  available stack for current thread, on success
 *          AEE_EINVALIDTHREAD if unable to get current thread id
 *          AEE_ERESOURCENOTFOUND if unable to get stack for current thread
 */
uint64 HAP_mem_available_stack(void);

/**
 * Allocate and map APPS memory from DSP
 *
 * Usage of this API over malloc() is recommended when client wants greater control over DSP virtual address space
 * as free() does not necessarily free the allocated memory depending on heap thresholds.
 * HAP_apps_mem_request and HAP_apps_mem_release guarantee freeing of the allocated memory.
 *
 * @param[in] len size of memory to be allocated
 * @param[in] flags Buffer attribute flags HAP_MEM_FLAGS_SKIP_DSP_MAP, HAP_MEM_FLAGS_DSP_MAP or HAP_MEM_FLAGS_EXTENDED_MAP
 * @param[out] fd file descriptor of buffer
 * @param[out] dsp_va DSP mapped virtual address
 * @return 0 on success
 */
int HAP_apps_mem_request(size_t len, uint32_t flags, int *fd, uint64_t *dsp_va);

/**
 * Release previously allocated APPS memory from DSP.
 * Releases memory from HLOS. Also unmaps memory from DSP
 * if HAP_MEM_FLAGS_DSP_MAP was previously passed while
 * requesting memory.
 *
 * @param[in] fd previously returned file descriptor of buffer
 * @return 0 on success
 */
int HAP_apps_mem_release(int fd);

#ifdef __cplusplus
}
#endif

#endif // HAP_MEM_H

