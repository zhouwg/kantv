# ETM Trace enable/disable APIs

The HAP ETM framework exposes a set of APIs to enable/disable
ETM tracing in a user module to trace a region of interest
provided ETM tracing is configured.

For configuring ETM tracing, refer to "profile on device" section
of Profiling example in base SDK.

After ETM tracing is configured, the API requires setting the
'--hap_etm_enable' flag via sysMonApp etmTrace option as below:
```
adb shell /data/local/tmp/sysMonApp etmTrace --command etm --hap_etm_enable 1
```

After ETM trace collection, this flag should be reset with the
command:
```
adb shell /data/local/tmp/sysMonApp etmTrace --command etm --hap_etm_enable 0
```

Call to the APIs are ignored in the following cases:
* ETM tracing is not configured.
* The '--hap_etm_enable' flag is set to 0.

***NOTE:*** The APIs work only on debug enabled device.
A test device or debug device, (Mobile Test Platform) MTP
or (Qualcomm Reference Design) QRD, is a device on which
the debug fuse is present. This fuse is not present on
production devices.

## Supported chipsets

Beyond Palima

## Framework APIs

Header file: @b HAP_etm_config.h