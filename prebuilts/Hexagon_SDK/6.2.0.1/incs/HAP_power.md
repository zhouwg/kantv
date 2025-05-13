# Introduction {#intro}

The Hexagon SDK provides APIs to control DSP core and bus clocks based on power and performance needs.
By default, every compute session votes for NOMINAL voltage corner and powers on HVX.
Clients can choose to overwrite this by HAP power APIs below.

# HAP_power API {#api}


## API Overview {#api-overview}

HAP_power_* APIS are used by clients to override the power settings of the DSPs according to their needs. This API is supported on ADSP, CDSP and SLPI.

The HAP power API contains a set of interfaces that allow programmers to adjust the DSP power usage as per the application's power requirement, thereby providing a good balance between power consumption and performance.

* HAP_power_set(): This is used to vote for performance levels on the DSP
* HAP_power_get(): This is used to query the DSP for current performance levels
* HAP_power_destroy(): This is used to destroy power clients created through HAP_power_set API

::HAP_power_set can be used to control these parameters on the DSP:
* DSP MIPS
* Bus speed / bandwidth
* Dynamic scaling of bus and DSP clocks and bus speeds (DCVS)
* Application type (client class), more details on this can be found [here](#app-type)
* L2 cache line locking
* Hexagon Vector eXtension (HVX) blocks

::HAP_power_get can be used to query the DSP for these parameters:
* Max MIPS supported
* Max bus speed / bandwidth supported
* Current Core clock speed
* Current application type (client class)
* Aggregate Mpps used by audio and voice

::HAP_power_destroy can be used to destroy the power clients created through HAP_power_set API.
  Destroys any existing HAP_power votes associated with the provided client context, and disassociates that context from HAP_power.


## Usage {#usage}

See HAP_power.h for more information on this API.

::HAP_power_set : This accepts two parameters
* context - Unique identifier (explained below)
* request - The power request.

context is a unique identifier (in the scope of the PD) provided by the user to identify an independent voting client of HAP_power. For each unique context passed in a HAP_power_set invocation, HAP_power adds a new client to its state to be associated with that context.

On targets after Lahaina, helper APIs HAP_utils_create_context and HAP_utils_destroy_context are added to create and destroy unique context identifiers. If these are not available, the recommended alternative is to create a context by allocating a dummy byte and using the pointer value as the context, and freeing that byte later after destroying the context's associated HAP_power client via HAP_power_destroy.
* HAP_utils_create_context(): This is used to create a unique context identifier
* HAP_utils_destroy_context(): This is used to destroy unique context identifier. HAP_utils_destroy_context should only be called on a context after destroying the HAP_power client associated to that context, via HAP_power_destroy(context). Failure to destroy both in the proper order may cause a leak.

Refer to the following table for voting/unvoting call flow:
<table>
<tr><th>Voting/Unvoting call flow<th>Library code
<tr><td>Create unique client context<td>context = userLibCodeToCreateUniqueContext() (or) context = HAP_utils_create_context()
<tr><td>Create power client and vote<td>HAP_power_set(context, request)
<tr><td>Destroy power client<td>HAP_power_destroy(context) (or) HAP_power_destroy_client(context)
<tr><td>Destroy unique client context<td>userLibCodeToDestroyUniqueContext(context) (or) HAP_utils_destroy_context(context)
</table>

NOTE: Using a context set to NULL has specific implications, discussed below in [default voting](#default_voting)

Example: Module1 and Module2 are two different clients running in the same user PD on DSP. Module1 creates a new, unique client context and votes for its needs. Module2 also creates a new, unique client context and votes for its needs. The figure below shows the different client contexts and their votes to power manager.

![screenshot](../../images/hap_power.png)

The type in the request is set to one of:
* HAP_power_set_mips_bw: Used to set MIPS and / or bus speed (bandwidth). The payload in this case should contain HAP_power_mips_bw_payload.
* HAP_power_set_HVX: Used to enable / disable power for HVX. The payload in this case should contain HAP_power_hvx_payload.
* HAP_power_set_apptype: Used to set the application type. The payload in this case should contain ::HAP_power_app_type_payload.
* HAP_power_set_linelock: Used to line lock memory in the L2 cache. The payload in this case should contain HAP_power_linelock_payload.
* HAP_power_set_DCVS: Used to participate / stop participating in DCVS. The payload in this case should contain HAP_power_dcvs_payload.
* HAP_power_set_DCVS_v2: Enhanced version of HAP_power_set_DCVS with more options. The payload in this case should contain HAP_power_dcvs_v2_payload.
* HAP_power_set_DCVS_v3: Enhanced version of HAP_power_set_DCVS_v2 with more options to select core and bus operating corners separately. The payload in this case should contain HAP_power_dcvs_v3_payload.

NOTE:
* More details on HAP_power_set_DCVS_v2 can be found [here](#DCVS_V2).
* HAP_power_set_DCVS_v3 is supported from SM8250 onwards. More details can be found [here](#DCVS_V3).
* In Older targets, maximum of 8 clients can be created per PD, (including the default client). This limitation has been removed from SM8250 onwards.
* HAP_power_hmx_payload_v2 is supported starting with v75. On chipsets (v75 onwards) without separate HMX clock plan, requests made for target corner or
frequency will return AEE_EBADPARM (invalid parameter) error.
* HAP_power_set_CENG_bus is supported from v75 onwards. On chipsets (v75 onwards) not supporting independent Q6-CENG bus clock scaling, this request type
will return AEE_EBADPARM (invalid parameter) error.

Example is provided below.

~~~{.c}
	//Vote
	/* Populate request structure */
	int retVal;
	HAP_power_request_t request;
	memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
	request.type = HAP_power_set_DCVS_v2;
	request.dcvs_v2.dcvs_enable = TRUE;
	request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
	request.dcvs_v2.set_latency = TRUE;
	request.dcvs_v2.latency = 1000;
	request.dcvs_v2.set_dcvs_params = TRUE;
	request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_SVS;
	request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_TURBO;
	request.dcvs_v2.dcvs_params.target_corner = HAP_DCVS_VCORNER_NOM;
	/* Call HAP_power_set API with the updated request structure */
	/* cv is a global variable or an address in heap to uniquely identify the clients */
	retVal = HAP_power_set(&cv, &request);
	...
~~~
::HAP_power_get : This accepts two parameters

* context - this parameter is ignored and response is for the system level
* response - The power response

The type in the request is set to one of
* HAP_power_get_max_mips: Used to query for maximum MIPS supported
* HAP_power_get_max_bus_bw: Used to query for maximum bus bandwidth supported
* HAP_power_get_client_class: Used to query for current application type.
* HAP_power_get_clk_Freq: Used to query for current core clock frequency.
* HAP_power_get_aggregateAVSMpps: Used to query for aggregate Mpps used by audio and voice.

::HAP_power_destroy : This accepts one parameter

* context - the unique client context identifying the HAP_power client to destroy.

Example to remove default vote for a PD and to destroy default client.
~~~{.c}
	//Vote
	int nErr= 0;

	if(0 == (nErr = HAP_power_destroy(NULL))){
	   //Client destroyed successfully
	}
~~~

### Default voting {#default_voting}
In older targets, NULL is a valid HAP_power context that is used by FastRPC to establish a default vote for some reasonable clock settings. In order to override the default vote on these targets, it is necessary for a client to place a canceling vote using the NULL context.

Permitting clients to use NULL context can lead to conflicts where multiple clients in the same PD may try to independently manage the NULL context.
To address these conflicts, support for any NULL context voting has been removed starting in targets after Lahaina. For these targets, behavior of default voting has been changed. A suitable vote is placed automatically at opportune times (such as startup or object loading) on a unique context, and is automatically removed when no longer needed. For example, the FastRPC driver places a high clock vote when a new session is started on the DSP, and removes it as soon as any user in that session places any other HAP_power vote. This ensures clocks are high during startup and object loading, up until the point the user application is able to place its own vote.

The recommended client behavior is as follows:

* Lahaina and older targets: While it is allowed to rely on the default vote to establish reasonable clocks, it is recommended to place a canceling vote (with low/zero clock values as shown below) on the NULL context, plus an active vote for the requirements on a different unique context.

* Targets after Lahaina: Simply place an active vote for required clock settings on a unique context.

If a single client implementation is required to work correctly on all targets, the recommendation is as follows:
* Make an attempt to place a canceling NULL-context vote. If the error code AEE_ENULLCONTEXT is returned, it means the target does not support NULL context. Thus, this error can be ignored.
* Place a (non-NULL) unique context vote for the clock requirements.

On targets that support NULL context default voting, it can be removed using HAP_power_destroy(NULL) or as follows:

~~~{.c}
req.type = HAP_power_set_DCVS_v2;
req.dcvs_v2.dcvs_enable = FALSE;
req.dcvs_v2.set_latency = FALSE;
req.dcvs_v2.set_dcvs_params = FALSE;
VERIFY(AEE_SUCCESS == (nErr = HAP_power_set(NULL, &req)));
~~~


### Application type/Client class {#app-type}
HAP_power_set() API exposes an API for users to register for an application type.

'apptype' in ::HAP_power_request_t passed as parameter to  HAP_power_set request allows user to register an application as one of the client classes available in ::HAP_power_app_type_payload.
Setting an appropriate client class can be important as this information is used in DSP DCVS, QoS, DSP power management drivers. HAP_power clients who do not explicitly vote their apptype are treated as general compute applications, which is appropriate for most cases.

####Users of Client class information
DCVS selects HAP_DCVS_V2_POWER_SAVER_MODE as default DCVS policy for COMPUTE and STREAMING class clients. Client can always power their own DCVS policy by issuing a DCVS_v2 request, click [here](#DCVS_V2) for more details on DCVS_v2 request type of HAP_power_set.
QoS driver modifies L2 scoreboard thresholds on detecting STREAMING class clients to allow DSP L2 slave accesses.


###DSP DCVS v2 HAP interface {#DCVS_V2}
Based on user configuration, DCVS module in DSP (ADSP/CDSP) can adjust the core and bus clock frequencies based on core and bus usage metrics captured by SysMon. The existing DCVS interface via HAP_power_set() (type: HAP_power_set_DCVS) only allows users to vote for DCVS participation with 2 different options. DSP DCVS v2 algorithm exposes an enhanced set of DCVS options for diversified clients and a simplified voltage corner based voting scheme. On supported targets (8998 and latest), these new DCVS options and voting scheme are exposed to clients via HAP_power_set()(type: HAP_power_set_DCVS_v2).

####HAP API Support
The HAP_power_set API is enhanced to support the new mode registrations with DSP DCVS logic. Following table illustrates the new type of request and the new dcvs_v2 request structure associated with it.

<table>
<tr><th> API <td colspan="3"> HAP_power_set (void* context, HAP_power_request_t* request)
<tr><th> context <td colspan="3"> Explained [here](#usage). Votes across all contexts will be aggregated accordingly.
<tr><th rowspan="7"> request <td> type <td> HAP_power_set_DCVS_v2 <td> This new request type allows user to request via the new dcvs_v2 request structure.
<tr><td rowspan="6"> dcvs_v2 <td> dcvs_enable <td> DCVS participation flag
<tr><td> dcvs_option <td> These options instruct DCVS algorithm to use a pre-defined set of thresholds and operation logic based on the selected option.
<tr><td> set_latency <td> Latency vote validity flag. If FALSE then default sleep latency vote of 65535 micro seconds will be considered.
<tr><td> latency <td> Sleep latency vote in micro seconds. Valid when the set_latency flag is set to TRUE
<tr><td> set_dcvs_params <td> DCVS params validity flag. If FALSE then all parameters of dcvs_params will be set to default zero.
<tr><td> dcvs_params <td> DCVS params structure with flexibility to set upper and lower DCVS thresholds and also vote for core and bus clocks using a voltage corner.
</table>

###DSP DCVS v3 HAP interface {#DCVS_V3}
Based on user configuration, DCVS module in DSP can adjust the core and bus clock frequencies based on core and bus usage metrics captured by SysMon. The existing DCVS v2 algorithm via HAP_power_set() (type: HAP_power_set_DCVS_v2) exposes multiple DCVS options for diversified clients and a simplified voltage corner based voting scheme. But along with existing features, DCVS v3 provides separate voltage corner voting option to user for core and bus clock and also option to disable all low power modes without explicit sleep latency vote need. In scenarios where user is ok for same voltage corner voting for core and bus clock then they can still use DCVS v2. Also, in DCVS v3 user can vote for individual field/multiple fields based on his requirement. On supported targets (SM8250 and latest), these new DCVS options and voting scheme are exposed to clients via HAP_power_set() (type: HAP_power_set_DCVS_v3). Also, added wrapper functions built around same HAP_power_set() (type: HAP_power_set_DCVS_v3) to help user to select and vote for individual functionality in DCVS v3 without bothering about DCVS v3 structure and related details. This document captures information on these new DCVS v3 features and ways to use them.

####HAP API Support
The HAP_power_set API is enhanced to support the new user options with DCVS v3 with the new request type HAP_power_set_DCVS_v3 with HAP_power_dcvs_v3_payload.

<table>
<tr><th> API <td colspan="3"> HAP_power_set (void* context, HAP_power_request_t* request)
<tr><th> context <td colspan="3">  Explained [here](#usage). Votes across all contexts will be aggregated accordingly.
<tr><th rowspan="14"> request <td> type <td> HAP_power_set_DCVS_v3 <td> This new request type allows user to request via the new dcvs_v3 request structure
<tr><td rowspan="13"> dcvs_v3 <td> set_dcvs_enable <td> DCVS participation validity flag. If FALSE then the dcvs_enable and dcvs_option fields will be ignored.
<tr><td> dcvs_enable <td> DCVS participation flag. Vaild when the set_dcvs_enable is set to TRUE.
<tr><td> dcvs_option <td> These options instruct DCVS algorithm to use a pre-defined set of thresholds and operation logic based on the selected option.
<tr><td> set_latency <td> Latency vote validity flag. If FALSE then the latency field will be ignored.
<tr><td> latency <td> sleep latency vote in micro seconds. Valid when the set_latency flag is set to TRUE
<tr><td> set_core_params <td> Core clock params validity flag. If FALSE then the core_params field be ignored.
<tr><td> core_params <td> Core clock params structure with flexibility to set upper and lower core clock DCVS thresholds and also vote for core clock using a voltage corner. Valid when set_core_params is set to TRUE.
<tr><td> set_bus_params <td> Bus clock params validity flag. If FALSE then the bus_params field will be ignored.
<tr><td> bus_params <td> Bus clock params structure with flexibility to set upper and lower bus clock DCVS thresholds and also vote for bus clock using a voltage corner. Valid when set_bus_params is set to TRUE.
<tr><td> set_dcvs_v3_params <td> Validity flag for reserved DCVS params. If FALSE then the dcvs_v3_params field will be ignored.
<tr><td> dcvs_v3_params <td> Reserved DCVS params
<tr><td> set_sleep_disable <td> Sleep param validity flag. If FALSE then the sleep_disable field will be ignored.
<tr><td> sleep_disable <td> To select low-power mode (LPM). Valid when set_sleep_disable is set to TRUE. Refer to [Sleep Disable](#sleep_disable) for options.
</table>

####Wrapper APIs
There are wrapper functions built around same HAP_power_set()(type: HAP_power_set_DCVS_v3) to help user to select and vote for individual functionality in DCVS v3 without bothering about DCVS v3 structure and related details. Below section provides these APIs details.

* HAP_power_set_dcvs_v3_init()
* HAP_power_set_dcvs_option()
* HAP_power_set_sleep_latency()
* HAP_power_set_core_corner()
* HAP_power_set_bus_corner()
* HAP_power_set_sleep_mode()

####DCVS Enable
'dcvs_enable' parameter of dcvs_v2 structure enables user to vote for DCVS participation.
<table>
<tr><th> Value <th> Description
<tr><td> TRUE <td> Enable DSP DCVS (if not already enabled). Using dcvs_option, based on the application demand, user can choose a particular option to guide DSP DCVS logic
<tr><td> FALSE <td> Don't enable DSP DCVS. Valid only when the client requesting is the only one actively voting for clocks or is one among the clients voting for this same option.
</table>

'set_dcvs_enable' and 'dcvs_enable parameters' of dcvs_v3 structure enables user to vote for DCVS participation.
<table>
<tr><th rowspan="2"> set_dcvs_enable <td> FALSE <td> No DCVS request from the client, dcvs_enable and dcvs_option fields will be ignored.
<tr><td> TRUE <td> Client request for DCVS is valid and desired DCVS participation is provided in dcvs_enable field.
<tr><th rowspan="2"> dcvs_enable <td> TRUE <td> Enable DSP DCVS (if not already enabled). Using dcvs_option, based on the application demand, user can choose a particular option to guide DSP DCVS logic.
<tr><td> FALSE <td> Don't enable DSP DCVS. Valid only when the client requesting is the only one actively voting for clocks or is one among the clients voting for this same option.
</table>

When a DCVS participating client is active, DCVS logic would be enabled, but the aggregated clients vote requesting for DCVS disable will be considered as a FLOOR request in DCVS logic i.e, DCVS would't lower the clocks below the aggregated value.

DCVS participation and options are considered only for active clients. A client is deemed inactive when there is no MIPS and bandwidth request (made by setting request type to 'HAP_power_set_mips_bw' in 'HAP_power_set' [API](#usage)) and when target_corner for core and bus under dcvs_params is set to HAP_DCVS_VCORNER_DISABLE.

####DCVS Options
'dcvs_option' parameter of dcvs_v2 structure enables user to request for a particular DCVS mode when 'dcvs_enable' option is set to TRUE.

'dcvs_option' parameter of dcvs_v3 structure enables user to request for a particular DCVS mode when 'set_dcvs_enable' and 'dcvs_enable' both are set to TRUE.

Following table captures the gist of the available DCVS modes.

<table>
<tr><th> Value <th> Description
<tr><td> HAP_DCVS_V2_ADJUST_UP_DOWN
<td> Legacy option: For clients voting via HAP_power_set_mips_bw request type.
This mode allows DCVS to both increase and decrease core/bus clock speeds based on need. DCVS selects thresholds corresponding to a balanced mode (legacy) of operation with respect to power and performance.

min_corner and max_corner votes via dcvs_params are used as lower and

upper limit guidelines in DCVS.

NOTE: If client votes via target_corner under dcvs_params of this structure, both HAP_DCVS_V2_ADJUST_ONLY_UP and HAP_DCVS_V2_ADJUST_UP_DOWN modes are identical. min_corner and max_corner votes are used as lower and upper limit guidelines in DCVS while using balanced mode (legacy) thresholds.

<tr><td> HAP_DCVS_V2_ADJUST_ONLY_UP
<td> Legacy option: For clients voting via HAP_power_set_mips_bw request type.

This mode restricts DCVS from lowering the clock below the values requested via HAP_power_set_mips_bw request. DCVS can only increase the clock above the requested levels. DCVS selects thresholds corresponding to a balanced mode(legacy) of operation with respect to power and performance. max_corner vote via dcvs_params is used as upper limit guideline in DCVS.

NOTE: If client votes via target_corner under dcvs_params of this structure, both HAP_DCVS_V2_ADJUST_ONLY_UP and HAP_DCVS_V2_ADJUST_UP_DOWN modes are identical. min_corner and max_corner votes are used as lower and upper limit guidelines in DCVS while using balanced mode (legacy) thresholds.

<tr><td> HAP_DCVS_V2_POWER_SAVER_MODE
<td> New option:

Default for all clients participating in DCVS. DCVS can both increase and decrease the core/bus clock speeds while min_corner and max_corner votes are used as lower and upper limit guidelines. DCVS selects thresholds corresponding to power saving model. This mode is meant for applications where saving power is of higher priority than achieving fastest performance. Performance may be slower in this mode than in HAP_DCVS_V2_PERFORMANCE_MODE or the legacy modes i.e, HAP_DCVS_V2_ADJUST_ONLY_UP HAP_DCVS_V2_ADJUST_UP_DOWN

<tr><td> HAP_DCVS_V2_POWER_SAVER_AGGRESSIVE_MODE
<td> New option:

DCVS can both increase and decrease the core/bus clock speeds while min_corner and max_corner votes are used as lower and upper limit guidelines. DCVS selects thresholds corresponding to a power saving model. Further, the DCVS monitoring durations in lowering the clocks is decreased for a faster ramp down and hence greater power saving compared to the power saver mode. This mode is meant for applications where saving power is of higher priority than achieving fastest performance. Performance may be slower in this mode than in HAP_DCVS_V2_PERFORMANCE_MODE HAP_DCVS_V2_POWER_SAVER_MODE or the legacy modes i.e, HAP_DCVS_V2_ADJUST_ONLY_UP HAP_DCVS_V2_ADJUST_UP_DOWN

<tr><td> HAP_DCVS_V2_PERFORMANCE_MODE
<td>New option:

DCVS can both increase and decrease the core/bus clock speeds while min_corner and max_corner votes are used as lower and upper limit guidelines. DCVS selects a set of aggressive thresholds in terms of performance. DCVS can quickly bump up the clocks in this mode assisting higher performance at the cost of power.

<tr><td> HAP_DCVS_V2_DUTY_CYCLE_MODE
<td> This mode is for periodic use cases. Starting with Lahaina, DCVS when in this mode detects the periodicity and sets/removes the core and bus clock votes for active/idle durations respectively. This mode helps save power significantly by reducing idle leakage current while keeping the performance intact. Compared to Applications setting/removing clock votes for each active frame to save the power, the DCVS duty cycle mode provides better performance and more power savings, as in this mode, the voting is done upfront by DCVS just before active duration start based on periodicity prediction.
</table>
In cases where multiple clients have registered different DCVS options, following table depicts the DCVS policy aggregation logic.
<table>
<tr><th> PERFORMANCE (Yes / No) <th> POWER SAVER (Yes / No) <th> POWER SAVER AGGRESSIVE (Yes / No) <th> BALANCED (UP ONLY/UP AND DOWN clients) (Yes / No) <th> Final DCVS thresholds
<tr><td> Y <td> Y /N <td> Y /N <td> Y /N <td> PERFORMANCE
<tr><td> N <td> Y <td> Y /N <td> Y /N <td> POWER SAVER
<tr><td> N <td> N <td> Y <td> Y <td> POWER SAVER
</table>

####DCVS Duty Cycle
DCVS duty cycle mode is for periodic use cases. The DCVS algorithm detects periodicity and sets the core and bus clock votes as per active and idle duration. This helps in saving the power to great extent by reducing idle leakage current while keeping the performance intact.

Below example illustrates DCVS duty cycle working for an application with 30FPS activity and TURBO_PLUS votes for core and bus clocks.

For this application run, core, bus clocks and related DSP metrics with and without DCVS duty cycle mode are shown below. In no duty cycle case, core and bus clocks are at TURBO_PLUS throughout the application run.
In DCVS duty cycle case, the DCVS algorithm detects periodicity in use case and sets core and bus clocks to TURBO_PLUS in activity time and to LOW SVS (SVS2) during idle time of each frame.

![screenshot](../../images/DCVS_CoreClock_DutyCycle.png)

![screenshot](../../images/DCVS_BusClock_DutyCycle.png)

With increasing processing capabilities, active time for applications will improve resulting in greater power savings for periodic activities with DCVS duty cycle mode due to increased idle time.

The DCVS duty cycle mode is supported starting with Lahaina. On chipsets prior Lahaina, DCVS fallsback to power saver mode on selecting duty cycling.

####DCVS Duty Cycle Modes
Starting with Waipio, DCVS duty cycle mode is further expanded to cover following scenarios/sub-modes.

####Fixed Corners Mode
Fixed active and idle clock corners:
* Client decides fixed active clock and idle clock
* DCVS only uses those selected corners

Example:
* Max corner : HAP_DCVS_VCORNER_DISABLE
* Target corner : TURBO
* Min corner : LOW SVS (SVS2)
* Mode : Duty_cycle
* DCVS Enable flag: 0
* Expectation : Duty cycle between TUBRO and LOW SVS (SVS2) only

![screenshot](../../images/HAP_set_dcvs_v3_duty_cycle_fixed_corners_mode.png)

####Active Range Mode
Client and the DCVS algorithm decides active clock corners:
* Client decides active clock range and idle clock
* The DCVS algorithm decides active corner within provided range based on power vs performance tradeoff and user given max active time (if provided)

Example:
* Max corner : TURBO
* Target corner : SVS PLUS
* Min corner : LOW SVS (SVS2)
* Mode : Duty_cycle
* DCVS Enable flag: 1
* Expectation : Active clock is decided by the DCVS algorithm within the provided range (Target corner, max corner)
* The DCVS algorithm starts with client provided max corner for active clock, tunes it based on performance vs power tradeoff and user given max active time.

![screenshot](../../images/HAP_set_dcvs_v3_duty_cycle_active_range_mode.png)

####Full DCVS Control Mode
DCVS decides active and idle clock corners:
* Client does not provide active and idle clock corner
* The DCVS algorithm can decide any clock corner for active and idle durations based on power vs performance tradeoff and user given max active time (if provided)
* Max corner : HAP_DCVS_VCORNER_DISABLE
* Target corner : HAP_DCVS_VCORNER_DISABLE
* Min corner : HAP_DCVS_VCORNER_DISABLE
* Mode : Duty_cycle
* DCVS Enable flag: 1
* Expectation : Active and idle clocks are decided by the DCVS algorithm
* DCVS starts with NOM as active corner and LOW SVS (SVS2) as idle corner and later tunes it based on performance vs power tradeoff and user given max active time (if provided).
* DCVS picks LOW SVS (SVS2) clock corner when there is no activity.

![screenshot](../../images/HAP_set_dcvs_v3_duty_cycle_full_dcvs_control_mode.png)

####DCVS Duty Cycle Helper APIs
Starting Waipio, the DCVS duty cycle helper APIs are added for ease of configuration. See [HAP_dcvs.h](../../doxygen/HAP_dcvs/index.html) for more information. 

####Sleep latency {#sleep-latency}
'set_latency' and 'latency' parameters of structure dcvs_v2 can be used to request for a sleep latency in micro seconds.

<table>
<tr><th rowspan="2"> set_latency <td> FALSE <td> No sleep latency request from the client. If FALSE then default sleep latency vote of 65535 micro seconds will be considered.
<tr><td> TRUE <td> Client request for a sleep latency is valid and desired latency is provided in latency field.
<tr><th> latency <td colspan="2"> Sleep latency request in micro-seconds.
</table>

Similarly 'set_latency' and 'latency' parameters of structure dcvs_v3 can be used to request for a sleep latency in micro seconds.

<table>
<tr><th rowspan="2"> set_latency <td> FALSE <td> No sleep latency request from the client. If FALSE then the latency field will be ignored.
<tr><td> TRUE <td> Client request for a sleep latency is valid and desired latency is provided in latency field.
<tr><th> latency <td colspan="2"> Sleep latency request in micro-seconds.
</table>

NOTE: HAP_power_set provides below possible ways for voting for sleep latency:

1. via HAP_power_set_mips_bw request type:
~~~{.c}
/* For sleep latency */
mips_bw.set_latency = TRUE;
mips_bw.latency = <Sleep latency tolerance in micro seconds>
~~~
2. via HAP_power_set_DCVS_v2 request type:
~~~{.c}
/* For sleep latency */
dcvs_v2.set_latency = TRUE;
dcvs_v2.latency = <Sleep latency tolerance in micro seconds>
~~~
   Or via HAP_power_set_DCVS_v3 request type:
~~~{.c}
/* For sleep latency */
dcvs_v3.set_latency = TRUE;
dcvs_v3.latency = <Sleep latency tolerance in micro second>
~~~

Clients should use only 1 of the above methods to vote for latency i.e, either via mips_bw or via dcvs_v2/dcvs_v3 but not both. Voting via dcvs_v2/dcvs_v3 does NOT cancel any previous vote done via mips_bw and vice versa.

latency value can be set to a minimum of 10 micro-second. The Application should vote for a latency that is tolerable. For latency critical applications, the latency can be set to its minimum value of 10 micro-second.


####DCVS params
set_dcvs_params and dcvs_params parameters of dcvs_v2 can be used to update DCVS thresholds and target corner vote.
set_core_params and core_params parameters of dcvs_v3 can be used to update DCVS thresholds and target corner vote for core clock. Similarly set_bus_params and bus_params parameters for bus clock.
This structure is valid irrespective of chosen dcvs_enable and dcvs_option values. Client can request for a target_corner even when the dcvs_enable option is set to FALSE.

When set_dcvs_params/set_core_params/set_bus_params is TRUE, target_corner, min_corner and max_corner parameters of dcvs_params/core_params/bus_params can take one of the value in ::HAP_dcvs_voltage_corner_t;

<table>
<tr><th> HAP_dcvs_voltage_corner_t <th> Description
<tr><td> HAP_DCVS_VCORNER_DISABLE <td> No specific corner request (No Vote)
<tr><td> HAP_DCVS_VCORNER_SVS2 <td>SVS2 / LOW SVS corner
Note: On targets that don't support this voltage corner, this option will be interpreted as HAP_DCVS_VCORNER_SVS
<tr><td> HAP_DCVS_VCORNER_SVS <td> SVS corner
<tr><td> HAP_DCVS_VCORNER_SVS_PLUS <td> SVS Plus corner
Note: On targets that don't support this voltage corner, this option will be interpreted as HAP_DCVS_VCORNER_SVS
<tr><td> HAP_DCVS_VCORNER_NOM <td> NOMINAL corner
<tr><td> HAP_DCVS_VCORNER_NOM_PLUS <td> NOMINAL Plus corner
Note: On targets that don't support this voltage corner, this option will be interpreted as HAP_DCVS_VCORNER_NOM
<tr><td> HAP_DCVS_VCORNER_TURBO <td>TURBO corner
<tr><td> HAP_DCVS_VCORNER_TURBO_PLUS <td> TURBO Plus corner
Note: On targets released till Kailua, this option selects the clock frequencies defined under corners TURBO_PLUS and above (TURBO_L2 / L3) and falls back to TURBO when there is no clock frequency available at these corners. On targets post Kailua, this option selects clock frequencies defined under TURBO_PLUS (or TURBO when no defined frequency under TURBO_PLUS). Frequencies defined under TURBO_L2 / L3 corners can be selected via the new HAP_DCVS_VCORNER_TURBO_L2 / L3 options.
<tr><td> HAP_DCVS_VCORNER_TURBO_L2 <td> TURBO L2 corner
Note: On targets released till Kailua, this option is interpreted as HAP_DCVS_VCORNER_TURBO_PLUS. On targets post Kailua, this option selects the closest TURBO clock frequency (corresponding to HAP_DCVS_VCORNER_TURBO_PLUS / TURBO) when there is no clock frequency defined under the TURBO_L2 voltage corner.
<tr><td> HAP_DCVS_VCORNER_TURBO_L3 <td> TURBO L3 corner
Note: On targets released till Kailua, this option is interpreted as HAP_DCVS_VCORNER_TURBO_PLUS. On targets post Kailua, this option selects the closest TURBO clock frequency (corresponding to HAP_DCVS_VCORNER_TURBO_L2 / TURBO_PLUS / TURBO) when there is no clock frequency defined under the TURBO_L3 voltage corner.
<tr><td> HAP_DCVS_VCORNER_MAX <td> MAX possible corner defined for maximum performance.
</table>
<br>
<table>
<tr><th rowspan="6"> dcvs_params/core_params/bus_params <td> target_corner <td>Type: HAP_dcvs_voltage_corner_t.
Alternative to HAP_power_set_mips_bw MIPS and Bandwidth request. HAP_power_set provides 2 possible ways for voting for sleep latency and core/bus clocks.
1. via HAP_power_set_mips_bw request type:
~~~{.c}
/* For core clock */
mips_bw.set_mips = TRUE;
mips_bw.mipsPerThread = <MIPS per thread request>
mips_bw.mipsTotal = <Total MIPS request>
/* For bus clock */
mips_bw.set_bus_bw = TRUE;
mips_bw.bwBytePerSec = <bandwidth request in bytes per second (Instantaneous)>
mips_bw.busbwUsagePercentage = <Usage percentage (Average)>
/* For sleep latency */
mips_bw.set_latency = TRUE;
mips_bw.latency = <Sleep latency in micro seconds>
~~~
2. via HAP_power_set_DCVS_v2 request type:
~~~{.c}
/* For core and bus clock */
dcvs_v2.set_dcvs_params = TRUE;
dcvs_v2.dcvs_params.target_corner = <Desired vote in terms of voltage corner for core, bus clocks>
/* For sleep latency */
dcvs_v2.set_latency = TRUE;
dcvs_v2.latency = <Sleep latency in micro seconds>
~~~
or

3. via HAP_power_set_DCVS_v3 request type:
~~~{.c}
/* For core clock */
dcvs_v3.set_core_params = TRUE;
dcvs_v3.core_params.target_corner = <Desired vote in terms of
voltage corner for core clock>
/* For bus clock */
dcvs_v3.set_bus_params = TRUE;
dcvs_v3.bus_params.target_corner = <Desired vote in terms of
voltage corner for bus clock>
/* For sleep latency */
dcvs_v3.set_latency = TRUE;
dcvs_v3.latency = <Sleep latency tolerance in micro seconds>
~~~
Client can request core and bus clock to run at at a particular voltage corner instead of providing MIPS and Bandwidth (bytes per second) requests. DCVS will convert the requested voltage corner value to appropriate core clock and bus clock votes and forwards the request to the power manager on client's behalf. Clients should use only 1 of the above methods to vote i.e, either via mips_bw or via dcvs_v2/dcvs_v3 but not both. Voting via dcvs_v2/dcvs_v3 does NOT cancel any previous vote done via mips_bw and vice versa. If one would like to switch between these 2 methods, cancel any previous vote done via the other method before requesting.

When target_corner = HAP_DCVS_VCORNER_DISABLE (No vote), DSP DCVS doesn't request for any core or bus clocks at the time of API call and it's client's responsibility to vote for core and bus clocks using HAP_power_set_mips_bw type request type.

If enabled > HAP_DCVS_VCORNER_DISABLE, DSP DCVS logic will pick the highest available frequency plan for both core and bus clocks at the given voltage corner and requests for these clock frequencies synchronously in the API context on client's behalf. When the HAP_power_set API returns with success, core and bus clock frequencies would be set by DSP DCVS on a valid target_corner request.

<tr><td> min_corner <td> Type: HAP_dcvs_voltage_corner_t.

If disabled, min_corner == HAP_DCVS_VCORNER_DISABLE, the lower threshold/minimum value that DCVS can correct the clock will remain unchanged. If enabled > HAP_DCVS_VCORNER_DISABLE, DSP DCVS picks the lowest core clock frequency at the given voltage corner and uses it as the lower threshold/minimum value that DCVS can correct the clock to, irrespective of the dcvs_option selected.

min_corner should always be less than or equal to target_corner and max_corner unless they are disabled HAP_DCVS_VCORNER_DISABLE.

For clients requesting dcvs_enable as FALSE and using target_corner, min_corner should be equal to target_corner.

<tr><td> max_corner <td>Type: HAP_dcvs_voltage_corner_t.

If disabled, max_corner == HAP_DCVS_VCORNER_DISABLE, the upper threshold/maximum value that DCVS can correct the clock will remain unchanged. Typically, that would be HAP_DCVS_VCORNER_MAX in this case. If enabled > HAP_DCVS_VCORNER_DISABLE, DSP DCVS picks the highest core and bus clock frequencies at the given voltage corner and uses it as the upper threshold/maximum value that DCVS can correct the clocks to, irrespective of the dcvs_option selected.

DSP DCVS logic overrides the max_corner vote from a client to MAX in presence of a concurrency. Concurrency is defined as a scenario where 2 or more FastRPC dynamic loaded clients are active or active Audio/Voice sessions with MPPS load greater than a pre-defined threshold.

max_corner should always be greater than or equal to target_corner and min_corner votes, or, should be disabled HAP_DCVS_VCORNER_DISABLE.

<tr><td> param1 <td> Type: HAP_dcvs_voltage_corner_t.

NOTE: Set this option to HAP_DCVS_VCORNER_DISABLE unless required.

This parameter allows user to set CPU L3 clock frequency to the requested corner. Valid only on CDSP subsystem in targets with CPU L3 cache and IO-coherency enabled (SDM845, SDM710, SM8150...), ignored elsewhere. On CDSP, based on the requested target_corner, CPU L3 clock vote from CDSP is set to a balanced level (with minimal power impact) to start with and DCVS (if enabled) increases the vote based on need to attain higher performance. This option is useful to peg CPU L3 clock at a higher level (at the cost of higher power) than that of the default balanced vote and that of the DCVS algorithm votes. This option is for advanced users and should be configured to default (HAP_DCVS_VCORNER_DISABLE) unless there is a need to explicitly set CPU L3 clock frequency based on performance and power analysis/characterization

<tr><td> param2 <td> Reserved.
<tr><td> param3 <td> Reserved.
</table>

####Clock frequency level selection at given target corner
By default DCVS picks the highest available frequency for a given core/bus clock target corner. On latest chipsets(released after Palima), APIs are added to allow the user to specify frequency level (highest/lowest) for given core/bus clock target corner. See [HAP_dcvs.h](../../doxygen/HAP_dcvs/index.html) for more information.

####DCVS vote aggregation logic in case of concurrency
Following logic explains the aggregation logic for min and target corner votes when there are multiple requesting clients:
~~~{.c}
DCVS min_corner vote = MAX (min_corner vote client 1, client 2, ...)
DCVS target_corner vote = MAX (target_corner vote client 1, client 2, ...)
~~~
The following scenarios are treated as a concurrency in DCVS vote aggregation logic where DCVS max corner vote is set to TURBO by DCVS:
* More than 1 active HAP client with or without active Audio/Voice clients.
* One active HAP client and active Audio/Voice clients with MPPS load greater than a pre-defined threshold.
~~~{.c}
	DCVS max_corner vote = HAP_DCVS_VCORNER_MAX
~~~

Note that DCVS overrides client's MAX corner vote to MAX to accommodate any concurrency requirement. DCVS MAX vote of MAX doesn't necessarily mean that DCVS will push the vote to MAX corner; MAX corner vote just sets the upper threshold for DCVS vote logic. DCVS will only bump up the clocks on need basis based on selected DCVS option.

####Sleep Disable {#sleep_disable}
'set_sleep_disable' and 'sleep_disable' parameters of dcvs_v3 structure enables user to select low-power mode (LPM) in DSP.

In general, applications are expected to vote for their latency tolerance via the [latency](#sleep-latency) parameter in dcvs_v3/dcvs_v2 options. The aggregated latency vote across clients is used in selecting appropriate low-power mode (LPM) of the DSP subsystem. LPM will save power when the DSP subsystem is idle by reducing leakage current. Deeper LPMs typically have higher wake up latencies, which will increase interrupt service delays and add to inter-processor communication latencies. Though the latency vote controls the selection of low-power modes, the vote required for disabling/allowing certain LPMs is difficult to calculate as the wakeup latency associated with these LPMs could change from chipset to chipset and between runs within the same chipset.

This 'sleep_disable' parameter in dcvs_v3 allows user to directly prevent certain LPM levels of the DSP subsystem. By default, there is no restriction placed on LPMs i.e. all the LPMs are enabled and the aggregated latency vote (along with other system parameters) is used in LPM selection. The 'sleep_disable' parameter in dcvs_v3 is for the advanced developers who would like to disable certain low-power modes explicitly irrespective of the latency vote. Developers need to consider their power-performance tradeoff requirements and if necessary profile the results before voting using this parameter. Regular users are suggested to choose the default i.e. 'HAP_DCVS_LPM_ENABLE_ALL'.

If any particular LPM level is not supported on the DSP subsystem then it will enable nearest shallow LPM level. For example, in absense of 'HAP_DCVS_LPM_LEVEL3' it will select
'HAP_DCVS_LPM_LEVEL2' which is nearest shallow LPM level to 'HAP_DCVS_LPM_LEVEL3'.

<table>
<tr><th rowspan="2"> set_sleep_disable <td> FALSE <td> No low-power mode request from the client. If FALSE then the sleep_disable field will be ignored.
<tr><td> TRUE <td> Client request for low-power mode is valid and desired option is provided in sleep_disable field.
<tr><th rowspan="4"> sleep_disable <td> HAP_DCVS_LPM_LEVEL1 <td> To disable sleep/low-power modes.
<tr><td> HAP_DCVS_LPM_LEVEL2 <td> To enable only standalone APCR.
<tr><td> HAP_DCVS_LPM_LEVEL3 <td> To enable RPM assisted APCR.
<tr><td> HAP_DCVS_LPM_ENABLE_ALL <td> To enable all low-power modes (enables full power collapse).
</table>

***NOTE:*** Till Palima, only HAP_DCVS_LPM_LEVEL1 and HAP_DCVS_LPM_ENABLE_ALL are supported.

####Illustrations (DCVS_V2)
NOTE:
For working example, refer `$HEXAGON_SDK_ROOT\examples\common\benchmark_v65` application; See benchmark_setClocks() in src_dsp\benchmark_imp.c

1. Requirement: Enable DCVS in PERFORMANCE mode, set sleep latency to 1000 micro-seconds, vote NOM in Target with SVS as Min and TURBO as Max.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v2;
request.dcvs_v2.dcvs_enable = TRUE;
request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
request.dcvs_v2.set_latency = TRUE;
request.dcvs_v2.latency = 1000;
request.dcvs_v2.set_dcvs_params = TRUE;
request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_TURBO;
request.dcvs_v2.dcvs_params.target_corner = HAP_DCVS_VCORNER_NOM;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block
 */
...
//To remove the vote
memset(&request, 0, sizeof(HAP_power_request_t)); //Remove all votes.
request.type = HAP_power_set_DCVS_v2;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

2. Requirement: Disable DCVS; do NOT vote for any corners/latency
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v2;
request.dcvs_v2.dcvs_enable = FALSE;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

3. Requirement: Enable DCVS in Power saver mode. Do NOT vote for any target corner/latency, but set MIN and MAX thresholds to DCVS to SVS and TURBO respectively. Clock voting will be done via HAP_power_set_mips_bw request.
~~~{.c}
//Vote

/* Populate request structure with dcvs_v2 request*/
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v2;
request.dcvs_v2.dcvs_enable = TRUE;
request.dcvs_v2.dcvs_option = HAP_DCVS_V2_POWER_SAVER_MODE;
request.dcvs_v2.set_dcvs_params = TRUE;
request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_TURBO;
request.dcvs_v2.dcvs_params.target_corner = HAP_DCVS_VCORNER_DISABLE; //no vote
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
/* Populate request structure with mips_bw request */
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_mips_bw;
request.mips_bw.set_mips = TRUE;
request.mips_bw.mipsPerThread = 150;
request.mips_bw.mipsTotal = 600;
request.mips_bw.set_bus_bw = TRUE;
request.mips_bw.bwBytesPerSec = 10*1000*1000;
request.mips_bw.busbwUsagePercentage = 50;
request.mips_bw.set_latency = TRUE;
request.mips_bw.latency = 1000;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request); // Core and bus clocks will be set by this request.
...
/*
 * Processing block
 */
...
//To remove the dcvs_v2 vote
memset(&request, 0, sizeof(HAP_power_request_t)); //Remove all votes.
request.type = HAP_power_set_DCVS_v2;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
//To remove the mips_bw vote
memset(&request, 0, sizeof(HAP_power_request_t)); //Remove all votes
request.type = HAP_power_set_mips_bw;
request.mips_bw.set_mips = TRUE;
request.mips_bw.set_bus_bw = TRUE;
request.mips_bw.set_latency = TRUE;
request.mips_bw.latency = 65535;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

4. Requirement: Enable DCVS in DUTY CYCLE mode, vote TURBO in Target with SVS as Min.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v2;
request.dcvs_v2.dcvs_enable = TRUE;
request.dcvs_v2.dcvs_option = HAP_DCVS_V2_DUTY_CYCLE_MODE;
request.dcvs_v2.set_latency = TRUE;
request.dcvs_v2.latency = 1000;
request.dcvs_v2.set_dcvs_params = TRUE;
request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_TURBO;
request.dcvs_v2.dcvs_params.target_corner = HAP_DCVS_VCORNER_TURBO;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block
 */
...
//To remove the vote
memset(&request, 0, sizeof(HAP_power_request_t)); //Remove all votes.
request.type = HAP_power_set_DCVS_v2;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

####Illustrations (DCVS_V3)


1. Requirement: Enable DCVS in POWER SAVER mode, set sleep latency to 1000 micro-seconds, vote NOM in Target with SVS as Min and TURBO as Max for core clock, vote TURBO in Target with NOM as Min and TURBO PLUS as Max for bus clock. Later change bus clock vote as SVS_PLUS in Target with SVS as Min and NOM as Max.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.dcvs_enable = TRUE;
request.dcvs_v3.dcvs_option = HAP_DCVS_V2_POWER_SAVER_MODE;
request.dcvs_v3.set_latency = TRUE;
request.dcvs_v3.latency = 1000;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO;
request.dcvs_v3.core_params.target_corner = HAP_DCVS_VCORNER_NOM;
request.dcvs_v3.set_bus_params = TRUE;
request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_NOM;
request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
request.dcvs_v3.bus_params.target_corner = HAP_DCVS_VCORNER_TURBO;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block 1
 */
...
//To update bus clock votes while keeping core clock and other parameters of dcvs_v3 request intact.
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_bus_params = TRUE;
request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_NOM;
request.dcvs_v3.bus_params.target_corner = HAP_DCVS_VCORNER_SVS_PLUS;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block 2
 */
...
//To remove the vote
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.set_latency = TRUE;
request.dcvs_v3.latency = 65535;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.set_bus_params = TRUE;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

2. Requirement: Enable DCVS in PERFORMANCE mode, vote TURBO in Target with NOM as Min and TURBO PLUS as Max for core clock, do NOT vote for latency and bus clock.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.dcvs_enable = TRUE;
request.dcvs_v3.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_NOM;
request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
request.dcvs_v3.core_params.target_corner = HAP_DCVS_VCORNER_TURBO;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block
 */
...
//To remove the vote
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.set_core_params = TRUE;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

3. Requirement: Disable DCVS; do NOT vote for any corners/latency.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.dcvs_enable = FALSE;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_seti(ctx, &request);
~~~

4. Requirement: Disable sleep (all low power modes) and re-enable it after task completion.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_sleep_disable = TRUE;
request.dcvs_v3.sleep_disable = HAP_DCVS_LPM_LEVEL1;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block
 */
...
//To re-enable sleep.
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_sleep_disable = TRUE;
request.dcvs_v3.sleep_disable = HAP_DCVS_LPM_ENABLE_ALL;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

5. Requirement: Enable DCVS in PERFORMANCE mode. Do NOT vote for any target corner/latency, but set MIN and MAX DCVS thresholds for core clock to NOM and TURBO respectively, set MIN and MAX DCVS thresholds for bus clock to SVS and NOM respectively. Clock voting will be done via HAP_power_set_mips_bw request.
~~~{.c}
//Vote

/* Populate request structure with dcvs_v3 request*/
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.dcvs_enable = TRUE;
request.dcvs_v3.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_NOM;
request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO;
request.dcvs_v3.set_bus_params = TRUE;
request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_NOM;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
/* Populate request structure with mips_bw request */
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_mips_bw;
request.mips_bw.set_mips = TRUE;
request.mips_bw.mipsPerThread = 150;
request.mips_bw.mipsTotal = 600;
request.mips_bw.set_bus_bw = TRUE;
request.mips_bw.bwBytesPerSec = 10*1000*1000;
request.mips_bw.busbwUsagePercentage = 50;
request.mips_bw.set_latency = TRUE;
request.mips_bw.latency = 1000;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request); // Core and bus clocks will be set by this request.
...
/*
 * Processing block
 */
...
//To remove the dcvs_v3 vote
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.set_bus_params = TRUE;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
//To remove the mips_bw vote
memset(&request, 0, sizeof(HAP_power_request_t)); //Remove all votes
request.type = HAP_power_set_mips_bw;
request.mips_bw.set_mips = TRUE;
request.mips_bw.set_bus_bw = TRUE;
request.mips_bw.set_latency = TRUE;
request.mips_bw.latency = 65535;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

6. Requirement: Use wrapper APIs to: Enable DCVS in POWER SAVER AGGRESSIVE mode, set sleep latency to 1000 micro-seconds, vote NOM in Target with SVS as Min and TURBO as Max for core clock, vote TURBO in Target with NOM as Min and TURBO PLUS as Max for bus clock.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
HAP_power_set_dcvs_v3_init(&request);
retVal = HAP_power_set_dcvs_option(NULL, TRUE, HAP_DCVS_V2_POWER_SAVER_AGGRESSIVE_MODE);
retVal = HAP_power_set_sleep_latency(NULL, 1000);
retVal = HAP_power_set_core_corner(NULL, HAP_DCVS_VCORNER_NOM, HAP_DCVS_VCORNER_SVS, HAP_DCVS_VCORNER_TURBO);
retVal = HAP_power_set_bus_corner(NULL, HAP_DCVS_VCORNER_TURBO, HAP_DCVS_VCORNER_NOM, HAP_DCVS_VCORNER_TURBO_PLUS);
...
/*
 * Processing block
 */
...
//To remove the vote
HAP_power_set_dcvs_v3_init(&request);
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~

7. Requirement: Enable DCVS in DUTY CYCLE mode, vote TURBO_PLUS in Target with SVS as Min for core and bus clock.
~~~{.c}
//Vote

/* Populate request structure */
int retVal;
HAP_power_request_t request;
memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.dcvs_enable = TRUE;
request.dcvs_v3.dcvs_option = HAP_DCVS_V2_DUTY_CYCLE_MODE;
request.dcvs_v3.set_latency = TRUE;
request.dcvs_v3.latency = 1000;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.core_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v3.core_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
request.dcvs_v3.core_params.target_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
request.dcvs_v3.set_bus_params = TRUE;
request.dcvs_v3.bus_params.min_corner = HAP_DCVS_VCORNER_SVS;
request.dcvs_v3.bus_params.max_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
request.dcvs_v3.bus_params.target_corner = HAP_DCVS_VCORNER_TURBO_PLUS;
/* Call HAP_power_set API with the updated request structure */
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
...
/*
 * Processing block
 */
...
//To remove the vote
memset(&request, 0, sizeof(HAP_power_request_t));
request.type = HAP_power_set_DCVS_v3;
request.dcvs_v3.set_dcvs_enable = TRUE;
request.dcvs_v3.set_latency = TRUE;
request.dcvs_v3.latency = 65535;
request.dcvs_v3.set_core_params = TRUE;
request.dcvs_v3.set_bus_params = TRUE;
/* ctx is an unique identifier, explained [here](#usage). */
retVal = HAP_power_set(ctx, &request);
~~~
