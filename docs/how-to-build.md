### Overview

the steps here has verified on Ubutun 20.04, Ubutun 24.04:

follow the steps below to generate the specified Android APK in a <b>fresh and clean</b> Ubuntu 24.04. the generated Android APK can be installed and run properly on Android phones equipped with Qualcomm Snapdragon 8Gen3 and 8Elite.

### Fetch source codes
```
git clone https://github.com/kantv-ai/kantv.git

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
  ./build-all.sh android
```

#### How to enable/disable debug build

- modify <a href="https://github.com/zhouwg/kantv/blob/master/android/kantvplayer/build.gradle#L17">kantvplayer/build.gradle#L17</a> accordingly

#### How to build project for Android phone equipped <b>without</b> Qualcomm mobile SoC

- modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L12">ggml/CMakeLists.txt#L12</a> accordingly if target Android phone is <b>NOT</b> equipped with Qualcomm mobile SoC

#### How to enable/disable StableDiffusion

- modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L14">ggml/CMakeLists.txt#L14</a> accordingly

#### How to build project for Android phone equipped with Qualcomm high-end mobile SoC

- modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L79">ggml/CMakeLists.txt#L79</a> accordingly if target Android phone is equipped with Qualcomm Snapdragon 8Gen3 series SoC or Qualcomm Snapdragon 8Elite series mobile SoC
