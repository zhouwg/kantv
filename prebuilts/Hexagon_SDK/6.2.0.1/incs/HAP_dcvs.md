# DCVS Helper APIs

DCVS Duty Cycle Helper APIs with usage examples.

## DCVS Duty Cycle Helper APIs

Header file: @b HAP_dcvs.h

## Usage examples

### HAP_set_dcvs_v3_duty_cycle

This is the most straightforward and therefore the recommended simplified API to enable DCVS Duty Cycle.

The user has to pass the context (power client identifier to be used in a HAP_power_set call) to this API.
The function calls into HAP_power_set API with the provided context and selects DCVS duty cycle mode via `HAP_power_set_DCVS_v3` request type.
The user optionally can provide max_active_time and periodicity. The DCVS algorithm selects appropriate operating levels to keep the activity time within the provided
maximum allowed time and uses periodicity as a hint while predicting activity.

The user does not need to specify any clock corners. Instead, the DCVS algorithm will select the appropriate clock corner with the best performance-power tradeoff that keeps the active time under the maximum value provided by the user for a given period.

The example below demonstrates the usage of HAP_set_dcvs_v3_duty_cycle API.

@code
    /*
     * Enabling DCVS Duty Cycle with 10ms max_active_time and 33ms periodicity
     */
    HAP_set_dcvs_v3_duty_cycle(context, 10, 33);
@endcode

Here DCVS Duty cycle starts with NOM as active corner and LOW SVS (SVS2) corner for idle cycle. Then, if the DCVS algorithm observes an active time longer than 10 ms (the user-defined max active time), it will increase the clock to the next level, NOM PLUS, to try bringing the active time under 10ms.

![screenshot](../../images/HAP_set_dcvs_v3_duty_cycle.png)

### HAP_set_dcvs_v3_duty_cycle_params

This API is useful in setting the max_active_time and periodicity in an existing DCVS request structure.

The user can set the DCVS params as per application requirement in DCVS request structure with request type set to `HAP_power_set_DCVS_v3` and pass it as an argument to this function.

This API allows the user to set the maximum active time and period values used by the DCVS algorithm. After invoking this function, the user will have to call HAP_power_set() using the same request structure.

@code
    HAP_power_request_t request;
    request.type = HAP_power_set_DCVS_v3;
    /*
     * Selecting Duty Cycle mode with DCVS enabled
     */
    request.dcvs_v3.set_dcvs_enable = TRUE;
    request.dcvs_v3.dcvs_enable = TRUE;
    request.dcvs_v3.dcvs_option = HAP_DCVS_V2_DUTY_CYCLE_MODE;
    /*
     * Setting TURBO PLUS as Max corner, NOM PLUS as Target corner
     * and LOW SVS as Min corner
     */
    request.dcvs_v3.set_core_params = TRUE;
    request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_SVS2;
    request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
    request.dcvs_v3.core_params.target_corner = HAP_DCVS_VCORNER_NOM_PLUS;
    request.dcvs_v3.set_bus_params = TRUE;
    request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_SVS2;
    request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
    request.dcvs_v3.bus_params.target_corner = HAP_DCVS_VCORNER_NOM_PLUS;
    /*
     * Setting 20ms max_active_time and 33ms periodicity
     */
    HAP_set_dcvs_v3_duty_cycle_params(&request, 20, 33);
    HAP_power_set(context, &request);
@endcode

Here DCVS duty cycle apply LOW SVS (SVS2) for idle cycle and active cycle corner in the range of Max to Target (TURBO PLUS to NOM PLUS) to maintain the user given max_active_time (20ms).

![screenshot](../../images/HAP_set_dcvs_v3_duty_cycle_params.png)

### HAP_set_dcvs_v3_core_perf_mode

This API helps to select core clock frequency level within target voltage corner.

By default, the highest core clock frequency available at the requested target corner is selected. Using this API, the user can select either the highest (`HAP_DCVS_CLK_PERF_HIGH`) or the lowest (`HAP_DCVS_CLK_PERF_LOW`) core clock frequency at any given target corner. If there is only one core clock frequency available at the requested target corner, both the
high and low settings will select the same.

