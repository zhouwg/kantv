#ifndef QURT_TLB_H
#define QURT_TLB_H

/**
  @file qurt_tlb.h 
  @brief  Prototypes of TLB API  
        The TLB APIs allow explicit control of the portion of TLB between TLB_first_replaceble and TLB_LAST_REPLACEABLE. 
        Both are nonconfigurable for the time being. This portion of TLB is permanently assigned/locked unless manually removed 
        by qurt_tlb_remove. Implementation does not change depending on the configuration, such as whether CONFIG_STATIC is set or not. 
        In CONFIG_STATIC=y, TLB_LAST_REPLACEABLE is set to the last TLB index, which indicates that the entire TLB is permanently 
        assigned and is not backed up by page table (page table does not exist). TLB indicies are maintained through a 64-bit bitmask. 
        A new entry is placed in the first available slot. 

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2013, 2021, 2023
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

#include <qurt_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/*=============================================================================
												FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_tlb_entry_create
  Creates a new TLB entry with the specified mapping attributes in the TLB of the Hexagon processor. \n
  @note1hang If the specified attributes are not valid (such as if the address is not aligned with the
             size), the entry is created and an error result is returned.\n
  @note1cont To set the G bit in the new TLB entry, set the ASID argument to -1.

  @datatypes
  #qurt_addr_t \n
  #qurt_paddr_t \n
  #qurt_mem_cache_mode_t \n
  #qurt_perm_t
  
  @param[out]  entry_id         TLB entry identifier.
  @param[in]   vaddr 			Virtual memory address.
  @param[in]   paddr  			Physical memory address.
  @param[in]   size  			Size of memory region to map (in bytes).
  @param[in]   cache_attribs    Cache mode (writeback, and so on).
  @param[in]   perms  			Access permissions.  
  @param[in]   asid  			ASID (space ID).
 
  @return
  #QURT_EOK -- TLB entry successfully created.\n
  #QURT_EFATAL -- Entry is not created; the TLB is full. \n
  #QURT_ETLBCREATESIZE -- Entry is not created; the incorrect size was specified. \n
  #QURT_ETLBCREATEUNALIGNED -- Entry is not created; an unaligned address was specified. \n
  #QURT_EINVALID -- Invalid cache attributes / permissions provided.

 */
int  qurt_tlb_entry_create (unsigned int *entry_id, qurt_addr_t vaddr, qurt_paddr_t paddr, qurt_size_t size, qurt_mem_cache_mode_t cache_attribs, qurt_perm_t perms, int asid);

/**@ingroup func_qurt_tlb_entry_create_64
  Creates a new TLB entry with the specified mapping attributes in the TLB of the Hexagon processor. \n
  @note1hang If the specified attributes are not valid (the address is not aligned with the
             size), the entry is not created, and an error result is returned.\n
  @note1cont To set the G bit in the new TLB entry, set the asid argument to -1.
  
  @param[out]  entry_id         TLB entry identifier.
  @param[in]   vaddr 			Virtual memory address.
  @param[in]   paddr_64         64-bit physical memory address.
  @param[in]   size  			Size of memory region to map (in bytes).
  @param[in]   cache_attribs    Cache mode (writeback, and so on).
  @param[in]   perms  			Access permissions.  
  @param[in]   asid  			ASID (space ID).
 
  @return
  #QURT_EOK -- TLB entry successfully created.\n
  #QURT_EFATAL -- Entry was not created; the TLB is full. \n
  #QURT_ETLBCREATESIZE -- Entry was not created; the incorrect size was specified. \n
  #QURT_ETLBCREATEUNALIGNED -- Entry was not created; an unaligned address was specified. \n
  #QURT_EINVALID -- Invalid cache attributes / permissions provided.

 */
int qurt_tlb_entry_create_64 (unsigned int *entry_id, qurt_addr_t vaddr, qurt_paddr_64_t paddr_64, qurt_size_t size, qurt_mem_cache_mode_t cache_attribs, qurt_perm_t perms, int asid);

