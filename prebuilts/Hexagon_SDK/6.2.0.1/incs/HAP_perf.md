# Introduction {#intro}

The Hexagon SDK provides APIs to measure the elapsed time in both microseconds
and processor cycles(pcycles).


# API Overview {#api-overview}

The HAP_perf APIs are used by clients for profiling their code when running on the DSP. The profiling
can be done in both microseconds and pcycles based on the needs. Morevover, the HAP_perf library
also provides sleep APIs to the clients.

The HAP_perf APIs include the following functions:

::HAP_perf_get_time_us

::HAP_perf_get_qtimer_count

::HAP_perf_qtimer_count_to_us

::HAP_perf_get_pcycles

::HAP_timer_sleep
