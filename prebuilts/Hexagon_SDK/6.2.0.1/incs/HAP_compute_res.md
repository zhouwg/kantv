# Compute resource manager framework

The cDSP has several shared resources such as L2 cache, HVX, HMX (where available), VTCM, hardware threads, and memory
buses.  The compute resource manager framework exposes in @b HAP_compute_res.h a set of APIs for managing, requesting
and releasing some of these resources.

## Legacy HAP_vtcm_mgr API

VTCM allocation APIs exposed under [VTCM Manager](../../doxygen/HAP_vtcm_mgr/index.html) are being deprecated, we
recommend using the compute resource APIs for VTCM management and allocation. The compute resource manager provides
options to:

* Query defined VTCM on an architecture and VTCM usage.
* Cached mode: Release and reacquire the same VTCM virtual address, size and page configuration.
* Cooperative preemption: Register release callbacks, which might be invoked when a high-priority client needs a resource
used by a lower-priority client.
* ThreadID based autonomous preemption: Register threads that work on the compute resources with resource manager, these
threads will be suspended by the resource manager when a high priority client requests. Clients also provide a backup buffer
for VTCM, used by the resource manager to save and restore VTCM context during preemption.
* Query supported preemption model (cooperative, ThreadID based autonomous preemption etc.)

## Serialization

The resource manager also offers a virtualized serialization resource to aid concurrencies in which constituent use cases
are to run with mutually exclusive access to the entire cDSP, for example, to avoid cache thrashing with each other.
Participating use cases issue blocking acquires on the serialization resource when ready to run, and each use case runs
in turn when it is granted that resource. Acquiring the serialization resource only ensures mutual exclusion from other
cooperating use cases that also block on acquisition of that resource; it does not guarantee exclusion from concurrent
use cases that do not block on the serialization resource.

## Cached mode

Clients requesting for VTCM are provided with a pointer (virtual address) to VTCM on success. The pointer to VTCM can
change once it's released (HAP_compute_res_release()) and re-acquired (HAP_compute_res_acquire()). Clients requiring a
constant VTCM pointer through out a session can use the cached mode. Cached mode can be enabled by setting cached attribute
using HAP_compute_res_attr_set_cache_mode() when acquiring (HAP_compute_res_acquire()) the resource. When cached attribute
is set while acquiring the resource, clients are expected to call HAP_compute_res_acquire_cached() with the context ID
returned by HAP_compute_res_acquire() before accessing the resource.

This mode is useful for periodic applications where VTCM pointer needs to remain the same at every execution while allocating
and releasing the resource periodically:
* HAP_compute_res_acquire() with cached attribute set is called for allocating VTCM during initialization.
* HAP_compute_res_acquire_cached() and HAP_compute_res_release_cached() called before and after every execution.
* HAP_compute_res_release() is called during de-initialization.

From v73 architecture, cached mode also provides clients with an option to have an overlapping mapping from within a process.

### overmapping / overlapping page mapping

Applications working on HMX may require all of the requested VTCM to be in a single page mapping in MMU. When overmapping /
overlapping page mapping feature is supported, the HMX applications requesting for a page size covering entire VTCM with
a smaller VTCM size can allow other applications running from the same user process to allocate remaining VTCM size when
cached mode is used.

For example, on an architecture supporting 8MB of VTCM, HMX application (APP1) requesting for 6MB of VTCM with a minimum of
8MB page in cached mode can allow another application (APP2) to acquire remaining 2MB of VTCM with a maximum page size of
1MB.

![screenshot](../../images/CRM_VTCM_overmapping_example.png)

Note:
* Only cached allocations requesting for VTCM page size covering entire VTCM defined for that architecture but with a
smaller VTCM size request will result in overmapping condition. For example, on architecture with 8MB VTCM, a cached/
non-cached request for 3MB VTCM with 4MB page size will get a 4MB allocation (3MB wrapped to the page size).
* Multiple cached/non-cached allocations from within the same process (as overmapping client) can use the left over space
in VTCM as long as their requests can be accomodated in that space. For example, a 2MB size request with single page/
4MB page size cannot coexist concurrently with a cached 6MB request with 8MB page size.
* Multiple overmapping clients cannot coexist concurrently. For example, a 4MB size with 8MB page request cannot coexist
concurrently with another 4MB sized request with 8MB page.