/**@ingroup func_qurt_tlb_entry_delete 
  Deletes the specified TLB entry from the TLB of the Hexagon processor.
  If the specified entry does not exist, no deletion occurs and an error result is returned.

  @param[in]   entry_id  TLB entry identifier.			

  @return
  #QURT_EOK -- TLB entry successfully deleted. \n
  #QURT_EFATAL -- TLB entry does not exist.

  @dependencies
  None.
 **/
int qurt_tlb_entry_delete (unsigned int entry_id);

/**@ingroup func_qurt_tlb_entry_query
  Searches for the specified TLB entry in the TLB of the Hexagon processor.
  If the TLB entry is found, its entry identifier is returned.

  @datatypes
  #qurt_addr_t

  @param[out]   entry_id     TLB entry identifier.  
  @param[in]    vaddr  		 Virtual memory address.
  @param[in]    asid 		 ASID (space ID).

  @return  
  #QURT_EOK -- TLB entry successfully returned. \n
  #QURT_EFATAL -- TLB entry does not exist.

  @dependencies
  None.
 **/
int qurt_tlb_entry_query (unsigned int *entry_id, qurt_addr_t vaddr, int asid);

/**@ingroup func_qurt_tlb_entry_set
  Sets the TLB entry by storing an entry at the specified location 
  in the TLB of the Hexagon processor.

  @param[in]   entry_id  		TLB entry identifier.
  @param[in]   entry  			64-bit TLB entry to store.

  @return
  #QURT_EOK -- Entry successfully stored in the TLB. \n
  #QURT_EFATAL -- Entry not set at the specified location.

  @dependencies
  None.
 **/
int qurt_tlb_entry_set (unsigned int entry_id, unsigned long long int entry);

/**@ingroup func_qurt_tlb_entry_get
  Gets the TLB entry. \n
  Returns the specified 64-bit TLB entry in the TLB of the Hexagon processor.

  @param[in]    entry_id  	TLB entry identifier.
  @param[out]   entry       64-bit TLB entry.

  @return
  #QURT_EOK -- TLB entry successfully returned. \n
  #QURT_EFATAL -- TLB entry does not exist.   

  @dependencies
  None.
 **/
int qurt_tlb_entry_get (unsigned int entry_id, unsigned long long int *entry);

/**@ingroup func_qurt_tlb_get_pager_physaddrs
  Searches the TLB of the Hexagon processor, and returns all physical addresses that belong to the pager.
  Each returned address indicates the starting address of an active page.

The function return value indicates the number of addresses returned.

  @param[out]  pager_phys_addrs  Pointer to the return array of pager physical addresses.
 
  @return
  Integer -- Number of addresses returned in array.

  @dependencies
    None.
*/

unsigned int qurt_tlb_get_pager_physaddr(unsigned int** pager_phys_addrs);

/**@ingroup func_qurt_tlb_get_pager_virtaddr
  Searches the TLB of the Hexagon processor, and returns all virtual addresses that belong to the pager.
  Each returned address indicates the starting address of an active page.

The function return value indicates the number of addresses returned.

  @param[out]  pager_virt_addrs  Pointer to the return array of pager virtual addresses.
 
  @return
  Integer -- Number of addresses returned in the array.

  @dependencies
    None.
*/

unsigned int qurt_tlb_get_pager_virtaddr(unsigned int** pager_virt_addrs);


/**@ingroup func_qurt_tlb_entry_set2
  Sets the TLB entry by storing an entry at the specified location 
  in the TLB of the Hexagon processor. An additional option can be passed 
  to lock the TLB entry in the TLB of the Hexagon processor.

  @param[in]   id     TLB entry identifier.
  @param[in]   tlb    64-bit TLB entry to store.
  @param[in]   lock   Nonzero value indicates that the TLB entry must be locked in the hardware TLB.

  @return
  #QURT_EOK -- Entry successfully stored in the TLB. \n
  #QURT_EFATAL -- Entry not set at the specified location.

  @dependencies
  None.
 **/
int qurt_tlb_entry_set2(unsigned id, unsigned long long tlb, unsigned lock);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_TLB_H */
