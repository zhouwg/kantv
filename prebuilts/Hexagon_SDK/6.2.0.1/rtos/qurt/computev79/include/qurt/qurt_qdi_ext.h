#ifndef QURT_QDI_EXT_H
#define QURT_QDI_EXT_H

/**
  @file qurt_qdi_driver.h
  @brief  Definitions, macros, and prototypes used when writing a
  QDI driver

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2018, 2019-2021 by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/
#include "qurt_qdi_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

struct qurt_qdi_ext_device {
    qurt_qdi_ext_obj_info_ptr qurt_qdi_ext_obj_info_head;
    struct qurt_qdi_ext_device * next;
    char * instance;
    fdt_node_handle context;
};
typedef struct qurt_qdi_ext_device *qurt_qdi_ext_device_ptr;

/**@ingroup func_qurt_qdi_dt_register
 Registers a QDI device with the generic QDI object in the current QDI context,
 if and only if a compatible device node is found in the device tree. This 
 function serves as a device tree aware wrapper for qurt_qdi_devname_register().

 @param  name   Device name or device name prefix.
 @param  opener Pointer to QDI ext specialized opener object for the driver.

 @return
 0 -- Device was successfully registered. \n
 Negative error code -- Device was not registered.
*/
static __inline int qurt_qdi_dt_register(const char *name, qurt_qdi_obj_t *opener)
{
    return qurt_qdi_handle_invoke(QDI_HANDLE_GENERIC, QDI_DT_REGISTER, name, opener);
}

static inline void qurt_qdi_ext_deviceobj_set_name (struct qurt_qdi_ext_device * device, char * name)
{
    device->instance = name;
}

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
