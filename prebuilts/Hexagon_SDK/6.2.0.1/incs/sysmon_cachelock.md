# Cache locking manager

The cache locking manager locks a section of the L2 cache from the
cDSP, and subsequently releases this lock.

The cache locking manager replaces the HAP_power_set APIs that are now deprecated.
This new cache locking manager utilizes available L2 cache by
allocating memory with appropriately aligned address based on L2 cache
availability and the request size. The cache locking manager also limits
maximum cache that can be locked to guarantee performance of the guest OS and
FastRPC threads.

The cache locking manager monitors cache locking usage by providing
APIs to get the maximum available cache size for locking and the total
currently locked cache.

Finally, a set of APIs passes the address of the memory to lock
along with its size information. These APIs are useful for applications where a
linker-defined section (code/library) must be locked into cache.

The cache locking manager APIs are not accessible from unsigned PD.

## Framework APIs

Header file: @b sysmon_cachelock.h
