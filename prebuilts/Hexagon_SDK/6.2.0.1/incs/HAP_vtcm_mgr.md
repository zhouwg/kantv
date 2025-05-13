# VTCM manager

Vector TCM (VTCM) is available on supported targets with cDSP. VTCM 
is a high-performance, tightly-coupled memory in the cDSP subsystem that can be used
for Hexagon Vector eXtensions (HVX) scatter/gather instructions, Hexagon Matrix
eXtension (HMX)(available in some cDSPs starting with Lahaina), or as
high-performance scratch memory for other HVX workloads.

The VTCM manager exposes APIs in from the `HAP_vtcm_mgr.h` file to allocate, free, and query the availability of VTCM.

***NOTE:***
Starting with Lahaina, use the [compute resource manager](../../doxygen/HAP_compute_res/index.html){target=_blank} API for VTCM allocations instead of this legacy VTCM manager API. The compute resource manager is expanded to provide user options to do the following:

* Allocate other compute resources (including VTCM)
* Manage application IDs, which control VTCM partitions and privileges
* Send release callbacks, which can be invoked when a high priority client requires the resource
* Release and reacquire the same VTCM size and page configuration
* Request VTCM with granular sizes (minimum and maximum required) and specific page size requirements

The VTCM manager API is restricted to allocate VTCM only from the
primary VTCM partition (if the partition is defined).
