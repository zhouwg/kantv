# Remote session API to interface with FastRPC


##  Overview

FastRPC exposes a set of APIs enabling the following functionality:

 - open, configure and close a remote session on the DSP
 - enable unsigned PD offload to the compute DSP
 - enable and manage QoS mode
 - make synchronous or asynchronous remote calls
 - query DSP capabilities
 - map or unmap pages onto the DSP

The 64-bit version of the API (`handle64`) enables multi-domain modules.
It is recommended for applications to use the multi-domain framework,
which provides multiple benefits over the older single-domain framework. remote_handle_*
APIs should be used for single-domain applications. For more information on multi-domain support,
refer to the RPC section in the Hexagon SDK documentation.

# `remote_handle_open`, `remote_handle64_open`
Loads the shared object on the remote process domain.

# `remote_handle_invoke`, `remote_handle64_invoke`
Executes a process on the remote domain.

# `remote_handle_close`, `remote_handle64_close`
Closes the remote handle opened by the remote process.

# `remote_handle_control`, `remote_handle64_control`
Manages the remote session.
This API allows to control or query the remote session:
- Control latency
    The user can vote for a specific latency requirement per session. This latency is not guaranteed by the driver. The driver will try to
    meet this requirement with the options available on a given target. Based on the arguments, either PM-QoS [Power Management] or adaptive
    QoS can be enabled.

    PM-QoS is recommended for latency-sensitive use cases, whereas adaptive QoS is recommended for moderately latency-sensitive use cases.
    Adaptive QoS is more power-efficient than PM-QoS.

    If PM-QoS enabled, CPU low power modes will be disabled.

    If Adaptive QoS is enabled, the remote DSP starts keeping track of the method execution times for that process. Once enough data is available,
    the DSP will try to predict when the method will finish executing and will send a "ping" to wake up the CPU prior to the completion of the
    DSP task so that there is no extra overhead due to CPU wakeup time.

- Enable wake lock
    Keep the CPU up until a response from the remote invocation call is received.  Disabling wake lock feature enables the CPU to be in suspend mode.

- Query DSP Capabilities
    Queries DSP support for:

    * domains available
    * unsigned PD
    * HVX, VTCM, HMX
    * async FastRPC
    * remote PD status notification

- Get DSP domain
    Returns the current DSP domain.

# `remote_session_control`

Sets remote session parameters such as thread stack size or unsigned PD mode.  Enables to kill remote process, closes sessions on the DSP,
generates a PD dump, or triggers remote process exceptions.

- [Stack thread parameters](structremote__rpc__thread__params.html)<br>
    Parameters to configure a thread: priority and stack size.

- [Unsigned PD](structremote__rpc__control__unsigned__module.html)<br>
    Flag to configure the session as unsigned. This allows third party applications to run compute
    intensive tasks on the compute DSP for better performance.

- [Kill remote process](structremote__rpc__process__clean__params.html)<br>
    Kills the remote process running on the DSP.

- [Session close](structremote__rpc__session__close.html)<br>
    Closes all sessions open on a given domain.

- [PD dump](structremote__rpc__control__pd__dump.html)<br>
    Enables PD dump feature.

- [Remote process exception](structremote__rpc__process__clean__params.html)<br>
    Introduces an exception in the remote process.

- [Query process type](structremote__process__type.html)<br>
    Query the type of process (signed or unsigned) running on remote DSP.

- [Relative thread priority](structremote__rpc__relative__thread__priority.html)<br>
    Set a lower or higher priority than the default thread priority, for the user threads on the DSP.

# `fastrpc_mmap`, `fastrpc_munmap`
Creates a new mapping of an ION memory into the virtual address space of a remote process on the DSP and associates the mapping with the
provided file descriptor. The parameter `flags` of type `fastrpc_map_flags` allows the user to control the page permissions and other
properties of the memory map. These mappings can be destroyed with `fastrpc_munmap()` API using the file descriptor. APIs `fastrpc_mmap`
and `fastrpc_munmap` are available and their use is recommended for Lahaina and later chipsets.

# `remote_mem_map`, `remote_mem_unmap`
Maps/unmaps large buffers statically on a given DSP.
Mapping the buffers statically saves the latency for the corresponding remote calls associated with these buffers.
These APIs are available only on SM8250 (Kona) or later targets.

# `remote_handle_invoke_async`, `remote_handle64_invoke_async`
Make remote invocations asynchronous. Running asynchronously does not improve the latency but improves the throughput by enabling the DSP
to run successive tasks continuously. This feature is supported on Lahaina and onward targets.

# `fastrpc_async_get_status`
Queries the status of the asynchronous job.

# `fastrpc_release_async_job`
Releases the asynchronous job after receiving the status either through callback or poll.

# `remote_register_buf`, `remote_register_buf_attr`
Registers a file descriptor for a buffer allocated with ION memory to share the memory with the DSP via SMMU.

# `remote_register_dma_handle`, `remote_register_dma_handle_attr`
Registers a DMA handle allocated with ION memory to share the memory with the DSP via SMMU.

Header file: @b remote.h

