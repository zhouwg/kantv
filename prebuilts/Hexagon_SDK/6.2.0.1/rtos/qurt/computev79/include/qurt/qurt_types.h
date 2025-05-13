#ifndef QURT_TYPES_H
#define QURT_TYPES_H
/**
  @file qurt_types.h 
  @brief  Contains types common to all configurations

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/


//#include <stddef.h>
#include <qurt_consts.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                            CONSTANTS AND MACROS
=============================================================================*/
#define PGA_BITFIELD_MASK(hi,lo)    (((~0u)>>(31U-((hi)-(lo))))<<(lo))
#define PGA_BITFIELD_GET(x,hi,lo)   (((x)&PGA_BITFIELD_MASK((hi),(lo)))>>(lo))
#define PGA_BITFIELD_INS(hi,lo,v)   (((v)<<(lo))&PGA_BITFIELD_MASK((hi),(lo)))
#define PGA_BITFIELD_SET(x,hi,lo,v) ((x)=((x)&~PGA_BITFIELD_MASK((hi),(lo)))|PGA_BITFIELD_INS((hi),(lo),(v)))
#define QURT_PGATTR_C_GET(pga)      PGA_BITFIELD_GET((pga).pga_value, 3U, 0U)       /* Bits 3-0:  cache */
#define QURT_PGATTR_A_GET(pga)      PGA_BITFIELD_GET((pga).pga_value, 5U, 4U)       /* Bits 5-4:  bus attr */
#define QURT_PGATTR_C_SET(pga,v)    PGA_BITFIELD_SET((pga).pga_value, 3U, 0U, (v))  /* Bits 3-0:  cache */
#define QURT_PGATTR_A_SET(pga,v)    PGA_BITFIELD_SET((pga).pga_value, 5U, 4U, (v))  /* Bits 5-4:  bus attr */
#define QURT_PGATTR_MKRAW(v)        ((qurt_pgattr_t){.pga_value = (v)})
#define QURT_PGATTR_MK(c,a)         QURT_PGATTR_MKRAW(PGA_BITFIELD_INS(3U,0U,(c))|PGA_BITFIELD_INS(5U,4U,(a)))

/*return types for qurt_island_get_status2*/
#define QURT_ISLAND_MODE_NORMAL    0U    /**< Normal operating mode */
#define QURT_ISLAND_MODE_ISLAND    1U    /**< Island mode */
#define QURT_ISLAND_MODE_EXITING   2U    /**< In transition from Island mode to Normal mode */

/*=============================================================================
                        FORWARD DECLARATIONS & TYPEDEFS
=============================================================================*/
/** @addtogroup memory_management_types
@{ */
typedef unsigned int qurt_addr_t;          /**< QuRT address type.*/
typedef unsigned int qurt_paddr_t;         /**< QuRT physical memory address type.  */ 
/** @cond rest_reg_dist  */
typedef unsigned long long qurt_addr_64_t;  /**< QuRT 64-bit memory address type. */
typedef unsigned long long qurt_paddr_64_t; /**< QuRT 64-bit physical memory address type. */
typedef unsigned int qurt_mem_region_t;    /**< QuRT memory regions type. */
typedef unsigned int qurt_mem_fs_region_t; /**< QuRT memory FS region type. */
/**@endcond */
typedef unsigned int qurt_mem_pool_t;      /**< QuRT memory pool type.*/
typedef unsigned int qurt_size_t;          /**< QuRT size type. */
/** @cond  */
typedef unsigned long long qurt_mmu_entry_t;/**< QuRT MMU entry type. */
#define QURT_PHYSPOOL_NAME_LEN (32)
typedef char qurt_physpool_name_t[QURT_PHYSPOOL_NAME_LEN];


