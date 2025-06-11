/*-----------------------------------------------------------------------------
   Copyright (c) 2021, 2022 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------------*/

#ifndef HAP_DCVS_H_
#define HAP_DCVS_H_

/**
 *  @file HAP_dcvs.h
 *  @brief Header file for DCVS APIs.
 */

#include "AEEStdErr.h"
#include "HAP_power.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Perf modes to specify core/bus clock frequency level within
 *  target voltage corner for HAP DCVS V3 interface.
 */
typedef enum {
	HAP_DCVS_CLK_PERF_HIGH, /**< To select max frequency at target voltage corner. */
	HAP_DCVS_CLK_PERF_LOW,  /**< To select min frequency at target voltage corner. */
} HAP_dcvs_clk_perf_mode_t;

/**
 * @cond DEV
 */
int __attribute__((weak)) sysmon_set_dcvs_v3_duty_cycle(
                                        void* context,
                                        uint32 max_active_time,
                                        uint32 periodicity);

int __attribute__((weak)) sysmon_set_dcvs_v3_duty_cycle_params(
                                                HAP_power_request_t* request,
                                                uint32 max_active_time,
                                                uint32 periodicity);

int __attribute__((weak)) sysmon_set_dcvs_v3_core_perf_mode(
                                        HAP_power_request_t* request,
                                        HAP_dcvs_clk_perf_mode_t perf_mode);

int __attribute__((weak)) sysmon_set_dcvs_v3_bus_perf_mode(
                                        HAP_power_request_t* request,
                                        HAP_dcvs_clk_perf_mode_t perf_mode);

int __attribute__((weak)) sysmon_set_dcvs_v3_protected_bus_corners(
                                        HAP_power_request_t* request,
                                        unsigned char enable_protected_corners);

int __attribute__((weak)) sysmon_set_ddr_perf_mode(
                                        HAP_power_request_t *request,
                                        unsigned int perf_mode);
/**
 * @endcond
 */

/**
 * @defgroup helperapi Helper APIs for DCVS Duty Cycle
 * @{
 */

/**
 * Method to enable DCVS Duty Cycle.
 *
 * Calls HAP_power_set API with the provided context and selects
 * DCVS duty cycle mode via HAP_power_set_DCVS_v3 request type.
 *
 * @param[in] context User context - power client identifier to be used in
 *                                   HAP_power_set call.
 *
 * @param[in] max_active_time Max active time allowed per frame in ms
 *                            (optional, can pass 0 if don’t want to specify).
 *                            DCVS selects appropriate operating levels to
 *                            keep the activity time within the provided
 *                            maximum allowed time.
 *
 * @param[in] periodicity Frame time in ms (optional, can pass 0 if
 *                        don’t want to specify periodicity). For example,
 *                        periodicity = 100 (milli-seconds) for a
 *                        10 FPS activity. DCVS uses this as a hint while
 *                        predicting activity.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * AEE_EVERSIONNOTSUPPORT if unsupported.
 */
