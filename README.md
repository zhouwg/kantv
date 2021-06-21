# hijkplayer

cross-platform video player based on [ffplay](http://ffmpeg.org) for Android & iOS, derived from original official [ijkplayer](https://github.com/bilibili/ijkplayer) , with some enhancements.



### Android

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


- [Android NDK-r14b](https://developer.android.com/ndk/downloads/older_releases)
- [Android Studio 4.2.1](https://developer.android.google.cn/studio)
- [Gradle 6.6.1](https://gradle.org/releases)

#### Before Build

```
git clone https://github.com/zhouwg/hijkplayer
cd hijkplayer
git checkout dev-baseon-latest-k0.8.8


# add these lines to your ~/.bash_profile or ~/.profile
# export ANDROID_SDK=<your sdk path>
# ndk-r14b was used in this project
# export ANDROID_NDK=<your ndk path>/android-ndk-r14b

# add these line to ./android/ijkplayer/local.properties

sdk.dir=<your sdk path>
ndk.dir=<your ndk path>/android-ndk-r14b

```

#### Build Android

step1:build all native libs

```
./build-all-native-libs.sh clean
./build-all-native-libs.sh init
time ./build-all-native-libs.sh build

```

step2: build APK


build apk by latest Android Studio IDE


### iOS

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
git clone https://github.com/zhouwg/hijkplayer
cd hijkplayer
git checkout dev-baseon-latest-k0.8.8

```

#### Build iOS

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


### Latest Changes
- [NEWS.md](NEWS.md)


  2021-06-18,00:13, launch branch "kantv" base on branch "dev-baseon-latest-k0.8.8"

  2021-06-18,00:13, rename project's name from hijkplayer to kantv, and this branch "dev-baseon-latest-k0.8.8" will be no longer supported from now on



### License

```
Copyright (c) 2017 Bilibili
Licensed under LGPLv2.1 or later
```

```
Copyright (c) 2021 maintainer of hijkplayer project

Licensed under Apachev2.0 or later
```

the original official ijkplayer & hijkplayer required features are based on or derives from projects below:
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

hijkplayer's build scripts was created and maintained by maintainer of hijkplayer project, thanks to Linux&Android open source community, special thanks to Zhang Rui(<bbcallen@gmail.com>) & [Bilibili](https://www.bilibili.com/) for the born of original great ijkplayer.

### Commercial Use
ijkplayer is licensed under LGPLv2.1 or later, so itself is free for commercial use under LGPLv2.1 or later

But ijkplayer is also based on other different projects under various licenses, which I have no idea whether they are compatible to each other or to your product.

[IANAL](https://en.wikipedia.org/wiki/IANAL), you should always ask your lawyer for these stuffs before use it in your product.


hijkplayer is licensed under Apachev2.0 or later, so itself is free for commercial use under Apachev2.0 or later
