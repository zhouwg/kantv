#include <stdio.h>
#include <stdbool.h>
#include "AEEStdErr.h"
#include "remote.h"
#include "dsp_capabilities_utils.h"


 /**
 * request_status_notifications_enable API: Allow users to enable status notification from client PD.
 *
 * @param[in]  domain value of a domain
 * @param[in]  Context of the client
 * @param[in]  callback function for status notification
 * @return      0          if successful.
 *              non-zero   if error, return value points to the error.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
int request_status_notifications_enable(int domain_id, void *context, int(*notif_callback_fn)(void *context, int domain, int session, remote_rpc_status_flags_t status));
#ifdef __cplusplus
}
#endif
