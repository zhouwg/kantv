#ifndef QURT_MMAP_H
#define QURT_MMAP_H
/**
  @file qurt_mmap.h 
  @brief  Prototypes of memory mapping/unmapping APIs.
          The APIs allow the user to map, un-map, and change permissions
          on memory regions. 

  EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2018-2021, 2022, 2023 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_mem_mmap
  Creates a memory mapping with the specified attributes. 
  This API allows the root process caller to create mapping on behalf of a user
  process. If the client_handle belongs to a valid user process, the resulting
  mapping is created for the process.
  If -1 is passed in place of client_handle, the API creates mapping
  for the underlying process of the caller.

  @note1hang If the specified attributes are not valid, an error result is returned.  
                
  @param[out]  client_handle  Client handle to use for this mapping (optional).
  @param[in]   pool           Optional argument that specifies a pool handle
                              if the user wants to allocate memory from a specific pool.
                              The default value for this argument is NULL.
  @param[in]   pRegion        Map region. This argument is unused, and the default value is NULL.
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   prot           Mapping access permissions (R/W/X).
  @param[in]   flags          Mapping modes.\n
                              - #QURT_MAP_NAMED_MEMSECTION 
                              - #QURT_MAP_FIXED            \n
                              - #QURT_MAP_NONPROCESS_VPOOL \n
                              - #QURT_MAP_TRYFIXED         \n
                              - #QURT_MAP_ANON             \n
                              - #QURT_MAP_PHYSADDR         \n
                              - #QURT_MAP_VA_ONLY @tablebulletend  
  @param[in]   fd             File designator.
  @param[in]   offset         Offset in file.
 
  @return
  Valid virtual address -- Success.\n
  #QURT_MAP_FAILED -- Mapping creation failed. 
 */
void *qurt_mem_mmap(int client_handle,
                    qurt_mem_pool_t pool,
                    qurt_mem_region_t *pRegion,
                    void *addr,
                    size_t length,
                    int prot,
                    int flags,
                    int fd,
                    unsigned long long offset);

/**@ingroup func_qurt_mem_mmap2
  Creates a memory mapping with the specified attributes. Returns a more descriptive 
  error code in case of failure.
  This API allows the root process caller to create mapping on behalf of a user
  process. If the client_handle belongs to a valid user process, the resulting
  mapping is created for the process.
  If -1 is passed in place of client_handle, the API creates mapping
  for the underlying process of the caller.

  @note1hang If the specified attributes are not valid, an error result is returned.

  @param[out]  client_handle  Client handle to use for this mapping (optional).
  @param[in]   pool           Optional argument that allows the user to specify a pool handle
                              when the user wants to allocate memory from a specific pool.
                              Default value for this argument is NULL.
  @param[in]   pRegion        Map region (unused argument); default value is NULL.
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   prot           Mapping access permissions (R/W/X).
                              Cache attributes, bus attributes, User mode.
  @param[in]   flags          Mapping modes;
                              Shared, Private, or Anonymous.
  @param[in]   fd             File designator.
  @param[in]   offset         Offset in file.
 
  @return
  Valid virtual address -- Success.\n
  #QURT_EMEM -- Physical address is not available. \n
  #QURT_EFAILED -- VA is not available or mapping failed.\n
  #QURT_EINVALID -- Invalid argument was passed (for example, an unaligned VA/PA).
 */
void *qurt_mem_mmap2(int client_handle,
                    qurt_mem_pool_t pool,
                    qurt_mem_region_t *pRegion,
                    void *addr,
                    size_t length,
                    int prot,
                    int flags,
                    int fd,
                    unsigned long long offset);

/**@ingroup func_qurt_mem_mmap_by_name
  Creates a memory mapping for a named-memsection using the specified attributes.
  The named memsection should be specified in cust_config.xml.

  @note1hang If the specified attributes are not valid or the named memsection is not found,
  an error result is returned.
                  
  @param[in]   name           Name of the memsection in cust_config.xml that specifies 
                              this mapping. Should be less than 25 characters.
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   prot           Mapping access permissions (R/W/X).
                              Cache attributes, bus attributes, User mode
  @param[in]   flags          Mapping modes, such as
                              Shared, Private, or Anonymous.
  @param[in]   offset         Offset relative to the physical address range specified in memsection. 
                              If offset + length exceeds size of memsection, failure is 
                              returned.
  @return
  Valid virtual address -- Success.\n
  #QURT_MAP_FAILED -- Mapping creation failed. 
 */
