# Introduction {#intro}

These APIs allow a user to perform the following actions:
* Manage the dynamic list of processes running on the current DSP
* Send a wakeup call to the CPU in order to decrease its response time upon returning from a FastRPC call
* Enquire about the thread priority ceiling for the current process


## API Overview {#api-overview}

The HAP_ps.h APIs include the following functions:

* ::HAP_get_process_list

* ::HAP_add_to_process_list

* ::HAP_remove_from_process_list

* ::HAP_set_process_name

* ::HAP_thread_migrate

* ::HAP_send_early_signal

* ::fastrpc_send_early_signal

* ::HAP_get_thread_priority_ceiling

* ::HAP_get_userpd_params

* ::HAP_get_pd_type

Header file: @b HAP_ps.h
