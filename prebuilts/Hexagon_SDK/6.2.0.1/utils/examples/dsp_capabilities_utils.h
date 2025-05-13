/**=============================================================================
@file
    dsp_capabilities_utils.h

@brief
    Wrapper functions for FastRPC Capability APIs.

Copyright (c) 2020-2021 Qualcomm Technologies Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
=============================================================================**/
#ifndef DSP_CAPABILITIES_UTILS_H
#define DSP_CAPABILITIES_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <stdbool.h>
#include "AEEStdErr.h"
#include "remote.h"

#if !defined (_WINDOWS)
    #pragma weak remote_system_request
#endif
 /**
 * Wrapper for FastRPC Capability API: query DSP support.
 *
 * @param[out]  domain pointer to supported domain.
 * @return      0          if query is successful.
 *              non-zero   if error, return value points to the error.
 */
int get_dsp_support(int *domain);

 /**
 * Wrapper for FastRPC Capability API: query VTCM information.
 *
 * @param[in]   domain value of domain in the queried.
 * @param[out]  capability capability value of the attribute queried.
 * @param[in]   attr value of the attribute to the queried.
 * @return      0          if query is successful.
 *              non-zero   if error, return value points to the error.
 */
int get_vtcm_info(int domain, uint32_t *capability, uint32_t attr);

 /**
 * Wrapper for FastRPC Capability API: query unsigned pd support on CDSP domain.
 *
 * @return      true          if unsigned pd is supported.
 *              false         if unsigned pd is not supported, capability query failed.
 */

bool get_unsignedpd_support(void);

 /**
 * Wrapper for FastRPC Capability API: query unsigned pd support.
 *
 * @param[in]   domain value of domain in the queried.
 * @return      true          if unsigned pd is supported.
 *              false         if unsigned pd is not supported, capability query failed.
 */

bool is_unsignedpd_supported(int domain_id);

 /**
 * is_valid_domain_id API: query a domain id is valid.
 *
 * @param[in]   domain value of domain in the queried.
 * @param[in]   compute_only value of domain is only compared with CDSP domains supported by the target when enabled.
 * @return      true          if value of domain is valid.
 *              false         if value of domain is not valid.
 */

bool is_valid_domain_id(int domain_id, int compute_only);

 /**
 * get_domain API: get domain struct from domain value.
 *
 * @param[in]  domain value of a domain
 * @return     Returns domain struct of the domain if it is supported or else
 *             returns NULL.
 *
 */

domain* get_domain(int domain_id);

 /**
 * get_domains_info API: get information for all the domains available on the device
 *
 * @param[in]  domain_type pointer to domain type
 * @param[in]  num_domains pointer to number of domains
 * @param[in]  domains_info pointer to save discovered domains information.
 * @return     0 if query is successful.
 *              non-zero if error, return value points to the error.
 *
 * It is user's responsibility to free the memory used to store the domains info whose address is present in domains_info before closing the application.
 *
 */

int get_domains_info(char *domain_type, int *num_domains, fastrpc_domain **domains_info);

  /**
 * is_async_fastrpc_supported API: query a domain id has async fastrpc supported or not
 *
 * @param[in]  domain_id value of a domain
 * @return     Returns true or false stating support of Async FastRPC
 *
 */

bool is_async_fastrpc_supported(int domain_id);

 /**
 * is_status_notification_supported API: query the DSP for STATUS_NOTIFICATION_SUPPORT information
 *
 * @param[in]  domain_id value of a domain
 * @return     Returns true or false stating status notification support information
 *
 */
bool is_status_notification_supported(int domain_id);

 /**
 * get_hmx_support_info API: query the DSP for HMX SUPPORT information
 *
 * @param[in]   domain_id value of a domain
 * @param[out]  capability capability value of the attribute queried.
 * @param[in]   attr value of the attribute to the queried.
 * @return      0 if query is successful.
 *              non-zero if error, return value points to the error.
 *
 */
int get_hmx_support_info(int domain, uint32_t *capability, uint32_t attr);

 /**
 * get_hex_arch_ver API: query the Hexagon processor architecture version information
 *
 * @param[in]   domain_id value of a domain
 * @param[out]  capability capability value of the attribute queried.
 *              The last byte of the capability value represents the architecture of the DSP being queried in hexadecimal format.
 *              Eg. 0x8D73 represents a v73 architecture. The other byte stands for other capabilities depending on the device.
 * @return      0 if query is successful.
 *              non-zero if error, return value points to the error.
 *
 */
int get_hex_arch_ver(int domain, uint32_t *capability);

 /**
 * get_hvx_support_info API: query the DSP for HVX SUPPORT information
 *
 * @param[in]   domain_id value of a domain
 * @param[out]  capability capability value of the attribute queried.
 * @param[in]   attr value of the attribute to the queried.
 * @return      0 if query is successful.
 *              non-zero if error, return value points to the error.
 *
 */
int get_hvx_support_info(int domain, uint32_t *capability, uint32_t attr);


#ifdef __cplusplus
}
#endif

#endif  //DSP_CAPABILITIES_UTILS_H