The user can set the DCVS params as per application requirement in DCVS request structure with request type set to `HAP_power_set_DCVS_v3` and pass the same as an arguement to this function along with perf_mode arguement which specifies the core clock frequency level (`HAP_DCVS_CLK_PERF_HIGH/HAP_DCVS_CLK_PERF_LOW`).

This API sets the user provided perf_mode for core clock in the given request structure. After invoking this function, the user will have to call HAP_power_set() using the same request structure.

@code
    HAP_power_request_t request;
    request.type = HAP_power_set_DCVS_v3;
    /*
     * Setting TURBO as Max corner, NOM as Target corner
     * and LOW SVS as Min corner for core clock
     */
    request.dcvs_v3.set_core_params = TRUE;
    request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_SVS2;
    request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO;
    request.dcvs_v3.core_params.target_corner = HAP_DCVS_VCORNER_NOM;
    /*
     * Setting perf_mode as HAP_DCVS_CLK_PERF_LOW
     */
    HAP_set_dcvs_v3_core_perf_mode(&request, HAP_DCVS_CLK_PERF_LOW);
    HAP_power_set(context, &request);
@endcode

Here DCVS will vote for minimum available core clock frequency at NOM target corner.

### HAP_set_dcvs_v3_bus_perf_mode

This API helps to select bus clock frequency level within target voltage corner.

By default, the highest bus clock frequency available at the requested target corner is selected. Using this API, the user can select either the highest (`HAP_DCVS_CLK_PERF_HIGH`) or the lowest (`HAP_DCVS_CLK_PERF_LOW`) bus clock frequency at any given target corner. If there is only one bus clock frequency available at the requested target corner, both the high and
low settings will select the same.

The user can set the DCVS params as per application requirement in DCVS request structure with request type set to `HAP_power_set_DCVS_v3` and pass the same as an arguement to this function along with perf_mode arguement which specifies the bus clock frequency level (`HAP_DCVS_CLK_PERF_HIGH/HAP_DCVS_CLK_PERF_LOW`).

This API sets the user provided perf_mode for bus clock in the given request structure. After invoking this function, the user will have to call HAP_power_set() using the same request structure.

@code
    HAP_power_request_t request;
    request.type = HAP_power_set_DCVS_v3;
    /*
     * Setting TURBO PLUS as Max corner, TURBO as Target corner
     * and LOW SVS as Min corner for bus clock
     */
    request.dcvs_v3.set_bus_params = TRUE;
    request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_SVS2;
    request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
    request.dcvs_v3.bus_params.target_corner = HAP_DCVS_VCORNER_TURBO;
    /*
     * Setting perf_mode as HAP_DCVS_CLK_PERF_LOW
     */
    HAP_set_dcvs_v3_bus_perf_mode(&request, HAP_DCVS_CLK_PERF_LOW);
    HAP_power_set(context, &request);
@endcode

Here DCVS will vote for minimum available bus clock frequency at TURBO target corner.

### HAP_set_dcvs_v3_protected_bus_corners

On chipsets supporting bus corners above `HAP_DCVS_VCORNER_TURBO_PLUS`, to optimize residency at these corners, target corner requests for bus are capped to `HAP_DCVS_VCORNER_TURBO_PLUS` by default.
Any request beyond `HAP_DCVS_VCORNER_TURBO_PLUS` (including `HAP_DCVS_VCORNER_MAX`) will be set to `HAP_DCVS_VCORNER_TURBO_PLUS`.

This API enables clients of HAP_power_set to override this protection when voting explicitly for bus corners above `HAP_DCVS_VCORNER_TURBO_PLUS` in necessary use cases.

Note:
This API is supported starting with V79 QDSP6 architecture, `AEE_EVERSIONNOTSUPPORT` error (can be safely ignored) is returned by the API when not supported.
Request type should be set to `HAP_power_set_DCVS_v3`.
