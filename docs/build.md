
#### Fetch source codes

```

git clone https://github.com/zhouwg/kantv.git

cd kantv

git checkout master

cd kantv

```

#### Setup development environment

##### Option 1: Setup docker environment

- Build docker image
  ```shell
  docker build build -t kantv --build-arg USER_ID=$(id -u) --build-arg GROUP_ID=$(id -g) --build-arg USER_NAME=$(whoami)
  ```

- Run docker container
  ```shell
  # map source code directory into docker container
  docker run -it --name=kantv --volume=`pwd`:/home/`whoami`/kantv kantv

  # in docker container
  . build/envsetup.sh

  ./build/prebuild-download.sh
  ```

##### Option 2: Setup local environment


  - <details>
      <summary>Prerequisites</summary>

      <ol>

        Host OS information:

    ```
    uname -a

    Linux 5.8.0-43-generic #49~20.04.1-Ubuntu SMP Fri Feb 5 09:57:56 UTC 2021 x86_64 x86_64 x86_64 GNU/Linux

    cat /etc/issue

    Ubuntu 20.04.2 LTS \n \l

    ```
    - tools & utilities
    ```
    sudo apt-get update
    sudo apt-get install build-essential -y
    sudo apt-get install cmake -y
    sudo apt-get install curl -y
    sudo apt-get install wget -y
    sudo apt-get install python -y
    sudo apt-get install tcl expect -y
    sudo apt-get install nginx -y
    sudo apt-get install git -y
    sudo apt-get install vim -y
    sudo apt-get install spawn-fcgi -y
    sudo apt-get install u-boot-tools -y
    sudo apt-get install ffmpeg -y
    sudo apt-get install openssh-client -y
    sudo apt-get install nasm -y
    sudo apt-get install yasm -y
    sudo apt-get install openjdk-17-jdk -y

    sudo dpkg --add-architecture i386
    sudo apt-get install lib32z1 -y

    sudo apt-get install -y android-tools-adb android-tools-fastboot autoconf \
            automake bc bison build-essential ccache cscope curl device-tree-compiler \
            expect flex ftp-upload gdisk acpica-tools libattr1-dev libcap-dev \
            libfdt-dev libftdi-dev libglib2.0-dev libhidapi-dev libncurses5-dev \
            libpixman-1-dev libssl-dev libtool make \
            mtools netcat python-crypto python3-crypto python-pyelftools \
            python3-pycryptodome python3-pyelftools python3-serial \
            rsync unzip uuid-dev xdg-utils xterm xz-utils zlib1g-dev

    sudo apt-get install python3-pip -y
    sudo apt-get install indent -y
    pip3 install meson ninja

    echo "export PATH=/home/`whoami`/.local/bin:\$PATH" >> ~/.bashrc

    ```

    or run below script accordingly after fetch project's source code

    ```

    ./build/prebuild.sh


    ```

    - Android Studio

      download and install Android Studio manually

      [Android Studio 4.2.1 or latest Android Studio](https://developer.android.google.cn/studio)


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
      </ol>
    </details>


 - Download android-ndk-r26c to prebuilts/toolchain, skip this step if android-ndk-r26c is already exist
    ```
    . build/envsetup.sh

    ./build/prebuild-download.sh

    ```




 - Modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L12">ggml/CMakeLists.txt#L12</a> accordingly if target Android phone is <b>NOT</b> equipped with Qualcomm mobile SoC

-  Modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L46">ggml/CMakeLists.txt#L46</a> accordingly if target Android phone is equipped with Qualcomm Snapdragon 8Gen3 SoC or Qualcomm Snapdragon 8Elite mobile SoC

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
