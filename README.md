# KanTV

KanTV("Kan", aka Chinese PinYin "Kan" or Chinese HanZi "看" or English "watch/listen") , an open source project focus on Kan(aka "Watch/Listen" in English) online TV for Android-based phone，derived from original ijkplayer(https://github.com/bilibili/ijkplayer) , with many enhancements(recording online TV program, performance benchmark, realtime subtitle with online TV program...)



### How to build project for target Android

#### prerequisites

- Host OS information:

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
sudo apt-get install python -y
sudo apt-get install tcl expect -y
sudo apt-get install nginx -y
sudo apt-get install git -y
sudo apt-get install vim -y
sudo apt-get install spawn-fcgi -y
sudo apt-get install u-boot-tools -y
sudo apt-get install ffmpeg -y
sudo apt-get install openssh-client -y

sudo dpkg --add-architecture i386
sudo apt-get install lib32z1

sudo apt-get install android-tools-adb android-tools-fastboot autoconf \
        automake bc bison build-essential ccache cscope curl device-tree-compiler \
        expect flex ftp-upload gdisk acpica-tools libattr1-dev libcap-dev \
        libfdt-dev libftdi-dev libglib2.0-dev libhidapi-dev libncurses5-dev \
        libpixman-1-dev libssl-dev libtool make \
        mtools netcat python-crypto python3-crypto python-pyelftools \
        python3-pycryptodome python3-pyelftools python3-serial \
        rsync unzip uuid-dev xdg-utils xterm xz-utils zlib1g-dev

```

- Tensorflow

  setup prerequisites according to

  https://docs.bazel.build/versions/main/install-ubuntu.html

  https://www.tensorflow.org/lite/guide/build_cmake

  before

```
  sudo apt-get update && sudo apt-get install bazel-3.7.2

```


- vim settings

fetch from http://ffmpeg.org/developer.html#Editor-configuration

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


- [Android NDK-r21e(LTS)](https://developer.android.com/ndk/downloads)
- [Android Studio 4.2.1](https://developer.android.google.cn/studio)
- [Gradle 6.6.1](https://gradle.org/releases)

#### Before Build

```
git clone https://github.com/zhouwg/kantv
cd kantv
git checkout kantv


# add these lines to your ~/.bash_profile or ~/.profile
# export ANDROID_SDK=<your sdk path>
# ndk-r21e was used in this project, if you intall from SDK Manager
# export ANDROID_NDK=<your sdk path>/sdk/ndk/21.4.7075529

# add these line to ./android/ijkplayer/local.properties

sdk.dir=<your sdk path>

```

#### Build Android APK

step1:build all native libs

```
./build-all-native-libs.sh clean
./build-all-native-libs.sh init
time ./build-all-native-libs.sh build

```

step2: build APK


build apk by latest Android Studio IDE


### Snapshot(Chinese available)

![Screenshot_20240220_085618_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/ac4f447a-35ea-411e-b34e-b818c8050b28)





### How to build project for target iOS(will be deprecated soon)

#### prerequisites

- Host OS information:

![macos-info](https://user-images.githubusercontent.com/6889919/122509084-bac13380-d035-11eb-9d8a-ae676a11fa39.png)

```
uname -a

Darwin 19.6.0 Darwin kernel Version 19.6.0 x86_64

```

- brew and GNU build tools

```
brew install automake

brew install autoconf

brew install wget

wget http://mirrors.ustc.edu.cn/gnu/libtool/libtool-2.4.6.tar.xz

tar Jxf libtool-2.4.6.tar.xz

cd libtool-2.4.6

./configure --prefix=/usr/local

make;make install

```


- [Xcode-12.5](https://developer.apple.com/download/more/)
- [Command Line Tools for Xcode 12.5](https://developer.apple.com/download/more/)
- [Additional Tools for Xcode 12.5](https://developer.apple.com/download/more/)


#### Before Build

```
git clone https://github.com/zhouwg/kantv
cd kantv
git checkout kantv

```

#### Build iOS APP

step1:build all native libs

```
./build-all-native-libs.sh clean
./build-all-native-libs.sh init
time ./build-all-native-libs.sh build

```

step2: build APP


build APP by latest Xcode IDE



### Support

- Please do not send e-mail to me. Public technical discussion on github is preferred.
- feel free to submit issues or new features(focus on Android at the moment), volunteer support would be provided if time permits.


### License

```
Copyright (c) 2017 Bilibili
Licensed under LGPLv2.1 or later
```

```
Copyright (c) 2021 maintainer of kantv project

Licensed under Apachev2.0 or later
```

the original official ijkplayer required features are based on or derives from projects below:
- LGPL
  - [FFmpeg](http://git.videolan.org/?p=ffmpeg.git)
  - [libVLC](http://git.videolan.org/?p=vlc.git)
  - [kxmovie](https://github.com/kolyvan/kxmovie)
  - [soundtouch](http://www.surina.net/soundtouch/sourcecode.html)
- zlib license
  - [SDL](http://www.libsdl.org)
- BSD-style license
  - [libyuv](https://code.google.com/p/libyuv/)
- ISC license
  - [libyuv/source/x86inc.asm](https://code.google.com/p/libyuv/source/browse/trunk/source/x86inc.asm)

android/ijkplayer-exo is based on or derives from projects below:
- Apache License 2.0
  - [ExoPlayer](https://github.com/google/ExoPlayer)

android/example is based on or derives from projects below:
- GPL
  - [android-ndk-profiler](https://github.com/richq/android-ndk-profiler) (not included by default)

ios/IJKMediaDemo is based on or derives from projects below:
- Unknown license
  - [iOS7-BarcodeScanner](https://github.com/jpwiddy/iOS7-BarcodeScanner)

ijkplayer's build scripts are based on or derives from projects below:
- [gas-preprocessor](http://git.libav.org/?p=gas-preprocessor.git)
- [VideoLAN](http://git.videolan.org)
- [yixia/FFmpeg-Android](https://github.com/yixia/FFmpeg-Android)
- [kewlbear/FFmpeg-iOS-build-script](https://github.com/kewlbear/FFmpeg-iOS-build-script)





the kantv required features are based on or derives from projects below:

- zlib license
  - [zlib](http://www.zlib.net/)

- libxml2 license
  - [libxml2](http://xmlsoft.org/)

- libcurl license
  - [curl](https://curl.se/libcurl/)

- GPL license
  - [libiconv](https://www.gnu.org/software/libiconv/)

- Apache license
  - [tensorflow](https://github.com/tensorflow/tensorflow)
  - [irsa](https://github.com/zhouwg/irsa)

- Github license
  - [VoisePlayingIcon](https://github.com/chaohengxing/VoisePlayingIcon/)



kantv's build scripts was created and maintained by maintainer of kantv project, thanks to Linux&Android open source community, special thanks to Zhang Rui(<bbcallen@gmail.com>) & [Bilibili](https://www.bilibili.com/) for the born of original great ijkplayer.



### Commercial Use
ijkplayer is licensed under LGPLv2.1 or later, so itself is free for commercial use under LGPLv2.1 or later

But ijkplayer is also based on other different projects under various licenses, which I have no idea whether they are compatible to each other or to your product.

[IANAL](https://en.wikipedia.org/wiki/IANAL), you should always ask your lawyer for these stuffs before use it in your product.


kantv is licensed under Apachev2.0 or later, so itself is free for commercial use UNDER Apachev2.0 or later

### ChangeLog

[ChangeLog](https://github.com/zhouwg/kantv/tree/kantv/release)


### Contribution

 **If you want to contribute to Project KanTV, be sure to review the
 [ChangeLog](https://github.com/zhouwg/kantv/blob/kantv/release/README.md) and
 [NEWS](NEWS.md) and
 [build script](build-all-native-libs.sh) and
 [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue).**

 **We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs,
 please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).**

### KanTV Android APK download

> The KanTV apk is about 10M (mainly because the apk integrates FFmpeg +  chinadrm client subsystem libraries (only arm64-v8a for reduce apk's size);  or could be built the KanTV apk from [source code ](https://github.com/zhouwg/kantv) according to this [README](https://github.com/zhouwg/kantv/blob/kantv/README.md).


[![Github](https://user-images.githubusercontent.com/6889919/122489234-c13db400-d011-11eb-9d8c-8e4b2555dabe.png)](https://github.com/zhouwg/kantv/blob/kantv/release/kantv-latest.apk?raw=true)

### KanTV iOS APP download

TBD

