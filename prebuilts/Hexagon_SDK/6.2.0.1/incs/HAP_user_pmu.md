# Performance monitoring unit

The DSP subsystem has the PMU (Performance Monitoring Unit) with counters to track
hardware events (called PMU events). The HAP PMU framework exposes a
set of APIs to read these PMU counters configured with specified PMU events. PMU
events are Hexagon DSP architecture specific and the most common PMU events are briefly
described in the Hexagon DSP architecture documentation.
The [itrace](../../doxygen/itrace/index.html) library's header file `itrace_dsp_events_pmu.h`
provides a complete list of all available public PMU events alongside their descriptions.

***NOTE:*** 
* aDSP and cDSP DCVS relies on a set of PMU events to monitor DSP
statistics and make necessary decisions. Using these HAP APIs to register PMU
events results in DCVS no longer being able to track these events. This might
lead DCVS to making incorrect decisions.
* HAP PMU APIs only work on [debug-enabled](../../tools/sign.html#test-device) devices.

The HAP PMU APIs are not accessible from unsigned PD.

## Supported chipsets

SM8250 and beyond

## Framework APIs

Header file: @b HAP_user_pmu.h