void *qurt_mem_mmap_by_name(const char* name,
                            void *addr,
                            size_t length,
                            int prot,
                            int flags,
                            unsigned long long offset);

/**@ingroup func_qurt_mem_mprotect2
  Changes access permissions and attributes on an existing mapping based on the client_handle argument. 

  @note1hang If the specified virtual address is not found or invalid attributes are passed,
  an error code is returned.

  @note2 When error is returned, it is possible that attributes/permissions are changed for some part of the
          mapping, while for the remaining it is unchanged. Clients should not use these mappings further.
                  
  @param[in]   client_handle  Obtained from the current invocation function (Section 3.4.1).                   
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   prot           Mapping access permissions (R/W/X).
                              Cache attributes, Bus attributes, User mode.
  @return
  #QURT_EOK -- Successfully changes permissions on the mapping.\n
  #QURT_EFATAL -- Failed to change permissions on the mapping. \n
  #QURT_EINVALID -- Attributes / permissions requested are invalid.
 */
int qurt_mem_mprotect2(int client_handle, const void *addr,
                      size_t length,
                      int prot);

/**@ingroup func_qurt_mem_mprotect
  Changes access permissions and attributes on an existing mapping. 

  @note1hang If the specified virtual address is not found or invalid attributes are passed,
  an error code is returned.\n

  @note2 When error is returned, it is possible that attributes/permissions are changed for some part of the
          mapping, while for the remaining it is unchanged. Clients should not use these mappings further.
                  
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   prot           Mapping access permissions (R/W/X).
                              Cache attributes, Bus attributes, User mode.
  @return
  #QURT_EOK -- Successfully changes permissions on the mapping. \n
  #QURT_EFATAL -- Failed to change permissions on the mapping. \n
  #QURT_EINVALID -- Attributes / permissions requested are invalid.
 */
int qurt_mem_mprotect(const void *addr,
                      size_t length,
                      int prot);

/**@ingroup func_qurt_mem_munmap
  Removes an existing mapping. 

  @note1hang If the specified mapping is not found in the context of the caller process
  or invalid attributes are passed, an error code is returned.
                  
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  
  @return
  #QURT_EOK -- Successfully changes permissions on the mapping. \n
  #QURT_EFATAL -- Failed to change permissions on the mapping.
  #QURT_ELOCKED - Buffer is locked. Mapping delete failed.
 */
int qurt_mem_munmap(void *addr,
                    size_t length);

/**@ingroup func_qurt_mem_munmap2
  Removes an existing mapping for a specified process. 

  @note1hang This API allows a root process entity, such as a driver, to remove mapping
  that was created for a user process. If the specified mapping is not found in the context 
  of client handle or invalid attributes are passed, an error code is returned.
             
  @param[out]  client_handle  Client handle of the user process that owns this mapping. 
  @param[in]   addr           Virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  
  @return
  #QURT_EOK -- Successfully changes permissions on the mapping. \n
  #QURT_EFATAL -- Failed to change permissions on the mapping. 
  #QURT_ELOCKED - Buffer is locked. Mapping delete failed.
 */
int qurt_mem_munmap2(int client_handle,
                     void *addr,
                     size_t length);

/**@ingroup func_qurt_mem_munmap3
  Removes an existing mapping or reservation for a specified process. 

  @param[in]   client_handle  Client handle of the user process that owns this mapping. 
  @param[in]   addr           Pointer to a virtual memory address.
  @param[in]   length         Size of mapping in bytes.
  @param[in]   flags          Specifies the flag.
  
  @return
  #QURT_EOK -- Successfully changes permissions on the mapping. \n
  #QURT_EFATAL -- Failed to change permissions on the mapping. 
  #QURT_ELOCKED - Buffer is locked. Mapping delete failed.
 */
int qurt_mem_munmap3(int client_handle,
                     void *addr,
                     size_t length,
                     int flags);

