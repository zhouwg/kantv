# HAP_mem APIs

## Overview
The HAP_mem APIs provide functionality available from the DSP to

* allocate and free memory - HAP_malloc() and HAP_free()
* map and unmap ION buffers allocated on the application processor and passed to the DSP using file descriptors - HAP_mmap() and HAP_munmap()
* get heap statistics and set properties - HAP_mem_get_stats(), HAP_mem_set_grow_size(), HAP_mmap_get(), HAP_mem_set_heap_thresholds() and HAP_mmap_put()
* allocate and free APPS memory - HAP_apps_mem_request() and HAP_apps_mem_free()

## Memory mapping

A common usage scenario for using the mapping functionality consists of the application processor allocating ION memory and passing the file descriptor to the DSP.
The DSP will then use the HAP_mem APIs to map the buffer onto the DSP and obtain a memory pointer. The mapping will remain valid until the buffer is being unmapped.
This approach allows to maintain a mapping across multiple FastRPC calls.

## Memory allocation

HAP_malloc and HAP_free are simple wrappers around the DSP malloc and free functions.
If a user memory allocation request cannot be fulfilled with the existing DSP heap, the FastRPC
runtime will attempt to grow the DSP heap by reserving additional memory from the HLOS.

The HAP_set_grow_size API can be called to configure the minimum and maximum size that should be added to the DSP heap when one of these growth events occurs.
If many growth events are anticipated, it may be appropriate to set a larger growth rate to reduce the number of growth events.  However, increasing
the heap more than necessary will impact HLOS performance.  Therefore, care must be taken in finding the appropriate growth rate for a given application.

Here is how the min and max values set by the HAP_set_grow_size control the growth of the heap:

    min_grow_bytes = MIN(max,MAX(min,min_grow_bytes));

    // The value will be aligned to the next 1MB boundary.

    actual_grow_bytes = min_grow_bytes + request_size
    actual_grow_bytes = ALIGN(actual_grow_bytes,0x100000)

`HAP_apps_mem_request()` and `HAP_apps_mem_release()` APIs can be called from the DSP to allocate APPS memory and map the same memory on the DSP if required.

These HAP request and release APIs are recommended when the user wants greater control over the DSP virtual address space: unlike `malloc` and `free`, these APIs guarantee that the memory will be mapped when allocated and unmapped when freed.

The mapping on the DSP can be controlled using the `flags` parameter in `HAP_apps_mem_request()`:

    * `HAP_MEM_FLAGS_SKIP_DSP_MAP` results in skipping the mapping on the DSP. In that case, the user needs to map the DSP memory by calling `HAP_mmap()`.

    * `HAP_MEM_FLAGS_DSP_MAP` results in mapping the buffer on the DSP upon calling `HAP_apps_mem_request()`.

`HAP_apps_mem_release()` will always free the allocated HLOS memory but will only unmap the buffer on the DSP if the flag `HAP_MEM_FLAGS_DSP_MAP` was used when calling `HAP_apps_mem_request()`.

***NOTE***
If HAP_MEM_FLAGS_SKIP_DSP_MAP flag was used when calling `HAP_apps_mem_request()`, and the memory was mapped later using `HAP_mmap()`, then the user needs to unmap DSP memory by calling `HAP_munmap()`.

## Memory statistics

HAP_mem_get_stats is useful when called at the beginning and end of an application to check for any memory leaks.

## Memory request API
HAP_mem_request is the request API, which support different request types. Requests supported are:

* `HAP_MEM_LOG_BLOCKS`: This request will log all the heap blocks to the csv file named - hprt_block_info_<asid>.csv, for parsing use QMemCheck tool.
    If block info logging is successful - 0 will be returned back by the HAP_mem_request. This request doesn't need any payload union.
* `HAP_MEM_SET_MARKER`: This request is to mark instances for leak detection, the markers can be START or END markers.
    When START marker is called, a marker instance number will be returned back to caller of the API (if the request is SUCCESS(0)) in the payload member:
    mem_marker_payload.
    When END marker is called, the caller should fill the instance number for which marker needs to be ended. If the request is success,
    all the leaks from the START to END of that instance will be logged to hprt_leak_block_info_<asid>_<instance_num>.csv
* `HAP_MEM_MAP`: This request is to create a DSP mapping for a shared buffer.
    The payload structure for this request can be referred to in `HAP_mem_map_t`. To create the mapping at a reserved va, the start address needs to be specified in the `addr` field of the payload.
    If the request is SUCCESS(0), the `dsp_va` member of payload will hold the mapped virtual address (VA).
* `HAP_MEM_UNMAP`: This request is to unmap the memory region on the DSP.
    The payload structure for this request can be referred to in `HAP_mem_unmap_t`. The starting virtual address and length of buffer needs to passed as payload members `dsp_va` and `len`.
    If the request is SUCCESS(0), the virtual address (VA) mapping is removed from the DSP.
* `HAP_RESERVE_VA`: This request is to reserve virtual address (VA) space on the DSP without creating any mappings.
    The payload structure for this request can be referred to in `HAP_mem_reserve_t`.
    If the request is SUCCESS(0), the `dsp_va` member of payload will hold the reserved virtual address (VA).
* `HAP_UNRESERVE_VA`: This request is to unreserve the virtual address (VA) space on the DSP.
    The payload structure for this request can be referred to in `HAP_mem_unreserve_t`. If the request is SUCCESS(0), the virtual address space is successfully unreserved.
