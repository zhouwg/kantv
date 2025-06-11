# HAP_farf

##  Overview

The FARF API on DSP is used to generate diagnostic messages. These messages are sent to a diagnostic (or DIAG) framework on the DSP, from which they can be collected via USB using a tool called mini-dm running on the host computer. Parallelly, the DSP FARF messages can be routed to the application processor, allowing the user to collect DSP messages with logcat. These tools and the process for collecting messages is explained in the Messaging resources page from the SDK documentation.

FARF messages can be enabled at compile-time and runtime. The Messaging resources page from the SDK documentation explains in detail the differences between compile-time and runtime FARF messages, how to enable them, and how to display them.
