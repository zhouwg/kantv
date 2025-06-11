/*-----------------------------------------------------------------------
   Copyright (c) 2022 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------*/

/**
 *  @file HAP_etm_config.h
 *  @brief Header file with APIs to enable/disable etm tracing
 */

#include "AEEStdErr.h"

#ifdef __cplusplus
extern "C" {
#endif

 /**
 * @cond DEV
 */

int __attribute__((weak)) __HAP_user_etm_enable(void);
int __attribute__((weak)) __HAP_user_etm_disable(void);

/**
 * @endcond
 */


/** @defgroup helperapi Helper APIs to enable/disable etm trace.
 *  API for users to enable or disable ETM tracing.
 *  The HAP user ETM API provides user capability to start/stop
 *  ETM tracing in a user module to cover a desired portion of
 *  execution. This API is disabled by default and will return
 *  an error when in that mode. To enable it, use
 *  --hap_etm_enable option of sysMonApp etmTrace service as
 *  mentioned in the sample command for default subsystem CDSP below:
 *  ```
 *  adb shell /data/local/tmp/sysMonApp etmTrace --command etm --hap_etm_enable 1
 *  ```
 *  ETM enablement requires setting up coresight driver on HLOS
 *  and configuring appropriate ETM trace type on Q6 subsystem.
 *  ETM configurations set via sysMonApp etmTrace option
 *  like etm tracing mode (cycle accurate PC tracing etc.,
 *  sample command on CDSP below)
 *  ```
 *  adb shell /data/local/tmp/sysMonApp etmTrace --command etm --etmType ca_pc
 *  ```
 *  are preserved across HAP user etm enable and disable calls.
 *  The API is only for debug purpose and shouldn't be used in
 *  production environments.
 *  @{
 */

/**
 * Requests ETM tracing to be enabled
 *
 * Call this function from the DSP user process to start ETM
 * tracing. To stop the tracing, call @ref HAP_user_etm_disable().
 * Supported on latest chipsets(released after Palima).
 * @param None
 * @return 0 upon success, other values upon failure.
 */
static inline int HAP_user_etm_enable(void) {
    if(__HAP_user_etm_enable)
        return __HAP_user_etm_enable();
    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * Requests ETM tracing to be disabled
 *
 * Call this function from the DSP user process to stop any active
 * ETM tracing. API returns error if there is no active ETM trace
 * enable call, e.g., if @ref HAP_user_etm_disable() is called
 * first without any active @ref HAP_user_etm_enable() being
 * present. The enable and disable requests are reference counted
 * in the driver. Nested calls are supported, e.g.
 * if @ref HAP_user_etm_enable() is called twice, two calls
 * to the disable API @ref HAP_user_etm_disable() will be needed
 * to disable the tracing.
 * Supported on latest chipsets(released after Palima).
 * @param None
 * @return 0 upon success, other values upon failure.
 */
static inline int HAP_user_etm_disable(void) {
    if(__HAP_user_etm_disable)
        return __HAP_user_etm_disable();
    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
