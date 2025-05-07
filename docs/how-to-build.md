
#### Fetch source codes

```

git clone https://github.com/kantv-ai/kantv.git

cd kantv

git checkout master

cd kantv

```

#### Setup development environment

    - prerequisites

      Ubuntu 20.04, 22.04, 24.04 is recommended.

    - tools & utilities

    run below script accordingly after fetch project's source code

    ```

    ./build/prebuild.sh


    ```

    - download and install Android Studio manually

      download Android Studio Jellyfish (| 2023.3.1 April 30, 2024) from https://developer.android.com/studio/archive
![Screenshot from 2025-05-07 22-06-08](https://github.com/user-attachments/assets/bb801dfe-57a7-4832-a40d-bd1e39c9904e)


    - vim settings


    borrow from http://ffmpeg.org/developer.html#Editor-configuration

    ```
    set ai
    set nu
    set expandtab
    set tabstop=4
    set shiftwidth=4
    set softtabstop=4
    set noundofile
    set nobackup
    set fileformat=unix
    set undodir=~/.undodir
    set cindent
    set cinoptions=(0
    " Allow tabs in Makefiles.
    autocmd FileType make,automake set noexpandtab shiftwidth=8 softtabstop=8
    " Trailing whitespace and tabs are forbidden, so highlight them.
    highlight ForbiddenWhitespace ctermbg=red guibg=red
    match ForbiddenWhitespace /\s\+$\|\t/
    " Do not highlight spaces at the end of line while typing on that line.
    autocmd InsertEnter * match ForbiddenWhitespace /\t\|\s\+\%#\@<!$/

    ```

 - download android-ndk and android-sdk to prebuilts/toolchain
    ```
    . build/envsetup.sh

    ./build/prebuild-download.sh

    ```





 - Remove the hardcoded debug flag in Android NDK <a href="https://github.com/android-ndk/ndk/issues/243">android-ndk issue</a>

    ```

    # open $ANDROID_NDK/build/cmake/android.toolchain.cmake for ndk < r23
    # or $ANDROID_NDK/build/cmake/android-legacy.toolchain.cmake for ndk >= r23
    # delete "-g" line
    list(APPEND ANDROID_COMPILER_FLAGS
    -g
    -DANDROID

    ```



#### Build native codes

```shell
. build/envsetup.sh

```

![Screenshot from 2024-04-07 09-45-04](https://github.com/zhouwg/kantv/assets/6889919/44a1f614-902c-48c1-babc-a73511c3a0f6)


#### Build Android APK

- Option 1: Build APK from source code by Android Studio IDE

- Option 2: Build APK from source code by command line

        . build/envsetup.sh
        lunch 1
        ./build-all.sh android

#### How to enable debug build

Modify <a href="https://github.com/zhouwg/kantv/blob/master/android/kantvplayer/build.gradle#L17">kantvplayer/build.gradle</a> accordingly

#### How to build project for Android phone equipped <b>without</b> Qualcomm mobile SoC

 - Modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L12">ggml/CMakeLists.txt#L12</a> accordingly if target Android phone is <b>NOT</b> equipped with Qualcomm mobile SoC

-  Modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L46">ggml/CMakeLists.txt#L46</a> accordingly if target Android phone is equipped with Qualcomm Snapdragon 8Gen3 SoC or Qualcomm Snapdragon 8Elite mobile SoC

- modify [ggml/CMakeLists.txt#L12](https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L12) accordingly.