/*
|| The macros here follow the style of the standard mmap() macros, but with
||  QURT_ prepended to avoid name conflicts, and to avoid having a dependency
||  on sys/mman.h.
||
|| Wherever possible, any values here that are also present in sys/mman.h
||  should have the same value in both places so that we can accept "mmap"
||  calls without having to remap parameters to new values.
||
|| In the future, it would be desirable to have a regression test that
||  checks, for instance, that these macros match.  Example:
||
||   assert(QURT_MAP_FAILED == MAP_FAILED);
||   ... repeat as needed ...
*/

/** @addtogroup memory_mapping_macros
@{ */
/** @cond */
#define QURT_PROT_NONE                  0x00U    /**< */
#define QURT_PROT_READ                  0x01U    /**< */
#define QURT_PROT_WRITE                 0x02U    /**< */
#define QURT_PROT_EXEC                  0x04U    /**< */
#define QURT_PROT_NODUMP                0x08U    /**< Skip dumping the mapping. During PD dump, must skip
                                                   some mappings on host memory to avoid a race condition
                                                      where the memory is removed from the host and the DSP process
                                                      crashes before the mapping is removed.*/
#define QURT_PROT_ISLAND                0x10U     /**< Island mapping. */

#define QURT_MAP_SHARED                 0x0001U   /**< Shared. */
#define QURT_MAP_PRIVATE                0x0002U   /**< Private. */
/** @endcond */
#define QURT_MAP_NAMED_MEMSECTION       0x0004U   /**< Named memsection. */
#define QURT_MAP_FIXED                  0x0010U   /**< Fixed virtual address. */
#define QURT_MAP_RENAME                 0x0020U   /**< Rename. */
#define QURT_MAP_NORESERVE              0x0040U   /**< No reserve. */
#define QURT_MAP_INHERIT                0x0080U   /**< Inherit. */
#define QURT_MAP_NONPROCESS_VPOOL       0x0100U   /**< Use a virtual address outside of the default range of the
                                                       processes. This option is only supported in the root process
                                                       and only when virtual memory split is enabled in the XML.
                                                       The root process can use this flag to create mapping for a
                                                       user process, for example, if the virtual address is configured
                                                       for a 3G/1G split, the root process can use this flag to create
                                                       mapping in the top 1 GB area for the user process or the
                                                       lower 3 GB area for the root process. This is useful for
                                                       shared buffer use cases. */
#define QURT_MAP_HASSEMAPHORE           0x0200U   /**< Has semaphore. */
#define QURT_MAP_TRYFIXED               0x0400U   /**< Try to create a mapping for a virtual address that was passed.
                                                       If the passed virtual address fails, use a random virtual address. */
#define QURT_MAP_WIRED                  0x0800U   /**< Wired. */
#define QURT_MAP_FILE                   0x0000U   /**< File. */
#define QURT_MAP_ANON                   0x1000U   /**< Allocate physical memory from the pool that was passed. 
                                                       By default, memory is allocated from the default physpool. */
#define QURT_MAP_VA_ONLY                0X2000U   /**< Reserve a virtual address without
                                                       mapping it. */

/** @cond */                                                   
#define QURT_MAP_ALIGNED(n)             ((n) << QURT_MAP_ALIGNMENT_SHIFT)
#define QURT_MAP_ALIGNMENT_SHIFT        24


#define QURT_MAP_ALIGNMENT_MASK         QURT_MAP_ALIGNED(0xff)   /**< */
#define QURT_MAP_ALIGNMENT_64KB         QURT_MAP_ALIGNED(16)     /**< */
#define QURT_MAP_ALIGNMENT_16MB         QURT_MAP_ALIGNED(24)     /**< */
#define QURT_MAP_ALIGNMENT_4GB          QURT_MAP_ALIGNED(32)     /**< */
#define QURT_MAP_ALIGNMENT_1TB          QURT_MAP_ALIGNED(40)     /**< */
#define QURT_MAP_ALIGNMENT_256TB        QURT_MAP_ALIGNED(48)     /**< */
#define QURT_MAP_ALIGNMENT_64PB         QURT_MAP_ALIGNED(56)     /**< */
/** @endcond */
#define QURT_MAP_FAILED                 ((void *) -1)            /**< Mapping creation failed. */

