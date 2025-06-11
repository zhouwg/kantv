/*-----------------------------------------------------------------------------
 * Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
-----------------------------------------------------------------------------*/

#ifndef HAP_VTCM_MGR_H_
#define HAP_VTCM_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif


void* __attribute__((weak)) HAP_request_async_VTCM(unsigned int size, unsigned int single_page_flag, unsigned int timeout_us);

/**
 * @defgroup vtcmapi HAP VTCM manager API.
 * This section describes the HAP VTCM manager API to allocate and release VTCM.
 * @{
 */

/**
 *  @file HAP_vtcm_mgr.h
 *  @brief APIs used to allocate, release, and query Vector TCM (VTCM) memory.
 *         VTCM is a high-performance, tightly-coupled memory in the cDSP
 *         subsystem. It can used for Hexagon Vector eXtensions (HVX)
 *         scatter/gather instructions, the Hexagon Matrix eXtension (HMX) engine
 *         (available in some cDSPs starting with Lahaina), or as high-performance
 *         scratch memory for other HVX workloads.
 */

/**
 * Request VTCM memory of a specified size and single page requirement.
 *
 * @param[in] size  Size of the request in bytes. \n
 *                  If (@p single_page_flag == 0), the size is aligned to 4 KB. \n
 *                  If (@p single_page_flag == 1), the size is aligned to
 *                      the closest possible page size: 4 KB, 16 KB, 64 KB, 256 KB,
 *                      1 MB, 4 MB, 16 MB.
 * @param[in] single_page_flag  Single page requirement for this allocation:
 *                              1 for single page requests, 0 otherwise.
 *                              Single page requests are mandatory for
 *                              scatter/gather operations because the operations
 *                              must be contained within a single page of memory.
 *                              (The memory region used by scatter/gather
 *                              HVX instructions must reside in VTCM and cannot
 *                              cross a page boundary).
 *
 * @return
 * @c void* pointer to the allocated memory region on success. \n
 * 0 on failure.
 *
 * @par Example
 * @code
 *   // Request for a single page of 4000 bytes
 *  void *pVTCM = HAP_request_VTCM(4000, 1);
 *  if (0 != pVTCM)
 *  {
 *      // Allocation is successful. Try a release
 *      int result = HAP_release_VTCM(pVTCM);
 *      if (0 == result)
 *      {
 *          //Release successful
 *      }
 *  }
 * @endcode
 */
void* HAP_request_VTCM(unsigned int size, unsigned int single_page_flag);

 /**
 * Request VTCM memory of a specified size and single page requirement with a
 * timeout option.
 *
 * This API can be used to wait for the provided timeout. The calling thread is
 * suspended until the requested VTCM memory is available or until the timeout,
 * whichever happens first.
 *
 * @b NOTE: A deadlock might occur when calling this API if the same
 *          thread holds a part of, or the entire VTCM memory prior to this call.
 *          This API is @a not supported from secure and CPZ PDs.
 *
 * @param[in] size  Size of the request in bytes. \n
 *                  If (@p single_page_flag == 0), the size is aligned to 4 KB. \n
 *                  If (@p single_page_flag == 1), the size is aligned to
 *                      the closest possible page size,: 4 KB, 16 KB, 64 KB, 256 KB,
 *                      1 MB, 4 MB, 16 MB
 * @param[in] single_page_flag  Single page requirement for this allocation:
 *                              1 for single page requests, 0 otherwise.
 *                              Single page requests are mandatory for
 *                              scatter/gather operations because the operations
 *                              must be contained within a single page of memory.
 *                              (The memory region used by scatter/gather
 *                              instructions must reside in VTCM and cannot
 *                              cross a page boundary).
 * @param[in] timeout_us  Timeout in microseconds. If the request is readily
 *                        available, return success with a void pointer. If the
 *                        request cannot be served, wait for the available VTCM
 *                        memory until the timeout, or return failure on the
 *                        timeout. This value must be greater than 200 for the
 *                        timeout implementation to work; otherwise, it is treated
 *                        like HAP_request_VTCM().
 *
 * @return
 * @c void* pointer to the allocated memory region on success. \n
 * 0 on failure.
 *
 * @par Example
 * @code
 *  // Request for a single page of 256 * 1024  bytes with
 *  // timeout set to 5 milliseconds
 *  void *pVTCM = HAP_request_async_VTCM(256 * 1024, 1, 5000);
 *  if (0 != pVTCM)
 *  {
 *      // Allocation is successful. Try a release
 *      int result = HAP_release_VTCM(pVTCM);
 *      if (0 == result)
 *      {
 *          //Release successful
 *      }
 *  }
 * @endcode
 */
void* HAP_request_async_VTCM(unsigned int size,
                             unsigned int single_page_flag,
                             unsigned int timeout_us);

/**
 * Release a successful request for VTCM memory by providing the pointer
 * to the previously allocated VTCM block.
 *
 * @param[in] pVA  Pointer returned by a successful VTCM request call.
 *
 * @return
 * @c int 0 on success. \n
 * Non-zero on failure.
 */
int HAP_release_VTCM(void* pVA);

/**
 * Query for the VTCM size defined on target.
 *
 * @param[out] page_size  Pointer to an @c unsigned @c int variable.
 *                        If this parameter is non-zero on success, the memory
 *                        location contains the maximum possible page size
 *                        allocation (in bytes) in VTCM.
 * @param[out] page_count  Pointer to an @c unsigned @c int variable.
 *                         If @p page_size is non-zero on success, the memory
 *                         location contains the number of @p page_size
 *                         blocks in VTCM.
 *
 * @return
 * @c int 0 on success. \n
 * Non-zero on failure.
 *
 * @par Example
 * @code
 *  unsigned int page_size, page_count;
 *  if (0 == HAP_query_total_VTCM(&page_size, &page_count))
 *  {
 *      // Query successful.
 *      // For SM8150 cDSP:
 *      //      page_size will be 256 * 1024.
 *      //      page_count will be 1.
 *      // VTCM memory defined for this chipset (256 KB)
 *      unsigned int total_vtcm = page_size * page_count;
 *  }
 * @endcode
 */
int HAP_query_total_VTCM(unsigned int* page_size, unsigned int* page_count);

/**
 * API to query VTCM allocation status.
 *
 * @param[out] avail_block_size  Pointer to an @c unsigned @c int variable.
 *                               If this parameter is non-zero on success, the
 *                               memory location contains the maximum contiguous
 *                               memory chunk (in bytes) available in VTCM.
 * @param[out] max_page_size  Pointer to an @c unsigned @c int variable.
 *                            If this parameter is non-zero, the memory location
 *                            contains the maximum possible page size allocation
 *                            (in bytes) in the available portion of VTCM.
 * @param[out] num_pages  Pointer to an @c unsigned @c int variable.
 *                        If this parameter is non-zero on success, the memory
 *                        location contains the value of @p max_page_size.
 *
 * @return
 * @c int 0 on success. \n
 * Non-zero on failure.
 *
 * @par Example
 * @code
 *  unsigned int avail_block_size, max_page_size, num_pages;
 *  if (0 == HAP_query_avail_VTCM(&avail_block_size, &max_page_size, &num_pages))
 *  {
 *      // Query successful.
 *      // Use avail_block_size, max_page_size, num_pages
 *  }
 * @endcode
 */
int HAP_query_avail_VTCM(unsigned int* avail_block_size,
                          unsigned int* max_page_size,
                          unsigned int* num_pages);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif //HAP_VTCM_MGR_H_
