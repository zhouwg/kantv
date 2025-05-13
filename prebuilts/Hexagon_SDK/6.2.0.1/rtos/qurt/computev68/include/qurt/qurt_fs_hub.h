#ifndef QURT_FS_HUB_H
#define QURT_FS_HUB_H

/**
  @file qurt_fs_hub.h
  @brief  Definitions, macros, and prototypes used when writing a
  QDI driver that provides file-system functionality.

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2023 by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
  This structure tracks a file-designator for a FS-hub QDI driver.
  File system's QDI interface should use this object to encapsulate
  true file-descriptor and return back a QDI handle. This QDI handle
  will be used as file-descriptor by File-systm-hub. 
 */

typedef struct qurt_qdi_fs_obj
{
    qurt_qdi_obj_t qdi_obj;
    int client_handle;
    int fd;
}qurt_qdi_fs_obj_t;


/**@ingroup fs_hub_support_functions
  This function allows a file-system to register it's QDI interface with file-system-hub.
  Once registered, all file open operations for any filenames containing the mountpoint will
  be forwarded to the QDI inteface.

  Mountpoint string must be encased in two forward slashes e.g. "/mountpoint/"

  @param  mtpoint         mount point for the file-system being registered.
  @param  opener          opener structure for the QDI driver interface
  
  @return
  QURT_EOK -- Successfully registered QDI driver with file-system-hub.
  Negative error code -- Failed to register with file-system-hub
 */
int qurt_fs_hub_mtpoint_register(const char *mtpoint, qurt_qdi_obj_t *opener);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
