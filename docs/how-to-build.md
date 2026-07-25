### Overview

the steps here has verified on Ubutun 20.04, Ubutun 24.04:

follow the steps below to generate the specified Android APK in a <b>fresh and clean</b> Ubuntu 24.04. the generated Android APK can be installed and run properly on Android phones equipped with Qualcomm Snapdragon 8Gen3 and 8Elite.

### Fetch source codes
```
git clone https://github.com/zhouwg/kantv.git

cd kantv

git checkout master
```

### Setup development environment

#### Prerequisites

- OS
    Ubuntu 20.04(EOL on 31 May 2025), 22.04, 24.04 is recommended.

- tools & utilities

    run below script accordingly
    ```
    ./build/prebuild.sh
    ```

 - download Android-NDK and Android-SDK and necessary LLVM toolchain for **command-line mode build**

   run below script accordingly
    ```
    . build/envsetup.sh

    ./build/prebuild-download.sh

    ```
 - download and install Android Studio manually (can be skipped for AI experts/researchers)

   download Android Studio Jellyfish (| 2023.3.1 April 30, 2024) from https://developer.android.com/studio/archive
![Screenshot from 2025-05-07 22-06-08](https://github.com/user-attachments/assets/bb801dfe-57a7-4832-a40d-bd1e39c9904e)


### Build

#### Build with Android Studio IDE(can be skipped for AI experts/researchers)

build the entire project by Android Studio IDE


#### Build with command line mode

```
  . build/envsetup.sh
  lunch 1
  # for Qualcomm devices with ggml-hexagon
  ./build/build-all.sh android_qcom
  # for non Qualcomm devices without ggml-hexagon
  ./build/build-all.sh android_non_qcom
```

#### How to enable/disable JZ's ggml-hexagon backend

the `core/ggml/llamacpp/` directory is synced from the [ggml-hexagon](https://github.com/zhouwg/ggml-hexagon) project (branch `self-build-jz`), which provides a complete JZ alternative AP-side implementation of the ggml-hexagon backend for Qualcomm Snapdragon cDSP (HTP/HMX).

- JZ implementation (default, `GGML_HEXAGON_JZ=ON`): builds `ggml-hexagon-jz.cpp` + `htp/` DSP skel via Makefile, outputs `libggmldsp-skel.so`
- Qualcomm implementation (`GGML_HEXAGON_JZ=OFF`): falls back to the official `ggml-hexagon.cpp` + CMake-based HTP skel build

to switch implementation, modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L31">ggml/CMakeLists.txt#L31</a> (`GGML_HEXAGON_JZ` option).

#### Runtime configuration

the `ggml-hexagon.cfg` file controls JZ's ggml-hexagon runtime behavior (cDSP thread count, cache mode, op fusion, flash attention kernel selection, etc.). key settings:

- `ndev`: number of Hexagon devices (PDs) to use
- `thread_counts`: cDSP-side thread count (2-8)
- `enable_graph_optimize`: cgraph reorder pass for MUL_MAT ops
- `enable_opfusion`: QKV/FFN op fusion (algotype=29)
- `fa_select`: flash attention kernel selection (0=CPU, 1=HVX, 2=HMX)
- `dsp_cache_mode`: DSP-side cache optimization bitmask (default 5)
- `enabled_types`: weight types to offload for MUL_MAT

refer to `ggml-hexagon.cfg` for full documentation of each option.

#### Supported HTP arch versions

| HTP arch | SoC | support |
|----------|-----|------------|
| v73 | Snapdragon 8 Gen2 | yes |
| v75 | Snapdragon 8 Gen3 | yes |
| v79 | Snapdragon 8 Elite(aka 8 Gen4) | yes |
| v81 | Snapdragon 8 Elite Gen5 | yes |
