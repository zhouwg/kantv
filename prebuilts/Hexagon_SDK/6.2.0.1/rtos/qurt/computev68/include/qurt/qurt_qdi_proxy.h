/*=============================================================================

                                    qurt_qdi_proxy.h

GENERAL DESCRIPTION

EXTERNAL FUNCTIONS
        None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
        None.

Copyright (c) 2013, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
=============================================================================*/
#ifndef _QURT_QDI_PROXY_H
#define _QURT_QDI_PROXY_H

#include "qurt_qdi_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* APIs allowing operation on the proxy object directly */
int qurt_qdi_proxy_ref_create(void);

/* APIs allowing to operate on proxy given a known proxy handle 
 * 1) using qdi handle of the object 
 * successful return: QURT_EOK, anything else -- failure 
 */
int qurt_qdi_proxy_ref_add_by_handle(int proxy_handle, int qdi_handle);
int qurt_qdi_proxy_ref_sub_by_handle(int proxy_handle, int qdi_handle);

/* 2) using object reference 
 * successful return: QURT_EOK, anything else -- failure 
 */
int qurt_qdi_proxy_ref_add_by_object(int proxy_handle, qurt_qdi_obj_t *obj_ptr);
int qurt_qdi_proxy_ref_sub_by_object(int proxy_handle, qurt_qdi_obj_t *obj_ptr);

/* API allowing to associate a proxy object with a particular client given a client handle 
 * successfule return: QURT_EOK, anything else -- failure 
 */
int qurt_client_proxy_ref_install (int client_handle, int proxy_handle);

/* APIs allowing operation on proxy object from user client 
 * successful return: QURT_EOK, anything else -- failure 
 */
int qurt_client_proxy_ref_add(int qdi_handle);
int qurt_client_proxy_ref_remove(int qdi_handle);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_QDI_PROXY_H */
