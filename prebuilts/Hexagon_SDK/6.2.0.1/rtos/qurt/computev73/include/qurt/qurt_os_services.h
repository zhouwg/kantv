/*=============================================================================

                                    qurt_os_services.c

GENERAL DESCRIPTION

EXTERNAL FUNCTIONS
        None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
        None.

             Copyright (c) 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
=============================================================================*/

#define QURT_OS_SERVICE_THREAD                "/os/thread"				/**< Thread service */
#define QURT_OS_SERVICE_FS_HUB                "/os/fs_hub"  			/**< file-system hub */
#define QURT_OS_SERVICE_CALLBACK              "/os/callback"            /**< QDI callback service */ 
#define QURT_OS_SERVICE_INTERRUPTS            "/os/interrupt"           /**< Interrupt service */
#define QURT_OS_SERVICE_PROXY                 "/os/proxy"               /**< QDI proxy serice */
#define QURT_OS_SERVICE_MEMORY                "/os/memory"              /**< Memory management service */
#define QURT_OS_SERVICE_MEMPOOL               "/os/mempool"             /**< Pool management service */
#define QURT_OS_SERVICE_PROCESS               "/os/process"             /**< Process management service */
#define QURT_OS_SERVICE_MMAP                  "/os/mem_mapper"          /**< mmapper service */
