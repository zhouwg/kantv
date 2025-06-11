/*-----------------------------------------------------------------------------
   Copyright (c) 2019-2020-2022,2024 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------------*/

#ifndef HAP_COMPUTE_RES_H_
#define HAP_COMPUTE_RES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup types Macros and structures
 * @{
 */

/** Error code for unsupported features. */
#define HAP_COMPUTE_RES_NOT_SUPPORTED                           0x80000404
/** Maximum thread identifiers supported */
#define HAP_COMPUTE_RES_MAX_NUM_THREADS                         16

/**
 *  @file HAP_compute_res.h
 *  @brief Header file with APIs to allocate compute resources.
 */

/**
 * Structure containing attributes for compute resources.
 */
typedef struct {
    unsigned long long attributes[8]; /**< Attribute array. */
} compute_res_attr_t;

/**
 * Structure containing a VTCM page size and the number of pages with that size.
 */
typedef struct {
    unsigned int page_size;  /**< Page size in bytes. */
    unsigned int num_pages;  /**< Number of pages of size page_size. */
} compute_res_vtcm_page_def_t;

/**
 * Structure describing the VTCM memory pages.
 */
typedef struct {
    unsigned int block_size;           /**< Block size in bytes */
    unsigned int page_list_len;      /**< Number of valid elements in page_list array */
    compute_res_vtcm_page_def_t page_list[8];  /**< Array of pages. */
} compute_res_vtcm_page_t;

/**
 * enum of HMX lock tyes
 */
typedef enum {
    HAP_COMPUTE_RES_HMX_NON_SHARED = 0,      /**< No sharing of HMX across threads */
    HAP_COMPUTE_RES_HMX_SHARED = 1,          /**< To share HMX across threads */
} compute_res_hmx_type_t;

/**
 * enum of capabilities supported by capability query API
 */
typedef enum {
    HAP_COMPUTE_RES_PREEMPTION_CAPABILITY = 1,  /**< Preemption capability */
} compute_res_capability_id;

/**
 * Masks returned by preemption capability query
 */
#define HAP_COMPUTE_RES_COOPERATIVE_PREEMPTION                  1
/**< Mask indicating support for cooperative preemption framework using
 *   capabilities query. The cooperative preemption framework involves applications
 *   registering a release callback for accepting yield requests from a high priority
 *   allocator.
 */
#define HAP_COMPUTE_RES_AUTONOMOUS_PREEMPTION                   2
/**< Mask indicating support for autonomous/optimized preemption framework using
 *   capabilities query. HMX resource management is moved out of #HAP_compute_res_acquire()/
 *   #HAP_compute_res_acquire_cached(), instead applications use #HAP_compute_res_hmx_lock3()/
 *   #HAP_compute_res_hmx_unlock3() to lock/unlock HMX resource directly from the threads
 *   using HMX. Applications shall implement HMX critical section using hmx_mutex object
 *   returned by #HAP_compute_res_hmx_lock3() around non-preemptable HMX sections.
 */
#define HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION       4
/**< Mask indicating support for thread identifiers based autonomous/optimized
 *   preemption framework. This feature is a subset of HAP_COMPUTE_RES_AUTONOMOUS_PREEMPTION.
 *   In this feature, applications register the threads that
 *   will be working on the allocated compute resources with the resource manager.
 *   The compute resource manager, as part of autonomous preemption, suspends the
 *   threads associated with the low priority context when a high priority thread
 *   requests for these resources.
 */


/**
 * enum of commands for providing thread ids to the resource manager
 */
typedef enum {
    HAP_COMPUTE_RES_THREADS_OVERRIDE = 1,
    /**< Command ID to override the thread list registered with a context */
    HAP_COMPUTE_RES_THREADS_APPEND = 2,
    /**< Command ID to append to an existing thread list associated with
     * the context
     */
    HAP_COMPUTE_RES_THREADS_REMOVE = 3,
    /**< Command ID to remove a thread from an existing thread list associated
     * with the context
     */
} compute_res_threads_cmd_id;

/**
 * Structure holding HMX critical section parameters
 */
typedef struct {
    void *mutex;
    /**< Mutex to be used for entering/exiting HMX critical section
     *   via lock and unlock functions
     */
    void (*lock)(void *mutex);
    /**< Lock function to be called for entering HMX critical section using
     *   mutex as argument
     */
    void (*unlock)(void *mutex);
    /**< Unlock function to be called for exiting HMX critical section using
     *   mutex as argument
     */
} compute_res_hmx_mutex_t;

/**
 * Structure for querying preemption data
 */
typedef struct {
    unsigned int num_preemptions;
    /**< Number of preemptions on the acquired context */
    unsigned long long preempted_duration;
    /**< Total duration the context remained preempted in terms of 19.2MHz ticks */
    unsigned long long preemption_overhead;
    /**< Total preemption overhead in terms of 19.2MHz ticks */
} compute_res_preempt_data_t;

/**
 * @}
 */

/**
 * @cond DEV
 */
int __attribute__((weak)) compute_resource_attr_init(
                                        compute_res_attr_t* attr);

int __attribute__((weak)) compute_resource_attr_set_serialize(
                                        compute_res_attr_t* attr,
                                        unsigned char b_enable);

int __attribute__((weak)) compute_resource_attr_set_hmx_param(
                                        compute_res_attr_t* attr,
                                        unsigned char b_enable);

int __attribute__((weak)) compute_resource_attr_set_vtcm_param(
                                        compute_res_attr_t* attr,
                                        unsigned int vtcm_size,
                                        unsigned char b_single_page);

int __attribute__((weak)) compute_resource_attr_set_vtcm_param_v2(
                                        compute_res_attr_t* attr,
                                        unsigned int vtcm_size,
                                        unsigned int min_page_size,
                                        unsigned int min_vtcm_size);

int __attribute__((weak)) compute_resource_attr_set_app_type(
                                        compute_res_attr_t* attr,
                                        unsigned int application_id);

int __attribute__((weak)) compute_resource_attr_set_cache_mode(
                                        compute_res_attr_t* attr,
                                        unsigned char b_enable);

int __attribute__((weak)) compute_resource_attr_set_release_callback(
                                        compute_res_attr_t* attr,
                                        int (*release_callback)(
                                            unsigned int context_id,
                                            void* client_context),
                                        void* client_context);

void* __attribute__((weak)) compute_resource_attr_get_vtcm_ptr(
                                        compute_res_attr_t* attr);

int __attribute__((weak)) compute_resource_attr_get_vtcm_ptr_v2(
                                        compute_res_attr_t* attr,
                                        void** vtcm_ptr,
                                        unsigned int* vtcm_size);

int __attribute__((weak)) compute_resource_query_VTCM(
                                unsigned int application_id,
                                unsigned int* total_block_size,
                                compute_res_vtcm_page_t* total_block_layout,
                                unsigned int* avail_block_size,
                                compute_res_vtcm_page_t* avail_block_layout);

unsigned int __attribute__((weak)) compute_resource_acquire(
                                        compute_res_attr_t* attr,
                                        unsigned int timeout_us);

int __attribute__((weak)) compute_resource_release(
                                        unsigned int context_id);

int __attribute__((weak)) compute_resource_acquire_cached(
                                        unsigned int context_id,
                                        unsigned int timeout_us);

int __attribute__((weak)) compute_resource_release_cached(
                                        unsigned int context_id);

int __attribute__((weak)) compute_resource_hmx_lock(
                                        unsigned int context_id);

int __attribute__((weak)) compute_resource_hmx_unlock(
                                        unsigned int context_id);

int __attribute__((weak)) compute_resource_check_release_request(
                                        unsigned int context_id);

int __attribute__((weak)) compute_resource_hmx_lock2(
                                        unsigned int context_id,
                                        compute_res_hmx_type_t type);

int __attribute__((weak)) compute_resource_hmx_unlock2(
                                        unsigned int context_id,
                                        compute_res_hmx_type_t type);

int __attribute__((weak)) compute_resource_update_priority(
                                        unsigned int context_id,
                                        unsigned short priority);

int __attribute__((weak)) crm_hmx_lock3(unsigned int context_id,
                                        compute_res_hmx_type_t type,
                                        compute_res_hmx_mutex_t *mutex,
                                        unsigned int timeout_us);

int __attribute__((weak)) crm_hmx_unlock3(unsigned int context_id,
                                          compute_res_hmx_type_t type,
                                          compute_res_hmx_mutex_t *mutex);

int __attribute__ ((weak)) crm_attr_set_vtcm_backup(
                                        compute_res_attr_t* attr,
                                        void *buffer,
                                        unsigned int buffer_size);

int __attribute__ ((weak)) crm_attr_set_threads(
                                        compute_res_attr_t* attr,
                                        unsigned int *threads,
                                        unsigned int num_threads);

int __attribute__ ((weak)) crm_attr_set_vtcm_clear_on_release(
                                        compute_res_attr_t* attr,
                                        unsigned char enable);

int __attribute__ ((weak)) crm_cached_set_threads(compute_res_threads_cmd_id command,
                                                  unsigned int context_id,
                                                  unsigned int *threads,
                                                  unsigned int num_threads);

int __attribute__((weak)) crm_query_capability(compute_res_capability_id capability_id,
                                               unsigned int* data);

int __attribute__((weak)) crm_get_preempt_data(unsigned int context_id,
                                                 compute_res_preempt_data_t *data);

int __attribute__((weak)) crm_tid_preemption_lock(void);

int __attribute__((weak)) crm_tid_preemption_unlock(void);
/**
 * @endcond
 */

/**
 * @defgroup attributes Manage attributes
 * Manage parameters affecting the requested shared resources
 * @{
 */

/**
 * Initializes the attribute structure for a resource request.
 *
 * The user must call this function before setting any specific resource property
 * via other helper functions.
 *
 * @param[in] attr Pointer to compute resource attribute structure,
 *                 #compute_res_attr_t.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED if unsupported.
 */
static inline int HAP_compute_res_attr_init(compute_res_attr_t* attr)
{
    if (compute_resource_attr_init)
        return compute_resource_attr_init(attr);

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets or clears the serialization option in the request resource structure.
 *
 * Serialization allows participating use cases to run with mutually exclusive
 * access to the entire cDSP which helps, for example, in avoiding cache
 * thrashing while trying to run simultaneously on different hardware threads.
 * Participating use cases issue blocking acquires on the serialization
 * resource when ready to run, and each runs in turn when it is granted that
 * resource.
 *
 * Acquiring the serialization resource only ensures
 * mutual exclusion from other cooperating use cases that also block on
 * acquisition of that resource, it does not guarantee exclusion from
 * concurrent use cases that do not request the serialization
 * resource.
 *
 * @param[in] attr Pointer to the compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] b_serialize 1 (TRUE) to participate in serialization. \n
 *                        0 (FALSE) otherwise.
 *
 * @return
 * 0 upon success \n
 * Nonzero upon failure.
 */
static inline int HAP_compute_res_attr_set_serialize(
                                                compute_res_attr_t* attr,
                                                unsigned char b_serialize)
{
    if (compute_resource_attr_set_serialize)
    {
        return compute_resource_attr_set_serialize(attr,
                                                   b_serialize);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets VTCM request parameters in the provided resource attribute structure.
 *
 * The user calls this function to request the specified VTCM size in the acquire call.
 * These VTCM request attributes are reset to 0 (no VTCM request) in the
 * resource attribute structure by HAP_compute_res_attr_init().
 *
 * @param[in] attr Pointer to compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] vtcm_size Size of the VTCM request in bytes;
                        0 if VTCM allocation is not required.
 * @param[in] b_single_page    1 - Requested VTCM size to be allocated in a
 *                                 single page. \n
 *                             0 - No page requirement (allocation can spread
 *                                 across multiple pages. VTCM manager
 *                                 always attempts the best fit).
 *
 * @return
 * 0 upon success. \n
 * Non-zero upon failure.
 */
static inline int HAP_compute_res_attr_set_vtcm_param(
                                             compute_res_attr_t* attr,
                                             unsigned int vtcm_size,
                                             unsigned char b_single_page)
{
    if (compute_resource_attr_set_vtcm_param)
    {
        return compute_resource_attr_set_vtcm_param(attr,
                                                    vtcm_size,
                                                    b_single_page);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Reads the VTCM memory pointer from the given attribute structure.
 *
 * On a successful VTCM resource request placed via #HAP_compute_res_acquire()
 * using HAP_compute_res_attr_set_vtcm_param(), a user can invoke this helper
 * function to retrieve the allocated VTCM address by passing the same attribute
 * structure used in the respective HAP_compute_res_acquire() call.
 *
 * @param[in] attr Pointer to compute the resource attribute structure
 *                 #compute_res_attr_t.
 *
 * @return
 * Void pointer to the allocated VTCM section. \n
 * 0 signifies no allocation.
 */
static inline void* HAP_compute_res_attr_get_vtcm_ptr(compute_res_attr_t* attr)
{
    if (compute_resource_attr_get_vtcm_ptr)
    {
        return compute_resource_attr_get_vtcm_ptr(attr);
    }

    return 0;
}

/**
 * Sets an extended set of VTCM request parameters in the attribute structure,
 * specifically VTCM Size, the minimum required page size, and the minimum
 * required VTCM size.
 *
 * This function cannot be used with HAP_compute_res_attr_set_vtcm_param().
 * Call this function after HAP_compute_res_attr_init().
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute the resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] vtcm_size Size of the VTCM request in bytes. 0 if VTCM allocation
 *                      is NOT required.
 * @param[in] min_page_size Minimum page size required in bytes. Valid pages include
 *                          4 KB, 16 KB, 64 KB, 256 KB, 1 MB, 4 MB, 16 MB. Setting 0
 *                          will select best possible fit (least page mappings)
 * @param[in] min_vtcm_size Minimum VTCM size in bytes, if the specified size
 *                          (vtcm_size) is not available. 0 means the
 *                          size is an absolute requirement.
 *
 * @return
 * 0 for success. \n
 * Non-zero for failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_vtcm_param_v2(
                                             compute_res_attr_t* attr,
                                             unsigned int vtcm_size,
                                             unsigned int min_page_size,
                                             unsigned int min_vtcm_size)
{
    if (compute_resource_attr_set_vtcm_param_v2)
    {
        return compute_resource_attr_set_vtcm_param_v2(attr,
                                                       vtcm_size,
                                                       min_page_size,
                                                       min_vtcm_size);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets VTCM backup buffer in the provided attribute structure.
 *
 * Compute resource manager uses the provided buffer to backup VTCM allocated
 * to the user during preemption of the associated request/context. The backup
 * buffer provided should be able to accomodate all of the requested VTCM size.
 * VTCM backup buffer is essential for preemption to work on architectures
 * supporting HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION (use
 * HAP_compute_res_query_capability() to query preemption model supported)
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * @param[in] attr Pointer to the compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] buffer Pointer to the backup buffer in main memory (DDR). To be
 *                   used by the compute resource manager for saving/restoring
 *                   user allocated VTCM region during preemption.
 * @param[in] buffer_size Size of the backup buffer in main memory (DDR) pointed
 *                        to by the #buffer argument. The provided buffer should
 *                        be sufficiently sized to accommodate user requested
 *                        VTCM size. Align the buffer to 128B for better performance.
 *
 * @return
 * 0 for success. \n
 * Non-zero for failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_vtcm_backup(
                                            compute_res_attr_t* attr,
                                            void *buffer,
                                            unsigned int buffer_size)
{
    if (crm_attr_set_vtcm_backup)
    {
        return crm_attr_set_vtcm_backup(attr, buffer, buffer_size);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Updates provided attribute structure with user-provided thread id array.
 *
 * On architectures supporting HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION,
 * Compute resource manager requires users to register the threads that will be
 * using the compute resources requested via #HAP_compute_res_acquire().
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * @param[in] attr Pointer to the compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] threads Pointer to an array of QuRT thread identifiers associated
 *                    with the resource request. This array should be valid
 *                    till #HAP_compute_res_acquire() is called on the prepared
 *                    attribute.
 * @param[in] num_threads Number of QuRT thread identifiers in the provided
 *                        threads array #threads. A maximum of
 *                        HAP_COMPUTE_RES_MAX_NUM_THREADS
 *                        number of threads can be provided.
 *
 * @return
 * 0 for success. \n
 * Non-zero for failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_threads(
                                            compute_res_attr_t* attr,
                                            unsigned int *threads,
                                            unsigned int num_threads)
{
    if (crm_attr_set_threads)
    {
        return crm_attr_set_threads(attr, threads, num_threads);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Updates thread id array for the associated cached context.
 *
 * Compute resource manager uses the QuRT thread identifiers provided by the
 * user during preemption of the associated context. For cached
 * allocations, the thread identifiers can either be provided at the time
 * of HAP_compute_res_acquire() call using #HAP_compute_res_attr_set_threads(),
 * or using this API with the context_id returned by a successful
 * #HAP_compute_res_acquire() call when the cached attribute is set via
 * #HAP_compute_res_attr_set_cache_mode().
 * The API has to be called before HAP_compute_res_acquire_cached() call.
 *
 * @param[in] command specifies a command from compute_res_threads_cmd_id:
 *                    HAP_COMPUTE_RES_THREADS_OVERRIDE : To provide a new
 *                                                  set of threads.
 *                    HAP_COMPUTE_RES_THREADS_APPEND : To append to previously
 *                                                  provided list of threads.
 *                    HAP_COMPUTE_RES_THREADS_REMOVE : To remove given threads
 *                                                  from previoulsy provided
 *                                                  list of threads.
 * @param[in] context_id Context ID returned by HAP_compute_res_acquire().
 * @param[in] threads Pointer to an array of QuRT thread identifiers associated
 *                    with the resource request.
 * @param[in] num_threads Number of QuRT thread identifiers in the provided
 *                        threads array #threads. A maximum of
 *                        HAP_COMPUTE_RES_MAX_NUM_THREADS
 *                        number of threads can be provided.
 *
 * @return
 * 0 for success. \n
 * Non-zero for failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_cached_set_threads(compute_res_threads_cmd_id command,
                                                     unsigned int context_id,
                                                     unsigned int *threads,
                                                     unsigned int num_threads)
{
    if (crm_cached_set_threads)
    {
        return crm_cached_set_threads(command, context_id, threads, num_threads);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets VTCM clear on release option in the provided attribute structure.
 *
 * The compute resource manager by default initializes the VTCM memory to 0
 * when VTCM is released by the caller either at the time of release or when
 * it's allocated to another process. For performance considerations (also
 * considering security implications if any), client can intimate the compute
 * resource manager not to clear out (zero-initialize) the allocated VTCM
 * on release.
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * @param[in] attr Pointer to the compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] enable  1 - zero-initialize VTCM memory after release (default)
 *                     0 - Do not zero-initialize VTCM memory after release.
 * @return
 * 0 for success. \n
 * Non-zero for failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_vtcm_clear_on_release(
                                            compute_res_attr_t* attr,
                                            unsigned char enable)
{
    if (crm_attr_set_vtcm_clear_on_release)
    {
        return crm_attr_set_vtcm_clear_on_release(attr, enable);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * On a successful VTCM resource request placed via
 * HAP_compute_res_acquire() or HAP_compute_res_acquire_cached() using
 * HAP_compute_res_attr_set_vtcm_param_v2(), users invoke this helper function
 * to retrieve the allocated VTCM address and size by passing the same
 * attribute structure used in the respective acquire call.
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute the resource attribute structure
 *                 #compute_res_attr_t.
 * @param[out] vtcm_ptr Assigned VTCM address; NULL for no allocation.
 * @param[out] vtcm_size Size of the allocated VTCM memory from the assigned pointer.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_get_vtcm_ptr_v2(
                                        compute_res_attr_t* attr,
                                        void** vtcm_ptr,
                                        unsigned int* vtcm_size)
{
    if (compute_resource_attr_get_vtcm_ptr_v2)
    {
        return compute_resource_attr_get_vtcm_ptr_v2(attr,
                                                     vtcm_ptr,
                                                     vtcm_size);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * On chipsets with HMX, sets/resets the HMX request parameter in the attribute
 * structure for acquiring the HMX resource.
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute the resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] b_enable 0 - do not request HMX resource (resets option). \n
 *                     1 - request HMX resource (sets option).
 * @return
 * 0 upon success. \n
 * Nonzero upon failure.
 */
static inline int HAP_compute_res_attr_set_hmx_param(
                                                compute_res_attr_t* attr,
                                                unsigned char b_enable)
{
    if (compute_resource_attr_set_hmx_param)
    {
        return compute_resource_attr_set_hmx_param(attr,
                                                   b_enable);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets or resets cacheable mode in the attribute structure.
 *
 * A cacheable request allows users to allocate and release based on the
 * context ID of the request. On a successful cacheable request via
 * HAP_compute_res_acquire(), users get the same VTCM address and
 * size across calls of HAP_compute_res_acquire_cached() and
 * HAP_compute_res_release_cached() until the context is explicitly
 * released via HAP_compute_res_release().
 *
 * After a successful cacheable request via HAP_compute_res_acquire(),
 * users can get the assigned VTCM pointer (if requested) by passing
 * the attribute structure to HAP_compute_res_attr_get_vtcm_ptr()
 * for v1 and HAP_compute_res_attr_get_vtcm_ptr_v2() for v2,
 * and they must call HAP_compute_res_acquire_cached() before using the
 * assigned resources.
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] b_enable  0 - Do not request cacheable mode (resets option). \n
 *                      1 - Request cacheable mode (sets option).
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure.\n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_cache_mode(
                                              compute_res_attr_t* attr,
                                              unsigned char b_enable)
{
    if (compute_resource_attr_set_cache_mode)
    {
        return compute_resource_attr_set_cache_mode(attr,
                                                    b_enable);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets the application ID parameter in the resource structure used to
 * select the appropriate VTCM partition.
 *
 * If this application ID parameter is not explicitly set, the default partition is selected.
 * The default application ID (0) is set when the attribute structure is initialized.
 * Application IDs are defined in the kernel device tree configuration.
 * If the given ID is not specified in the tree, the primary VTCM partition is selected.
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute the resource attribute structure
 *                 #compute_res_attr_t.
 * @param[in] application_id Application ID used to specify the VTCM partition.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure.
 */
static inline int HAP_compute_res_attr_set_app_type(
                                              compute_res_attr_t* attr,
                                              unsigned int application_id)
{
    if (compute_resource_attr_set_app_type)
    {
        return compute_resource_attr_set_app_type(attr,
                                                  application_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */


/**
* @defgroup query VTCM query API
* @{
*/

/**
 * Returns the total and available VTCM sizes and page layouts
 * for the given application type.
 *
 * Supported starting with Lahaina.
 *
 * @param[in] application_id Application ID used to specify the VTCM partition.
 * @param[out] total_block_size Total VTCM size assigned for this application type.
 * @param[out] total_block_layout Total VTCM size (total_block_size)
 *                                represented in pages.
 * @param[out] avail_block_size Largest contiguous memory chunk available in
                                VTCM for this application type.
 * @param[out] avail_block_layout Available block size (avail_block_size)
 *                                represented in pages.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_query_VTCM(
                                unsigned int application_id,
                                unsigned int* total_block_size,
                                compute_res_vtcm_page_t* total_block_layout,
                                unsigned int* avail_block_size,
                                compute_res_vtcm_page_t* avail_block_layout)
{
    if (compute_resource_query_VTCM)
    {
        return compute_resource_query_VTCM(application_id,
                                           total_block_size,
                                           total_block_layout,
                                           avail_block_size,
                                           avail_block_layout);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */

/**
 * @defgroup acquire_release Acquire and release
 * Manage the process of resource acquisition and release
 * @{
 */

/**
 * Checks the release request status for the provided context.
 * When a context is acquired by providing a release callback, the callback
 * can be invoked by the compute resource manager when a high priority client
 * is waiting for the resource(s). If a client defers a release request waiting
 * for an outstanding work item, this API can be used to check if a release is
 * still required before releasing the context once the work is done.
 *
 * Note: It is not mandatory to call this API once a release request via
 * the registered callback is received. The context can be released and reacquired
 * if necessary. This API can be useful to avoid a release and reacquire in cases
 * where the high priority client times out and is no longer waiting for the
 * resource(s).
 *
 * Supported starting with Lahaina.
 *
 * @param[in] context_id  Context ID returned by HAP_compute_res_acquire call().
 *
 * @return
 * 0 if the provided context need not be released. \n
 * Nonzero up on failure or if the context needs to be released. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported. \n
 */
static inline int HAP_compute_res_check_release_request(
                                                    unsigned int context_id)
{
    if (compute_resource_check_release_request)
    {
        return compute_resource_check_release_request(context_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Accepts a prepared attribute structure (attr) and returns a context ID
 * for a successful request within the provided timeout (microseconds).
 *
 * @param[in] attr Pointer to compute the resource attribute structure
 *                 #compute_res_attr_t.
 * @param[in] timeout_us Timeout in microseconds; 0 specifies no timeout
 *                       i.e., requests with unavailable resources
 *                       immediately return failure. If nonzero, should
 *                       be at least 200.
  *
 * @return
 * Nonzero context ID upon success. \n
 * 0 upon failure (i.e., unable to acquire requested resource
 * in a given timeout duration).
 */
static inline unsigned int HAP_compute_res_acquire(
                                              compute_res_attr_t* attr,
                                              unsigned int timeout_us)
{
    if (compute_resource_acquire)
    {
        return compute_resource_acquire(attr, timeout_us);
    }

    return 0;
}

/**
 * Releases all the resources linked to the given context ID.
 *
 * Call this function with the context_id returned by a successful
 * HAP_compute_res_acquire call().
 *
 * @param[in] context_id Context ID returned by HAP_compute_res_acquire call().
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_release(unsigned int context_id)
{
    if (compute_resource_release)
    {
        return compute_resource_release(context_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Acquires or reacquires the resources pointed to the context_id returned by
 * a successful HAP_compute_res_acquire() call. If VTCM resource was requested,
 * the VTCM address, size, and page configuration remain the same.
 *
 * Supported from Lahaina.
 *
 * @param[in] context_id Context ID returned by HAP_compute_res_acquire().
 * @param[in] timeout_us Timeout in microseconds; 0 specifies no timeout
 *                       i.e., requests with unavailable resources
 *                       immediately return failure. If nonzero, should
 *                       be at least 200.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_acquire_cached(
                                              unsigned int context_id,
                                              unsigned int timeout_us)
{
    if (compute_resource_acquire_cached)
    {
        return compute_resource_acquire_cached(context_id, timeout_us);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Releases all the resources pointed to by the context_id acquired
 * by a successful HAP_compute_res_acquire_cached() call, while allowing the
 * user to reacquire the same resources via HAP_compute_res_acquire_cached()
 * in the future until the context is released via HAP_compute_res_release().
 *
 * Supported starting with Lahaina.
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_release_cached(unsigned int context_id)
{
    if (compute_resource_release_cached)
    {
        return compute_resource_release_cached(context_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Sets the release callback function in the attribute structure.

 * The compute resource manager calls the release_callback function when any of the
 * resources reserved by the specified context are required by a higher priority
 * client. Clients act on the release request by explicitly calling the release
 * function HAP_compute_res_release() or HAP_compute_res_release_cached()
 * to release all acquired resources of the given context_id.
 *
 * Client-provided context (client_context) is passed to the release callback. On
 * receiving a release request via the provided callback, clients should call the
 * release function within 5 milliseconds. The release_callback function
 * should not have any blocking wait.
 *
 * Call this function after HAP_compute_res_attr_init().
 *
 * Supported starting with Lahaina.
 *
 * @param[in] attr Pointer to compute the resource attribute structure,
 *                 #compute_res_attr_t.
 * @param[in] release_callback Function pointer to the registered callback to
                               receive the release request.
 * @param[in] client_context User-provided client context.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_attr_set_release_callback(
                                        compute_res_attr_t* attr,
                                        int (*release_callback)(
                                            unsigned int context_id,
                                            void* client_context),
                                        void* client_context)
{
    if (compute_resource_attr_set_release_callback)
    {
        return compute_resource_attr_set_release_callback(attr,
                                                          release_callback,
                                                          client_context);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Updates the priority of an allocated context reflecting the caller's
 * thread priority.
 * The compute resource manager uses the callers thread priority as the resource
 * priority when acquired (HAP_compute_res_acquire() /
 * HAP_compute_res_acquire_cached()). If the thread priority of the caller is
 * changed after acquiring the resource, caller should intimate the compute
 * resource manager of a priority change by invoking this API. Failing to do
 * so will result in resource manager assuming an incorrect priority for
 * the allocated resource which may result in unwanted release requests.
 * For a cached allocation, this API should be called after a successful
 * HAP_compute_res_acquire_cached() call.
 *
 * Supported on latest chipsets(released after Palima).
 *
 * @param[in] context_id Context ID returned by HAP_compute_res_acquire()..
 * @param[in] priority 0 - The compute resource manager would use the caller
 *                         thread priority
 *                     1..255 - priority value in terms of QuRT thread priority.
 *                              Priority ceiling will be applied for unprivileged
 *                              processes.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_update_priority(unsigned int context_id,
                                                  unsigned short priority)
{
    if (compute_resource_update_priority)
    {
        return compute_resource_update_priority(context_id, priority);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */

/**
 * @defgroup Critical section for autonomous thread id preemption
 *
 * API to enter and exit critical section to prevent autonomous thread identifiers
 * based preemption (HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION) from
 * resource manager when acquiring global mutexes (used
 * in I/O, standard library functions like printf, user implemented
 * serialization etc.)
 *
 * @{
 */

/**
 * API to enter critical section to prevent autonomous thread identifiers
 * based preemption (HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION) from
 * resource manager when acquiring global mutexes (used
 * in I/O, standard library functions like printf, user implemented
 * serialization etc.)
 *
 * On architectures supporting HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION,
 * holding global mutexes can lead to deadlocks within the preempted task's
 * user process. The critical section exposed by this API should be implemented
 * by users around I/O, logging or any standard libraries/user implementations
 * which acquires global mutexes.
 *
 * Implementation uses a per-process global mutex, callers of this API will
 * be serialized across threads within the caller user process on NSP.
 *
 * NOTE: The critical section implementation should only be done when,
 *       - HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION is supported
 *       - Applications with different priorities co-exist in a single user process
 *         exposing the risk of deadlock between a running and preempted
 *         application.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 *
 */

static inline int HAP_compute_res_tid_preemption_lock(void)
{
    if (crm_tid_preemption_lock)
    {
        return crm_tid_preemption_lock();
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Releases the critical section acquired by #HAP_compute_res_tid_preemption_lock().
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */

static inline int HAP_compute_res_tid_preemption_unlock(void)
{
    if (crm_tid_preemption_unlock)
    {
        return crm_tid_preemption_unlock();
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */

/**
 * @defgroup Capability query and profiling data
 * API to query capabilities of the compute resource manager and to get
 * profiling data associated with a context.
 *
 * @{
 */

/**
 * Queries compute resource manager capabilities listed under
 * compute_res_capability_id enum.
 *
 * @param[in] capability_id Identifier from compute_res_capability_id corresponding
 *                          to the compute resource manager capability.
 * @param[out] data Pointer to an unsigned int data. On success, the memory
 *                  is updated with the data associated with the queried capability.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_query_capability(compute_res_capability_id capability_id,
                                                   unsigned int* data)
{
    if (crm_query_capability)
    {
        return crm_query_capability(capability_id, data);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * On implementations supporting HAP_COMPUTE_RES_AUTONOMOUS_PREEMPTION,
 * this API returns preemption statistics associated with the context_id
 * acquired via HAP_compute_res_acquire().
 *
 * This API needs to be called before the associated context is released via
 * HAP_compute_res_release() call, data returned is invalid otherwise.
 *
 * @param[in] context_id Context ID returned by HAP_compute_res_acquire().
 * @param[out] Pointer to compute_res_preempt_data_t.
 *             On success, the preemption-related statistics are updated in
 *             the provided structure.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_get_preempt_data(unsigned int context_id,
                                                   compute_res_preempt_data_t* data)
{
    if (crm_get_preempt_data)
    {
        return crm_get_preempt_data(context_id, data);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */


/**
 * @defgroup HMX HMX lock and unlock
 * Manage HMX lock once HMX has been acquired
 *
 * @{
 */

/**
 * Locks the HMX unit to the current thread and prepares the thread to
 * execute HMX instructions. The client must have already acquired the
 * HMX resource with HAP_compute_res_acquire() or HAP_compute_res_acquire_cached(),
 * and context_id must refer to the corresponding resource manager context.
 *
 * Before executing HMX instructions, a client must call this function from
 * the same software thread used for HMX processing. Only the calling thread
 * with a valid HMX lock may execute HMX instructions.
 *
 * Supported starting with Lahaina.
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_lock(unsigned int context_id)
{
    if (compute_resource_hmx_lock)
    {
        return compute_resource_hmx_lock(context_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * Unlocks the HMX unit from the calling thread. The HMX unit can then be
 * locked to another thread or released with HAP_compute_res_release().
 *
 * This function must be called from the same thread as the previous
 * HMX_compute_res_hmx_lock() call.
 *
 * Supported starting with Lahaina.
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_unlock(unsigned int context_id)
{
    if (compute_resource_hmx_unlock)
    {
        return compute_resource_hmx_unlock(context_id);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * This function is an extension to HAP_compute_res_hmx_lock() with an additional
 * option to lock HMX across multiple participating threads within a user process
 * and timeshare the HMX resource (only one thread should be using HMX at a time).
 *
 * Supported on latest chipsets(released after Palima).
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 * @param[in] tye        HAP_COMPUTE_RES_HMX_NON_SHARED
 *                          Analogous to #HAP_compute_res_hmx_lock()
 *                       HAP_COMPUTE_RES_HMX_SHARED
 *                          Threads within a process can lock and timeshare the same HMX
 *                          resource. When using this option, it is caller's responsibility
 *                          to timeshare HMX (only one thread should use HMX at a time)
 *                          among participating threads using HAP_COMPUTE_RES_HMX_SHARED
 *                          option from the same process.
 *                          Note that the sharing of HMX is allowed between the threads of
 *                          the same user process. A single Context ID (context_id) should be
 *                          used across the participating threads in a user process.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_lock2(unsigned int context_id,
                                            compute_res_hmx_type_t type)
{
    if (compute_resource_hmx_lock2)
    {
        return compute_resource_hmx_lock2(context_id, type);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * To be used in conjunction with HAP_compute_res_hmx_lock2() to release a successfully
 * locked HMX unit.
 * 'type' provided should match with the type provided to a successful
 * HAP_compute_res_hmx_lock2() call from this thread.
 *
 * Supported on latest chipsets(released after Palima).
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 *
 * @param[in] type  Should be the same paramter used to lock HMX
 *                  via #HAP_compute_res_hmx_lock2()
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_unlock2(unsigned int context_id,
                                              compute_res_hmx_type_t type)
{
    if (compute_resource_hmx_unlock2)
    {
        return compute_resource_hmx_unlock2(context_id, type);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */

/**
 * @defgroup HMX HMX lock and unlock
 * Manage HMX on architectures supporting HAP_COMPUTE_RES_AUTONOMOUS_PREEMPTION
 *
 * @{
 */

/**
 * On architectures supporting HAP_COMPUTE_RES_AUTONOMOUS_PREEMPTION preemption,
 * this funciton locks the HMX unit to the current thread and prepares the thread to
 * execute HMX instructions. The client must have already acquired the
 * VTCM using HAP_compute_res_acquire() or HAP_compute_res_acquire_cached(),
 * and context_id must refer to the corresponding resource manager context.
 *
 * Before executing HMX instructions, a client must call this function from
 * the same software thread used for HMX processing. Only the calling thread
 * with a valid HMX lock may execute HMX instructions.
 *
 * The calling thread shall acquire lock on HMX mutex before executing HMX
 * instructions and release the lock when program reaches to a point where the
 * acquired HMX unit can be re-assigned to a higher priority waiter (in case of
 * multiple clients contending for HMX resource) without affecting
 * functionality. For entering HMX critical section, user shall call
 * hmx_mutex->lock(hmx_mutex->mutex). For exiting the HMX critical section, user
 * shall call hmx_mutex->unlock(hmx_mutex->mutex)
 * autonomous preemption will wait for applications to release the HMX critical section
 * before preempting HMX from the allocator.
 *
 * @param[in] context_id Context ID returned by
 *                       #HAP_compute_res_acquire().
 * @param[in] type       HAP_COMPUTE_RES_HMX_NON_SHARED
 *                          Analogous to #HAP_compute_res_hmx_lock()
 *                       HAP_COMPUTE_RES_HMX_SHARED
 *                          Threads within a process can lock and timeshare the same HMX
 *                          resource. When using this option, it is caller's responsibility
 *                          to timeshare HMX (only one thread should use HMX at a time)
 *                          among participating threads using HAP_COMPUTE_RES_HMX_SHARED
 *                          option from the same process.
 *                          Note that the sharing of HMX is allowed between the threads of
 *                          the same user process. A single Context ID (context_id) should be
 *                          used across the participating threads in a user process.
 * @param[out] hmx_mutex  Pointer to structure of type compute_res_hmx_mutex_t.
 *                        On Success, the structure is updated with mutex, lock
 *                        and unlock parameters.
 * @param[in] timeout_us Timeout in microseconds; 0 specifies no timeout
 *                       i.e., requests with unavailable resources
 *                       immediately return failure. If nonzero, should
 *                       be at least 200.
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_lock3(unsigned int context_id,
                                              compute_res_hmx_type_t type,
                                              compute_res_hmx_mutex_t *hmx_mutex,
                                              unsigned int timeout_us)
{
    if (crm_hmx_lock3)
    {
        return crm_hmx_lock3(context_id, type, hmx_mutex, timeout_us);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * To be used in conjunction with HAP_compute_res_hmx_lock3() to release a
 * successfully locked HMX unit.
 * 'type' provided should match with the type provided to a successful
 * #HAP_compute_res_hmx_lock3() call from this thread.
 *
 * @param[in] context_id Context ID returned for a successful VTCM acquisition by
 *                       #HAP_compute_res_acquire().
 *
 * @param[in] type  Should be the same parameter used to lock HMX
 *                  via #HAP_compute_res_hmx_lock3()
 *
 * @param[in] hmx_mutex Should be the same parameter used to lock HMX via
 *                      #HAP_compute_res_hmx_lock3()
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * HAP_COMPUTE_RES_NOT_SUPPORTED when not supported.
 */
static inline int HAP_compute_res_hmx_unlock3(unsigned int context_id,
                                              compute_res_hmx_type_t type,
                                              compute_res_hmx_mutex_t *hmx_mutex)
{
    if (crm_hmx_unlock3)
    {
        return crm_hmx_unlock3(context_id, type, hmx_mutex);
    }

    return HAP_COMPUTE_RES_NOT_SUPPORTED;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif //HAP_COMPUTE_RES_H_