## VTCM window feature
Starting with v79, the NSP has a VTCM window hardware feature, which can be used to prevent a thread from accessing a specific VTCM region. The compute resource manager utilizes this feature as an additional access control layer on top of the page mappings per process.
### Use case:
* Defined VTCM memory: 8MB
* Process 1: VTCM memory request 6MB, single page mapping
* Process 2: VTCM memory request 2MB

In this use case, 6MB of VTCM to be mapped in a single page requires all of 8MB VTCM to be allocated.  The difference between architectures with or without the VTCM window feature is in how the remaining 2 MB of VTCM allocated but unused by process 1 may be used by another process.

### Q6 architecture < V79 (Where VTCM window feature not available)
As the entire 8MB of defined VTCM is mapped to the user process, the allocating user process, process 1, has access to the 2MB free space as well. The 2MB is marked free for other allocations from the same user process and not available for requests from other user processes.

![screenshot](../../images/hap_compute_res_mgr_no_vtcm_window.png)

Process 2 cannot use the 2MB of free space while process 1 still holds its allocation.

### Q6 architecture >= V79 (Where VTCM window feature available)
The compure resource manager restricts the allocating user process, process 1, to access only 6MB of allocated space using `VTCM window` hardware feature. This allows other user processes to access this 2MB region. In this use case, process 1 has neither read nor write access to the remaining 2 MB of VTCM.

![screenshot](../../images/hap_compute_res_mgr_vtcm_window.png)

### VTCM window - restrictions
`VTCM window` can be useful to restrict threads within a process to desired VTCM regions. `VTCM window` should be a single contiguous memory region within the VTCM space: gaps inbetween allocations cannot be free for allocation from other user processes. For example, in the below scenario, the `VTCM window` cannot be used to allow other processes to allocate the 1MB of unallocated free space: it is only available for allocation from process 1.

![screenshot](../../images/hap_compute_res_mgr_vtcm_window_restrictions.png)

## Cooperative preemption framework

The resource manager offers cooperative preemption framework where in clients can register a release callback when
requesting for compute resources using HAP_compute_res_attr_set_release_callback(). When a higher-priority client requests
a resource already in use by a lower-priority client, the lower-priority client will be notified by the callback to suspend
its work and release the resource.

## Autonomous preemption framework (threadId based)

On supported architecutres (can be queried using HAP_compute_res_query_capability()), the resource manager implements
an autonomous based preemption framework where clients register thread IDs associated with a resource request and provide
VTCM backup buffer when VTCM is being requested. As part of preempting a context, the resource manager waits for HMX critical section
when HMX is in used, suspends registered threads and saves VTCM in provided backup buffer. When the resource becomes available
the resource manager resumes suspended threads after restoring VTCM and reattaching HMX (if previously assigned).

HMX under this preemption scheme is handled differently in comparison to the cooperative preemption framework.
In cooperative preemption framework, HMX as a resource is acquired first and then locked using HAP_compute_res_hmx_lock()/lock2() while
in autonomous preemption framework, HMX is directly locked via HAP_compute_res_hmx_lock3() using the context returned by
a successful VTCM allocation done using HAP_compute_res_acquire() call. As the resource manager can preempt a low
priority client, HMX applications need to implement HMX critical section using the mutex structure returned by a successful
HAP_compute_res_HMX_lock3() API.

## Usage examples

### Cached VTCM request - cooperative preemption

@code
int release_callback(unsigned int context, void *state)
{
    if (!context || !state) return FAILURE;
    /*
     * Got release request, set release required in state variable
     */
    application_state_t* local_state = (application_state_t *)state;
    if (local_state->context != context) return FAILURE;
    local_state->release_request = TRUE;
    return 0;
}

void initialization_routine()
{
    compute_res_attr_t attr;
    unsigned int context;
    unsigned int vtcm_size = 8 * 1024 * 1024;   //8MB of VTCM
    void *p_vtcm = NULL;
    unsigned int result_vtcm_size = 0;
    /*
     * Initialize the attribute structure
     */
    if (0 != HAP_compute_res_attr_init(&attr))
        return;
    /*
     * Query VTCM defined in the architecture and set our request VTCM size
     * to the defined one (request for entire VTCM size)
     */
    if (0 != HAP_compute_res_query_VTCM(0, &vtcm_size, NULL, NULL, NULL))
        return;
    /*
     * Set VTCM params:
     * Requesting for entire VTCM size, minimum page size set to VTCM size,
     * minimum required VTCM size is set to the same as VTCM size
     */
    if (0 != HAP_compute_res_attr_set_vtcm_param_v2(&attr, vtcm_size, vtcm_size, vtcm_size))
        return;
    /*
     * Set cached mode
     */
    if (0 != HAP_compute_res_attr_set_cache_mode(&attr, 1))
        return;
    /*
     * Set release callback
     */
    if (0 != HAP_compute_res_attr_set_release_callback(&attr, &release_callback, (void *)state))
        return;
    /*
     * Acquire a context with the prepared attribute structure
     */
    if (0 == (context = HAP_compute_res_acquire(&attr, 0)))
        return;
    /*
     * Get VTCM pointer
     */
    if (0 != HAP_compute_res_attr_get_vtcm_ptr_v2(&attr, &p_vtcm, &result_vtcm_size))
    {
        HAP_compute_res_release(context);
        return;
    }
    state->context = context;
    /*
     * Setup algorithm using p_vtcm and result_vtcm_size
     */
    return;
 }

