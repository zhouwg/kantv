### Overview

the steps here has verified on Ubutun 20.04, Ubutun 24.04, Ubuntu 26.04:

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
    Ubuntu 20.04(EOL on 31 May 2025), 24.04, 26.04 is recommended.

- tools & utilities

    run below script accordingly
    ```
    ./build/prebuild.sh
    ```

 - download Android-NDK, Android-SDK, Hexagon SDK, and necessary LLVM toolchain for **command-line mode build** (set `SKIP_HEXAGON_SDK=1` to skip Hexagon SDK download for non-Qualcomm builds)

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

#### How to enable/disable ggml-hexagon backend

the `core/llamacpp/` directory is synced from the [ggml-hexagon](https://github.com/ggml-hexagon/ggml-hexagon) project, which provides ggml-hexagon backend for Qualcomm Snapdragon NPU.

- Mempool/FastRPC-invoke implementation (default, `GGML_HEXAGON_USE_MEMPOOL=ON`): builds `ggml-hexagon-fastrpc.cpp` + `htp/` DSP skel via ExternalProject
- Dspqueue/per-buffer implementation (`GGML_HEXAGON_USE_MEMPOOL=OFF`): falls back to `ggml-hexagon.cpp` + `htp/` DSP skel via ExternalProject

to switch implementation, modify <a href="https://github.com/zhouwg/kantv/blob/master/core/CMakeLists.txt#L33">core/CMakeLists.txt#L33</a> (`GGML_HEXAGON_USE_MEMPOOL` option).

#### Runtime configuration

the `ggml-hexagon.cfg` file controls FastRPC-based ggml-hexagon runtime behavior, refer to `ggml-hexagon.cfg` for full documentation of each option.

#### Supported HTP arch versions

| HTP arch | SoC | support |
|----------|-----|------------|
| v73 | Snapdragon 8 Gen2 | yes |
| v75 | Snapdragon 8 Gen3 | yes |
| v79 | Snapdragon 8 Elite(aka 8 Gen4) | yes |
| v81 | Snapdragon 8 Elite Gen5 | yes |