/*
 * Mapping type
 *
 * QMEM_MAPPING_VIRTUAL is the default mode, in which the system 
 * picks up the available range of the virtual address, and maps it to 
 * available contiguous physical addresses. Physical-to-virtual
 * is not guaranteed to be 1:1; both virtual and physical memory is 
 * contiguous.
 *
 * In QMEM_MAPPING_IDEMPOTENT mode, the user provides the physical address;
 * the kernel allocates 1:1 physical-to-virtual memory. Primary use of 
 * of this mapping is to allocate physical-to-virtual memory 1:1.
 *
 * In QMEM_MAPPING_PHYS_CONTIGUOUS mode, the virtual address might
 * not be the same as the physical address. But the physical address of the
 * memory region is guaranteed to be contiguous starting at the provided
 * address, it is required to provide a fixed physical address. The primary 
 * use of this mapping is to allocate physical memory from a particular 
 * address, where 1:1 physical-to-virtual is not required.
 *
 * QMEM_MAPPING_NONE mode must be used to reserve a virtual memory
 * area (VMA); no physical memory is reserved or mapped to this virtual
 * space; all standard qmem_region APIs apply to a VMA, however physical
 * address is always INVALID_ADDR. qmem_region_create() in this mode
 * returns a handle to the VMA, both virt_addr and phys_addr must
 * be set to INVALID_ADDR, kernel allocates any available virtual
 * memory of the specified size. Obtain the starting virtual address 
 * of VMA through qmem_region_attr_getvirtaddr().
 * Primary purpose of this mapping mode is to provide a mechanism for
 * delayed binding in QuRT, for example reserve virtual memory and map it at
 * some later time to possibly discontiguous physical blocks. Thus, a
 * single VMA can be partitioned among several physical-virtual mappings
 * created via qmem_region_create() with QMEM_VIRTUAL_FIXED mapping mode.
 * Each VMA keeps track of associated mapped regions.
 * Deletion of VMA succeeds only if all associated "virtual_fixed"
 * regions are freed prior to VMA deletion.
 *
 * Use QMEM_MAPPING_VIRTUAL_FIXED mode to create a region
 * from virtual space that has been reserved via qmem_region_create()
 * with QMEM_MAPPING_NONE mapping. A valid virt_add is required, if
 * phys_addr is specified, the kernel attempts to map it accordingly,
 * if no phys_addr is specified, kernel maps any available physical
 * memory. All standard qmem_region APIs apply to such region. Remapping
 * a virtual range without prior freeing of the region is not permitted.
 * When such region is deleted its corresponding VMA remains intact.
 *
 * QMEM_MAPPING_PHYS_DISCONTIGUOUS mode can obtain contiguous
 * virtual memory but physical memory can be discontiguous. This method
 * tries to club small physical memory blocks to obtain requested
 * memory and is useful in case where there is no contiguous full block
 * of requested size. If client does not need contiguous physical memory, 
 * (for example, if client does not use physical addressing), this helps
 * use smaller physical memory blocks rather than using contiguous memory.
 * Note: When memory is allocated through this method, physical address is
 * not returned to the caller using the qurt_mem_region_attr_get() API as there might
 * not be a single physical address.
 *
 */
/**@endcond */
/** QuRT memory region mapping type. */
typedef enum {
        QURT_MEM_MAPPING_VIRTUAL=0,            /**< Default mode. The region virtual address range maps to an 
                                          available contiguous area of physical memory. For the most
                                                    efficient use of virtual memory, the QuRT system 
                                                    chooses the base address in physical memory. This works for most memory
                                          use cases.*/
        QURT_MEM_MAPPING_PHYS_CONTIGUOUS = 1,  /**< The region virtual address space must be mapped to a 
                                               contiguous area of physical memory. This is necessary when the
                                               memory region is accessed by external devices that bypass Hexagon
                                               virtual memory addressing. The base address in physical 
                                               memory must be explicitly specified.*/
        QURT_MEM_MAPPING_IDEMPOTENT=2,         /**< Region virtual address space maps
                                             to the identical area of physical memory. */
        QURT_MEM_MAPPING_VIRTUAL_FIXED=3,      /**< Virtual address space of the region maps either to the 
                                           specified area of physical memory or (if no area is specified)
                                                    to available physical memory. Use this mapping to create
                                           regions from virtual space that was reserved by calling 
                                           qurt_mem_region_create() with mapping. */
        QURT_MEM_MAPPING_NONE=4,  /**< Reserves a virtual memory area (VMA). Remapping a virtual range is not
                                       permitted without first deleting the memory region. When such a region is
                                       deleted, its corresponding virtual memory addressing remains intact. */
        QURT_MEM_MAPPING_VIRTUAL_RANDOM=7,     /**< System chooses a random virtual address and
                                            maps it to available contiguous physical addresses.*/
        QURT_MEM_MAPPING_PHYS_DISCONTIGUOUS=8, /**< While virtual memory is contiguous, allocates in discontiguous physical 
                                                    memory blocks. This helps when there are smaller contiguous blocks
                                                    than the requested size.
                                                    Physical address is not provided as part of the get_attr call */
        QURT_MEM_MAPPING_INVALID=10,        /**< Reserved as an invalid mapping type. */
} qurt_mem_mapping_t;  