int yield(unsigned int context)
{
    /*
     * Synchronize with workers to make sure all accesses to VTCM are complete
     * Backup VTCM if required
     * Release context and reacquire
     */
    if (0 == HAP_compute_res_check_release_request(context))
        return FAILURE;
    if (0 == HAP_compute_res_release_cached(context))
        return FAILURE;
    if (0 == HAP_compute_res_acquire_cached(context, <TIMEOUT_US>))
        return FAILURE;
    /*
     * Restore VTCM and continue remaining work
     */
    return 0;
}

void execution_loop()
    /*
     * Acquire the cached resource
     */
    if (0 != HAP_compute_res_acquire_cached(context, <TIMEOUT_US>))
        return;
    /*
     * Work items
     */
    for (i = 0; i < WORK_ITEMS; i++)
    {
        /*
         * Check if cooperative preemption requested for a release
         * param set in release_callback
         */
        if (state->release_request)
        {
            if (0 != yield(context))
                return;
        }
        //Execute work item
    }
    /*
     * Release the cached resource
     */
    if (0 != HAP_compute_res_release_cached(context))
        return;
}
@endcode

### Cached VTCM request - autonomous threadID based preemption

@code
int release_callback(unsigned int context, void *state)
{
    if (!context || !state) return FAILURE;
    /*
     * Got release request, set release required in state variable
     */
    application_state_t* local_state = (application_state_t *)state;
    if (local_state->context != context) return FAILURE;
    local_state->release_request = TRUE;
    return 0;
}

int check_autonomous_threads_compute_res_capability()
{
    unsinged int capability = 0;

    if (0 != HAP_compute_res_query_capability(HAP_COMPUTE_RES_PREEMPTION_CAPABILITY, &capability))
        return FAILURE;
    if (capability & HAP_COMPUTE_RES_THREADS_FOR_AUTONOMOUS_PREEMPTION)
        return 0;
    else
        return FAILURE;
}

void initialization_routine()
{
    compute_res_attr_t attr;
    unsigned int context;
    unsigned int vtcm_size = 8 * 1024 * 1024;   //8MB of VTCM
    void *p_vtcm = NULL, *p_vtcm_backup = NULL;
    unsigned int result_vtcm_size = 0;
    unsigned int thread_id = NULL;
    /*
     * Initialize the attribute structure
     */
    if (0 != HAP_compute_res_attr_init(&attr))
        return;
    /*
     * Query VTCM defined in the architecture and set our request VTCM size
     * to the defined one (request for entire VTCM size)
     */
    if (0 != HAP_compute_res_query_VTCM(0, &vtcm_size, NULL, NULL, NULL))
        return;
    /*
     * Set VTCM params:
     * Requesting for entire VTCM size, minimum page size set to VTCM size,
     * minimum required VTCM size is set to the same as VTCM size
     */
    if (0 != HAP_compute_res_attr_set_vtcm_param_v2(&attr, vtcm_size, vtcm_size, vtcm_size))
        return;
    /*
     * Set cached mode
     */
    if (0 != HAP_compute_res_attr_set_cache_mode(&attr, 1))
        return;
    /*
     * Check threads based autonomous preemption support and register threads
     */
    if (0 == check_autonomous_threads_compute_res_capability())
    {
        /*
         * Allocate backup buffer for VTCM to be registered with the resource
         * manager
         */
        p_vtcm_backup = malloc(vtcm_size);
        /*
         * Register VTCM backup buffer
         */
        if (0 != HAP_compute_res_attr_set_vtcm_backup(&attr, p_vtcm_backup, vtcm_size))
        {
            free(p_vtcm_backup);
            return;
        }
        /*
         * Register threads that will be working on the requested VTCM buffer
         */
        thread_id = qurt_thread_get_id();
        if (0 != HAP_compute_res_attr_set_threads(&attr, &thread_id, 1))
        {
            free(p_vtcm_backup);
            return;
        }
    } else {
        /*
         * Falling back to cooperative preemption when autonomous preemption
         * is not supported
         */
        if (0 != HAP_compute_res_attr_set_release_callback(&attr, &release_callback, (void *)state))
            return;
    }
    /*
     * Acquire a context with the prepared attribute structure
     */
    if (0 == (context = HAP_compute_res_acquire(&attr, 0)))
        return;
    /*
     * Get VTCM pointer
     */
    if (0 != HAP_compute_res_attr_get_vtcm_ptr_v2(&attr, &p_vtcm, &result_vtcm_size))
    {
        HAP_compute_res_release(context);
        return;
    }
    state->context = context;
    /*
     * Setup algorithm using p_vtcm and result_vtcm_size
     */
    return;
 }