static inline int HAP_set_dcvs_v3_duty_cycle(
                                        void* context,
                                        uint32 max_active_time,
                                        uint32 periodicity)
{
    if (sysmon_set_dcvs_v3_duty_cycle)
        return sysmon_set_dcvs_v3_duty_cycle(
                                        context,
                                        max_active_time,
                                        periodicity);

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * Method to set duty cycle threshold params (periodicity and activity time hints)
 * in the request structure intended for HAP_power_set for request type set to
 * HAP_power_set_DCVS_v3.
 *
 * Sets the max_active_time and periodicity fields under dcvs_v3 payload of given
 * request structure.
 *
 * Note: Request type should be set to HAP_power_set_DCVS_v3.
 *
 * @param[in] request Pointer to request structure.
 *
 * @param[in] max_active_time Max active time allowed per frame in ms.
 *                            DCVS selects appropriate operating levels to
 *                            keep the activity time within the provided
 *                            maximum allowed time.
 *
 * @param[in] periodicity Frame time in ms (optional, can pass 0 if
 *                        don’t want to specify periodicity). For example,
 *                        periodicity = 100 (milli-seconds) for a
 *                        10 FPS activity. DCVS uses this as a hint while
 *                        predicting activity.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * AEE_EVERSIONNOTSUPPORT if unsupported.
 */
static inline int HAP_set_dcvs_v3_duty_cycle_params(
                                        HAP_power_request_t* request,
                                        uint32 max_active_time,
                                        uint32 periodicity)
{
    if (sysmon_set_dcvs_v3_duty_cycle_params)
    {
        return sysmon_set_dcvs_v3_duty_cycle_params(
                                            request,
                                            max_active_time,
                                            periodicity);
    }

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * @}
 */

/**
 * @defgroup enable_protected_corner_api Helper API for protected bus corners
 *
 * @{
 */
/**
 * On chipsets supporting bus corners above HAP_DCVS_VCORNER_TURBO_PLUS, to optimize residency at these corners,
 * target corner requests for bus are capped to HAP_DCVS_VCORNER_TURBO_PLUS by default.
 * Any request beyond HAP_DCVS_VCORNER_TURBO_PLUS (including HAP_DCVS_VCORNER_MAX) will be wrapped to HAP_DCVS_VCORNER_TURBO_PLUS.
 *
 * This API enables clients of HAP_power_set to override this protection when voting explicitly for bus corners
 * above HAP_DCVS_VCORNER_TURBO_PLUS in necessary use cases.
 *
 * Note:
 * API is supported starting with V79 QDSP6 architecture, AEE_EVERSIONNOTSUPPORT error (can be safely ignored) is returned by the API when not supported.
 *
 * Request type should be set to HAP_power_set_DCVS_v3.
 *
 * @param[in] request    Pointer to HAP_power_request_t structure with request type set to HAP_power_set_DCVS_v3.
 * @param[in] enable_protected_corners   1 - to consider bus corner requests above HAP_DCVS_VCORNER_TURBO_PLUS
 *                                       0 (default) - to cap bus corner requests to HAP_DCVS_VCORNER_TURBO_PLUS
 * @return
 *   0 upon success. \n
 *   Nonzero upon failure. \n
 *   AEE_EVERSIONNOTSUPPORT if unsupported.
 */

static inline int HAP_set_dcvs_v3_protected_bus_corners(
                                            HAP_power_request_t* request,
                                            unsigned char enable_protected_corners)
{
    if (sysmon_set_dcvs_v3_protected_bus_corners)
    {
        return sysmon_set_dcvs_v3_protected_bus_corners(request,
                                                enable_protected_corners);
    }

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * @}
 */
/**
 * @defgroup enable_ddr_perf_mode_api Helper API to enable DDR perf mode
 *
 * @{
 */
/**
 * This API enables clients of HAP_power_set to vote for DDR performance mode.
 *
 * Note:
 * API is supported starting with V79 QDSP6 architecture, AEE_EVERSIONNOTSUPPORT error (can be safely ignored) is returned by the API when not supported.
 *
 * Note: Request type should be set to HAP_power_set_DCVS_v3.
 *
 * @param[in] request Pointer to HAP_power_request_t structure with request type set to HAP_power_set_DCVS_v3
 *
 * @param[in] perf_mode  1 - to enable DDR performance mode
 *                       0 - to disable the DDR performance mode
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * AEE_EVERSIONNOTSUPPORT if unsupported.
 */
static inline int HAP_set_ddr_perf_mode(
                                    HAP_power_request_t *request,
                                    unsigned int perf_mode)
{
    if (sysmon_set_ddr_perf_mode)
    {
        return sysmon_set_ddr_perf_mode(request, perf_mode);
    }

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * @}
 */

/**
 * @defgroup clk_perfmode_api APIs to specify core/bus clock frequency level within target voltage corner
 *
 * @{
 */

/**
 * Method to specify core clock frequency level corresponding to the
 * target corner request in the request structure intended for
 * HAP_power_set for request type set to HAP_power_set_DCVS_v3.
 *
 * By default, the highest core clock frequency available at the requested
 * target_corner is selected. Using this API, user can select either the
 * highest (HAP_DCVS_CLK_PERF_HIGH) or the lowest (HAP_DCVS_CLK_PERF_LOW)
 * core clock frequency at any given target_corner. If there is only one
 * core clock frequency available at the requested target_corner, both the
 * high and low settings will select the same.
 *
 * Note: Request type should be set to HAP_power_set_DCVS_v3.
 *
 * Supported on latest chipsets(released after Palima).
 *
 * @param[in] request Pointer to request structure.
 *
 * @param[in] perf_mode Perf mode to specify core clock frequency level
 *                      within target voltage corner.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * AEE_EVERSIONNOTSUPPORT if unsupported.
 */
static inline int HAP_set_dcvs_v3_core_perf_mode(
                                        HAP_power_request_t* request,
                                        HAP_dcvs_clk_perf_mode_t perf_mode)
{
    if (sysmon_set_dcvs_v3_core_perf_mode)
    {
        return sysmon_set_dcvs_v3_core_perf_mode(
                                            request,
                                            perf_mode);
    }

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * Method to specify bus clock frequency level corresponding to the
 * target corner request in the request structure intended for
 * HAP_power_set for request type set to HAP_power_set_DCVS_v3.
 *
 * By default, the highest bus clock frequency available at the requested
 * target_corner is selected. Using this API, user can select either the
 * highest (HAP_DCVS_CLK_PERF_HIGH) or the lowest (HAP_DCVS_CLK_PERF_LOW)
 * bus clock frequency at any given target_corner. If there is only one
 * bus clock frequency available at the requested target_corner, both the
 * high and low settings will select the same.
 *
 * Note: Request type should be set to HAP_power_set_DCVS_v3.
 *
 * Supported on latest chipsets(released after Palima).
 *
 * @param[in] request Pointer to request structure.
 *
 * @param[in] perf_mode Perf mode to specify bus clock frequency level
 *                      within target voltage corner.
 *
 * @return
 * 0 upon success. \n
 * Nonzero upon failure. \n
 * AEE_EVERSIONNOTSUPPORT if unsupported.
 */
static inline int HAP_set_dcvs_v3_bus_perf_mode(
                                        HAP_power_request_t* request,
                                        HAP_dcvs_clk_perf_mode_t perf_mode)
{
    if (sysmon_set_dcvs_v3_bus_perf_mode)
    {
        return sysmon_set_dcvs_v3_bus_perf_mode(
                                            request,
                                            perf_mode);
    }

    return AEE_EVERSIONNOTSUPPORT;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif //HAP_DCVS_H_