/** QuRT cache mode type. */
typedef enum {
        QURT_MEM_CACHE_WRITEBACK=7,     /**< Write back. */
        QURT_MEM_CACHE_NONE_SHARED=6,   /**< Normal uncached memory that can be shared with other subsystems.*/
        QURT_MEM_CACHE_WRITETHROUGH=5,  /**< Write through. */
        QURT_MEM_CACHE_WRITEBACK_NONL2CACHEABLE=0,    /**< Write back non-L2-cacheable.*/
        QURT_MEM_CACHE_WRITETHROUGH_NONL2CACHEABLE=1,  /**< Write through non-L2-cacheable. */
        QURT_MEM_CACHE_WRITEBACK_L2CACHEABLE=QURT_MEM_CACHE_WRITEBACK,  /**< Write back L2 cacheable. */
        QURT_MEM_CACHE_WRITETHROUGH_L2CACHEABLE=QURT_MEM_CACHE_WRITETHROUGH,  /**< Write through L2 cacheable.  */
        QURT_MEM_CACHE_DEVICE = 4,  /**< Volatile memory-mapped device. Access to device memory cannot be cancelled by interrupts, re-ordered, or replayed.*/
        QURT_MEM_CACHE_NONE = 4,  /**< Deprecated -- use #QURT_MEM_CACHE_DEVICE instead. */
        QURT_MEM_CACHE_DEVICE_SFC = 2, /**< Enables placing limitations on the number of outstanding transactions. */
        QURT_MEM_CACHE_INVALID=10,  /**< Reserved as an invalid cache type. */
} qurt_mem_cache_mode_t;

/** Memory access permission. */
#define     QURT_PERM_NONE    0x0U     /**< No permission. */
#define     QURT_PERM_READ    0x1U     /**< Read permission. */
#define     QURT_PERM_WRITE   0x2U     /**< Write permission. */
#define     QURT_PERM_EXECUTE 0x4U     /**< Execution permission. */
#define     QURT_PERM_NODUMP  0x8U   
                                    /**<  Skip dumping the mapping. During process domain dump, must skip
                                     some mappings on host memory to avoid a race condition
                                     where the memory is removed from the host and DSP process
                                     crashed before the mapping is removed. */
#define     QURT_PERM_FULL  QURT_PERM_READ | QURT_PERM_WRITE | QURT_PERM_EXECUTE  /**< Read, write, and execute permission. */

typedef unsigned char qurt_perm_t;


/** @cond rest_reg_dist*/
/** QuRT cache type; specifies data cache or instruction cache. */
typedef enum {
        QURT_MEM_ICACHE, /**< Instruction cache.*/
        QURT_MEM_DCACHE  /**< Data cache.*/
} qurt_mem_cache_type_t;