int yield(unsigned int context)
{
    /*
     * Synchronize with workers to make sure all accesses to VTCM are complete
     * Backup VTCM if required
     * Release context and reacquire
     */
    if (0 == HAP_compute_res_check_release_request(context))
        return FAILURE;
    if (0 == HAP_compute_res_release_cached(context))
        return FAILURE;
    if (0 == HAP_compute_res_acquire_cached(context, <TIMEOUT_US>))
        return FAILURE;
    /*
     * Restore VTCM and continue remaining work
     */
    return 0;
}

void execution_loop()
    /*
     * Acquire the cached resource
     */
    if (0 != HAP_compute_res_acquire_cached(context, <TIMEOUT_US>))
        return;
    /*
     * Work items
     */
    for (i = 0; i < WORK_ITEMS; i++)
    {
        /*
         * Check if cooperative preemption requested for a release
         * param set in release_callback
         */
        if (state->release_request)
        {
            if (0 != yield(context))
                return;
        }
        //Execute work item
    }
    /*
     * Release the cached resource
     */
    if (0 != HAP_compute_res_release_cached(context))
        return;
}
@endcode

### Serialized VTCM acquisition

This example shows two threads requesting VTCM and both participating in serialization by invoking HAP_compute_res_attr_set_serialize().

@code
    /*
     * PROCESS/THREAD 1
     */
    compute_res_attr_t res_info;
    unsigned int context_id = 0;
    void *p_vtcm = NULL;
    /*
     * Initialize the attribute structure
     */
    if (0 != HAP_compute_res_attr_init(&res_info) )
        return;
    /*
     * Set serialization option
     */
    if (0 != HAP_compute_res_attr_set_serialize(&res_info, 1) )
        return;
    /*
     * Set VTCM request parameters - 256KB single page
     */
    if (0 != HAP_compute_res_attr_set_vtcm_param(&res_info,
                                                 (256 * 1024),
                                                 1) )
        return;
    /*
     * Call acquire with a timeout of 10 milliseconds.
     */
    if (0 != (context_id = HAP_compute_res_acquire(&res_info, 10000) ) )
    {
        /*
         * Successfully requested for serialization and acquired VTCM.
         * The serialization request from PROCESS/THREAD 2 waits
         * until the resource is released here.
         */
        p_vtcm = HAP_compute_res_attr_get_vtcm_ptr(&res_info);
        if (0 == p_vtcm)
        {
            /*
             * VTCM allocation failed, should not reach here as the acquire
             * returned with valid context ID.
             */
            HAP_compute_res_release(context_id);
            return;
        }
        //Do my work in process/thread 1
        /*
         * Done. Release the resource now using the acquired context ID.
         * This releases both the serialization request and VTCM allocation.
         */
        HAP_compute_res_release(context_id);
        p_vtcm = NULL;
    } else {
        /*
         * Unsuccessful allocation. Timeout would have triggered.
         * Implement a fallback or fail gracefully.
         */
    }

    ...

    /*
     * PROCESS/THREAD 2
     */
    compute_res_attr_t res_info;
    unsigned int context_id = 0;
    /*
     * Initialize the attribute structure.
     */
    if (0 != HAP_compute_res_attr_init(&res_info) )
        return;
    /*
     * Set serialization option.
     */
    if (0 != HAP_compute_res_attr_set_serialize(&res_info, 1) )
        return;
    /*
     * Call acquire with a timeout of 10 milliseconds.
     */
    if (0 != (context_id = HAP_compute_res_acquire(&res_info, 10000) ) )
    {
        /*
         * Successfully requested for serialization.
         * The serialization request from PROCESS/THREAD 1 waits
         * until the resource is released here even when the PROCESS/THREAD 1s
         * request for VTCM can be served.
         */
        //Do my work in process/thread 2
        /*
         * Done. Release the resource now using the acquired context ID.
         */
        HAP_compute_res_release(context_id);
    } else {
        /*
         * Unsuccessful allocation. Timeout would have triggered.
         * Implement a fallback or fail gracefully.
         */
    }
