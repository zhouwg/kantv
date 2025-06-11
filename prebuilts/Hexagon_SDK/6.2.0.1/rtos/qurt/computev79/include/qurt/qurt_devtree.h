#ifndef QURT_DEVTREE_H
#define QURT_DEVTREE_H
/**
 @file qurt_devtree.h 
 @brief  Prototypes and structures for device tree aware QuRT library function.

Copyright (c) 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
*/
/*qurt_callback is included by qurt_qdi_driver.h and depends on NULL being def.
  callback is not used here, so define NULL here to avoid including the world*/
#ifndef NULL
#define NULL ((void *) 0)
#endif

#include "libfdt.h"
#include "DTBExtnLib.h"
#include "qurt_qdi_ext.h"
#include "qurt_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_BLOB_ID       (-1)
#define DEFAULT_BLOB_ID         0

/** QURT Device Tree Mapping Macros */
#define QURT_DT_MAPPING_FAILED         (-1)
#define QURT_DT_FLAG_ISLAND             0x1
#define QURT_DT_FLAG_PHYSADDR           0x2

/** Device Tree type for Root PD Device tree.
    Root PD Device Tree will typically describe the hardware in the subsystem.
    This is the /soc portion of the Device Tree. */
#define QURT_DT_BLOB_TYPE_ROOT  0

/** Device Tree type for Local Device tree.
    Local Device Tree will typically contain the software settings.
    This is the /sw portion of the Device Tree. */
#define QURT_DT_BLOB_TYPE_LOCAL 1

int qurt_devtree_init(void);

/**@ingroup func_qurt_dt_mapping_create
 Creates a memory mapping from the specified property of the specified device
 tree node. Returns virtual addresses and sizes.
                  
 @param[in]   offset         Device tree node offset.
 @param[in]   flags          Flags to configure memory. Overloaded as property 
                              index if reg_name is NULL.
 @param[in]   reg_name       Identifies property to use for mapping, should 
                              resemble a region.
 @param[out]   vaddr         Return pointer for the virtual region address.
 @param[out]   size          Return pointer for the virtual region size.

 @return
 Result code indicating success or failure \n
*/
int qurt_dt_mapping_create(fdt_node_handle *devtreeNode, int flags, char *regionName, int regionIdx, 
                                unsigned long long *vaddr, unsigned long long *size);

/**@ingroup func_qurt_dt_mapping_create2
 
 Creates a memory mapping from the specified property of the specified device
 tree node.

 Returns virtual addresses and sizes according to architecture (i.e either 32 bit or 64 bit). 

 @param[in]   devtreeNode    Device Tree node    

 @param[in]   dt_map_flags   Flags to configure memory mapping and are reserved for future purpose.
                              (0) - Default value assumes details from DT node are phys address, size.
                              QURT_DT_FLAG_ISLAND <IslandMode-Mapping>

                              NOTE: The PA needs to be added to corresponding island spec to create an island mapping

 @param[in]   regionName     NULL or name of index in range to return, should 
                              resemble a region.       Ex.reg-names =  "base",         "rx",               "tx";

 @param[in]   regionIdx      Index of range to return.  Ex reg       = <0x1000 0x20>, <0x10000 0x100>, <0x18000 0x100 >;
                              
                              NOTE: If client specifies both re_name & regionIdx. The precedence of 
                              region name is taken over and region index is ignored.

 @param[in]   dt_map_perm    Mapping access permissions(R/W),
                              QURT_PERM_READ <Read only>
                              QURT_PERM_WRITE

 @param[in]   cache_attr     QuRT cache mode type's :
                              QURT_MEM_CACHE_DEVICE <memory-mapped device>
                              QURT_MEM_CACHE_WRITEBACK <Cached WB>
                              Other required cache type enums in qurt_types.h can also be passed.

                             NOTE: No default value for cache & perm is present. 
                             Client always needs to pass any of defined the flags.

 @param[out]  vaddr          Return pointer to the variable that holds the virtual address
 @param[out]  size           Return pointer for the virtual region size.

 @return
 #QURT_EOK                   Success indicating mapping created properly.
 #QURT_DT_MAPPING_FAILED     Failed to create mapping.
 #QURT_EINVALID              Mismatch in the architecture.

                             else FdtLib or thirdparty error code.

*/
int qurt_dt_mapping_create2(fdt_node_handle *devtreeNode, unsigned int dt_map_flags, 
                              char *regionName, int regionIdx, unsigned int dt_map_perm, int cache_attr, void **vaddr, size_t *size);

/**@ingroup func_qurt_dt_isr_register
  Device tree aware registration of an interrupt service routine (ISR) to an ISR thread. 
  The interrupt defined in the specified device tree node is enabled when this function returns success.

  @datatypes
  #qurt_thread_t \n
  #fdt_node_handle

  @param[in]   dt_node       Device tree node that specifies the interrupt property.
  @param[in]   dt_int_index  Index of the specific interrupt to use within the device tree node structure.
                             Specify either this or int_name, use -1 if string is used.
  @param[in]   dt_int_name   Name of the specific interrupt to use within the device tree node structure.
                             Either this or int_index should be specified, use NULL if index is used
  @param[in]   isr_thread_id ISR thread ID, returned from qurt_isr_create(), defined by qurt_isr_register2().  
  @param[in]   prio          Priority of the ISR, defined by qurt_isr_register2().
  @param[in]   flags         Defines ACK type. Values : \n
                             #QURT_INT_NON_DELAYED_ACK - ISR is acknowledged by the interrupt handle routine 
			                                     in the kernel.
                             #QURT_INT_DELAYED_ACK     - Client chooses to acknowledge.
                             Defined by qurt_isr_register2().             
  @param[in]   isr           ISR with proto type void isr (void *arg, int int_num), defined by qurt_isr_register2().
  @param[in]   arg  	     First argument of the ISR when it is called to service the interrupt, defined by qurt_isr_register2().
   
  @return 
  #QURT_EOK          -- Successfully registered the ISR for the interrupt \n
  #QURT_EINT         -- Interrupt not configured \n
  #QURT_EINVALID     -- Invalid thread ID \n
  #QURT_EDISABLED    -- The feature is disabled \n
  #QURT_EDUPLICATE   -- Interrupt is already registered

  @dependencies
   Create the thread ID qurt_isr_create().
   ISR registration completed with qurt_isr_register2().
 */
int qurt_dt_isr_register(fdt_node_handle *dt_node, int dt_int_index, char * dt_int_name, qurt_thread_t isr_thread_id, 
                         unsigned short prio, unsigned short flags, void (*isr) (void *, int), void *arg);

/**@ingroup func_qurt_dt_blob_id_get
 Returns the Blob ID for the Blob type passed.
 The value returned from this API can be passed as Blob ID parameter to DTBExtnLib APIs.

 @param[in] blob_type  Blob type to look up.
 @return Blob ID for the passed Blob Type.
*/
int qurt_dt_blob_id_get(unsigned int blob_type);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