/*
|| The macros below are extensions beyond the standard mmap flags, but follow
||  the style of the mmap flags.
*/
/** @cond */
// Describe bitfields in (prot)
#define QURT_PROT_CACHE_BOUNDS          16U,19U,7U         /**< Bits 16 through 19 are cache attribute, default is 0. */
#define QURT_PROT_BUS_BOUNDS            20U,21U,0U         /**< Bits 20 through 21 are bus attributes, default is 0. */
#define QURT_PROT_USER_BOUNDS           22U,23U,3U         /**< Bits 22 through 23 are user mode, default is 3;
                                                                default of 3 means to derive user mode setting from the
                                                                default mode of the client. */

// Describe bitfields in (flags)
#define QURT_MAP_PHYSADDR_BOUNDS        15U,15U,0U         /**< Bits 15 through 15 are physaddr, default is 0. */
#define QURT_MAP_TYPE_BOUNDS            16U,19U,0U         /**< Bits 16 through 19 are mapping type, default is 0. */
#define QURT_MAP_REGION_BOUNDS          20U,23U,0U         /**< Bits 20 through 23 are region type, default is 0. */
/** @endcond */

// These macros get OR'ed into (prot)
#define QURT_PROT_CACHE_MODE(n)         QURT_MMAP_BUILD(QURT_PROT_CACHE_BOUNDS,(n)) /**< */
#define QURT_PROT_BUS_ATTR(n)           QURT_MMAP_BUILD(QURT_PROT_BUS_BOUNDS,(n))   /**< */
#define QURT_PROT_USER_MODE(n)          QURT_MMAP_BUILD(QURT_PROT_USER_BOUNDS,(n))  /**< */
// These macros get OR'ed into (flags)

#define QURT_MAP_PHYSADDR               QURT_MMAP_BUILD(QURT_MAP_PHYSADDR_BOUNDS,1U) /**< Use the physical address that was passed in offset field. 
                                                                                          This is allowed only for root process. */
#define QURT_MAP_TYPE(n)                QURT_MMAP_BUILD(QURT_MAP_TYPE_BOUNDS,(n))    /**< */
#define QURT_MAP_REGION(n)              QURT_MMAP_BUILD(QURT_MAP_REGION_BOUNDS,(n))  /**< */
/** @} */ /* end_addtogroup memory_mapping_macros */
/** @cond */
// These macros extract fields from (prot)
#define QURT_PROT_GET_CACHE_MODE(n)     QURT_MMAP_EXTRACT(QURT_PROT_CACHE_BOUNDS,(n))  /**< */
#define QURT_PROT_GET_BUS_ATTR(n)       QURT_MMAP_EXTRACT(QURT_PROT_BUS_BOUNDS,(n))    /**< */
#define QURT_PROT_GET_USER_MODE(n)      QURT_MMAP_EXTRACT(QURT_PROT_USER_BOUNDS,(n))   /**< */

// These macros extract fields from (flags)
#define QURT_MAP_GET_TYPE(n)            QURT_MMAP_EXTRACT(QURT_MAP_TYPE_BOUNDS,(n))   /**< */
#define QURT_MAP_GET_REGION(n)          QURT_MMAP_EXTRACT(QURT_MAP_REGION_BOUNDS,(n)) /**< */

// Macros for bitfield insertion and extraction
#define QURT_MMAP_MASK(lo,hi)           (~((~0u) << ((hi)-(lo)+1U)))                     /**< Mask of same size as [lo..hi]. */
#define QURT_MMAP_BUILD_(lo,hi,def,n)   ((((n)^(def))&QURT_MMAP_MASK((lo),(hi)))<<(lo)) /**< */
#define QURT_MMAP_EXTRACT_(lo,hi,def,n) ((((n)>>(lo))&QURT_MMAP_MASK((lo),(hi)))^(def)) /**< */
#define QURT_MMAP_BUILD(a,b)            QURT_MMAP_BUILD_(a,b)                           /**< */
#define QURT_MMAP_EXTRACT(a,b)          QURT_MMAP_EXTRACT_(a,b)                         /**< */
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