@endcode

### Non-serialized VTCM acquisition

This example shows two threads requesting VTCM alone without a serialization option.

If the total size requested by both threads exceeds the size of VTCM that is available, only one thread gets
access to VTCM while the other thread waits. In this case, the threads are serializing their workload
implicitly.

If enough VTCM memory is available to meet the requests of both threads, both threads acquire VTCM upon request
and can end up executing in parallel.

@code
    /*
     * PROCESS/THREAD 1
     */
    compute_res_attr_t res_info;
    unsigned int context_id = 0;
    void *p_vtcm = NULL;
    /*
     * Initialize the attribute structure.
     */
    if (0 != HAP_compute_res_attr_init(&res_info) )
        return;

	/* By not calling HAP_compute_res_attr_set_serialize, we enable thread 1 to acquire VTCM
	 * as long as enough memory is available	
	 */

    /*
     * Set VTCM request parameters - 256 KB single page
     */
    if (0 != HAP_compute_res_attr_set_vtcm_param(&res_info,
                                                 (256 * 1024),
                                                 1) )
        return;
    /*
     * Call acquire with a timeout of 10 milliseconds.
     */
    if (0 != (context_id = HAP_compute_res_acquire(&res_info, 10000) ) )
    {
        /*
         * Successfully acquired VTCM.
         * The VTCM request from PROCESS/THREAD 2 waits if enough
         * VTCM is not left to serve the request until the resource is released
         * here.
         */
        p_vtcm = HAP_compute_res_attr_get_vtcm_ptr(&res_info);
        if (0 == p_vtcm)
        {
            /*
             * VTCM allocation failed, should not reach this point as the acquire
             * returned with valid context ID.
             */
            HAP_compute_res_release(context_id);
            return;
        }
        //Do my work in process/thread 1
        /*
         * Done. Release the resource now using the acquired context ID.
         * This releases the VTCM allocation.
         */
        HAP_compute_res_release(context_id);
        p_vtcm = NULL;
    } else {
        /*
         * Unsuccessful allocation. Timeout would have triggered.
         * Implement a fallback or fail gracefully.
         */
    }

    ...

    /*
     * PROCESS/THREAD 2
     */
    compute_res_attr_t res_info;
    unsigned int context_id = 0;
    void *p_vtcm = NULL;
    /*
     * Initialize the attribute structure
     */
    if (0 != HAP_compute_res_attr_init(&res_info) )
        return;

	/* By not calling HAP_compute_res_attr_set_serialize, we enable thread 2 to acquire VTCM
	 * as long as enough memory is available	
	 */

    /*
     * Set VTCM request parameters - 256 KB single page.
     */
    if (0 != HAP_compute_res_attr_set_vtcm_param(&res_info,
                                                 (256 * 1024),
                                                 1) )
        return;
    /*
     * Call acquire with a timeout of 10 milliseconds.
     */
    if (0 != (context_id = HAP_compute_res_acquire(&res_info, 10000) ) )
    {
        /*
         * Successfully acquired VTCM.
         * The VTCM request from PROCESS/THREAD 1 waits if enough
         * VTCM is not left to serve the request until the resource is released
         * here.
         */
        p_vtcm = HAP_compute_res_attr_get_vtcm_ptr(&res_info);
        if (0 == p_vtcm)
        {
            /*
             * VTCM allocation failed, should not reach this point as the acquire
             * returned with valid context ID.
             */
            HAP_compute_res_release(context_id);
            return;
        }
        //Do work in PROCESS/THREAD 2
        /*
         * Done. Release the resource now using the acquired context ID.
         * This releases the VTCM allocation.
         */
        HAP_compute_res_release(context_id);
        p_vtcm = NULL;
    } else {
        /*
         * Unsuccessful allocation. Timeout would have triggered.
         * Implement a fallback or fail gracefully.
         */
    }
@endcode