/** QuRT cache operation code type. */
typedef enum {
    QURT_MEM_CACHE_FLUSH, /**< Flush. */
    QURT_MEM_CACHE_INVALIDATE, /**< Invalidate */
    QURT_MEM_CACHE_FLUSH_INVALIDATE, /**< Flush invalidate. */
    QURT_MEM_CACHE_FLUSH_ALL, /**< Flush all. */
    QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, /**< Flush invalidate all. */
    QURT_MEM_CACHE_TABLE_FLUSH_INVALIDATE, /**< Table flush invalidate. */
    QURT_MEM_CACHE_FLUSH_INVALIDATE_L2, /**< L2 flush invalidate.*/
} qurt_mem_cache_op_t;

/** QuRT memory region type. */
typedef enum {
        QURT_MEM_REGION_LOCAL=0,  /**< Local. */
        QURT_MEM_REGION_SHARED=1,  /**< Shared.*/
        QURT_MEM_REGION_USER_ACCESS=2,  /**< User access. */
        QURT_MEM_REGION_FS=4,  /**< FS. */
        QURT_MEM_REGION_INVALID=10,  /**< Reserved as an invalid region type. */
} qurt_mem_region_type_t;

/* Cache and bus attributes are combined into a value of this type for convenience,
    and macros for combining and extracting fields are defined here.  */
/** @cond */
struct qurt_pgattr {
   unsigned pga_value; /**< PGA value.*/
};
typedef struct qurt_pgattr qurt_pgattr_t;
/** @endcond */
/** QuRT memory region attributes type.*/  
/* QMEM_MAPPING_IDEMPOTENT and QMEM_MAPPING_PHYS_CONTIGUOUS mode can specify physaddr.
   virtaddr cannot be specified for a memory region, it can only be queried by the 
   qmem_attr_getvirtaddr() function.
 */
typedef struct {
    /** @cond */
    qurt_mem_mapping_t    mapping_type; 
    unsigned char          perms;
    unsigned short         owner;
    qurt_pgattr_t          pga;
    unsigned               ppn; //physical page number (physical>>12)
    qurt_addr_t            virtaddr;
    qurt_mem_region_type_t   type;   
    qurt_size_t               size;
    /** @endcond */
} qurt_mem_region_attr_t;


/** QuRT user physical memory pool type. */
typedef struct {
    /** @cond */
    char name[32];
    struct ranges{
        unsigned int start;
        unsigned int size;
    } ranges[MAX_POOL_RANGES];
     /** @endcond */
} qurt_mem_pool_attr_t;

/** QuRT memory pool status type.*/
typedef struct _qurt_mem_pool_status {

    qurt_size_t         contig_size; /**< Largest contiguous free memory in bytes. */
    qurt_size_t         free_size;   /**< Total free memory in bytes. */
    qurt_size_t         total_size;  /**< Total declared memory in bytes. */

} qurt_mem_pool_status_t;

typedef enum {
    HEXAGON_L1_I_CACHE = 0,     /**< Hexagon L1 instruction cache. */
    HEXAGON_L1_D_CACHE = 1,     /**< Hexagon L1 data cache. */
    HEXAGON_L2_CACHE = 2        /**< Hexagon L2 cache. */
} qurt_cache_type_t;

typedef enum {
    FULL_SIZE = 0,                /**< Fully shared cache, without partitioning. */
    HALF_SIZE = 1,                /**< 1/2 for main, 1/2 for auxiliary. */
    THREE_QUARTER_SIZE = 2,       /**< 3/4 for main, 1/4 for auxiliary. */
    SEVEN_EIGHTHS_SIZE = 3        /**< 7/8 for main, 1/8 for auxiliary; for L2 cache only. */
} qurt_cache_partition_size_t;

typedef enum {
	QURT_PROCESS_CB_GENERIC,        /**< generic unconditional cb called after image loading. */
	QURT_PROCESS_NOTE_CB_PRE_MAP,   /**< note cb called before segment loading. */
	QURT_PROCESS_NOTE_CB_POST_MAP   /**< note cb called after segment loading. */
} qurt_process_cb_type_t;

typedef union {
    void *ptr;
    int num;
} qurt_process_callback_arg_t;


/**@endcond*/

/** @} */ /* end_addtogroup memory_management_types */
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_TYPES_H */
