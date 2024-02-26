# KanTV

KanTV("Kan", aka Chinese PinYin "Kan" or Chinese HanZi "看" or English "watch/listen") , an open source project focus on Kan(aka "Watch/Listen" in English) online TV for **Android-based device**，derived from original ![ijkplayer](https://github.com/bilibili/ijkplayer) , with many enhancements:

- Watch online TV(by customized FFmpeg and Exoplayer and gstreamer with updated version:FFmpeg 6.1, Exoplayer 2.15, gstreamer 1.23)

- Record online TV to automatically generate videos (usable for short video creators to generate short video materials but pls respect IPR)

- Watch local media (movies, videos, music, etc.) on your mobile phone

- Set up a custom playlist and then use this software to watch the content of the custom playlist

- Performance benchmark for mobile phone

- UI refactor


Pls attention this is **NOT** the latest source code of project KanTV. I think it should be fully here someday.


### How to build project

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


- [Android NDK-r21e(LTS)](https://developer.android.com/ndk/downloads)
- [Android Studio 4.2.1](https://developer.android.google.cn/studio)
- [Gradle 6.6.1](https://gradle.org/releases)

#### Fetch source codes

```
git clone https://github.com/zhouwg/kantv
cd kantv
git checkout kantv
```

#### Build Android APK


build apk by latest Android Studio IDE


#### Run kantv apk on real Android phone

<h2>snapshot (Chinese)</h2>

![Screenshot_20240220_085618_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/ac4f447a-35ea-411e-b34e-b818c8050b28)


![Screenshot_20240220_085622_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/e34ac861-8242-4fcc-9f22-7e7d63ab27dc)

![Screenshot_20240220_085630_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/171d37d3-e709-4b72-9025-1bc214d334af)

![Screenshot_20240220_085636_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/4a32fd83-5731-478a-88d5-de4c089df56c)


![Screenshot_20240220_085646_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/ab5e1ed8-d724-43eb-8ed7-3af23b0706b7)

![1453910779](https://github.com/zhouwg/kantv/assets/6889919/23692a5b-f897-4bcd-9955-57c2fcc7cc1c)



![371274193](https://github.com/zhouwg/kantv/assets/6889919/b189260b-6011-4c18-a3c0-cd55c2a36b8f)

![1398616229](https://github.com/zhouwg/kantv/assets/6889919/e798b58c-554c-443f-ad76-59f795760ed5)

![1476549041](https://github.com/zhouwg/kantv/assets/6889919/badae664-c864-4d84-87a4-f3d5636c4ff6)


![1109625356](https://github.com/zhouwg/kantv/assets/6889919/829a88d1-a021-4766-a3fc-c65265cc3fdc)


![Screenshot_20240222_111241_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/5762ce61-3e4a-4cbf-988f-65e7910a616d)


![Screenshot_20240222_111248_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/7a8c110d-5f8a-47a8-8018-a5c9d0e1d30a)

![Screenshot_20240222_111326_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/2794894b-1911-465c-a274-cfff9b116e40)


<h2>snapshot (English)</h2>

TBD




### Support

- Please do not send e-mail to me. Public technical discussion on github is preferred.
- feel free to submit issues or new features(focus on Android at the moment), volunteer support would be provided if time permits.



### Contribution

 If you want to contribute to project DeepSpeech, be sure to review the [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue).

 We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).

 

### Acknowledgement

Many/sincerely thanks to all contributors of the great open source community, especially all original authors and all contributors of the great Linux & Android & FFmpeg and other excellent projects. 

The KanTV application software has used the following open-source projects. Thanks to the following open-source software projects:
<ul>
 	<li><a href="http://ffmpeg.org/" target="_blank" rel="noopener">FFmpeg</a></li>
 	<li><a href="https://blog.google/products/android/" target="_blank" rel="noopener">Android</a></li>
 	<li><a href="https://github.com/bilibili/ijkplayer" target="_blank" rel="noopener">ijkplayer</a></li>
 	<li><a href="https://github.com/google/ExoPlayer" target="_blank" rel="noopener">ExoPlayer</a></li>
 	<li><a href="https://gstreamer.freedesktop.org/" target="_blank" rel="noopener">GStreamer</a></li>
 	<li><a href="https://www.videolan.org/vlc/" target="_blank" rel="noopener">libx264/libx265</a></li>
 	<li><a href="https://github.com/ggerganov/whisper.cpp" target="_blank" rel="noopener">whisper.cpp</a></li>
 	<li><a href="https://github.com/openai/whisper" target="_blank" rel="noopener">OpenAI/Whisper</a></li>
 	<li><a href="https://github.com/mozilla/DeepSpeech" target="_blank" rel="noopener">DeepSpeech</a></li>
 	<li><a href="https://www.intel.com/content/www/us/en/developer/articles/technical/scalable-video-technology.html" target="_blank" rel="noopener">SVT-AV1</a></li>
 	<li><a href="https://github.com/fraunhoferhhi/vvenc" target="_blank" rel="noopener">VVenc</a></li>
 	<li><a href="https://github.com/deniskropp/DirectFB" target="_blank" rel="noopener">DirectFB</a></li>
 	<li><a href="https://www.libsdl.org/" target="_blank" rel="noopener">SDL</a></li>
 	<li><a href="https://www.intel.com/content/www/us/en/developer/articles/technical/scalable-video-technology.html" target="_blank" rel="noopener">SVT-HEVC</a></li>
 	<li><a href="https://aomedia.org/" target="_blank" rel="noopener">AOM-AV1</a></li>
 	<li><a href="https://opencv.org/" target="_blank" rel="noopener">OpenCV</a></li>
 	<li><a href="https://webrtc.github.io/webrtc-org/start/" target="_blank" rel="noopener">WebRTC</a></li>
 	<li><a href="https://github.com/PaddlePaddle/PaddleSpeech" target="_blank" rel="noopener">PaddleSpeech</a></li>
 	<li><a href="https://github.com/Tencent/ncnn" target="_blank" rel="noopener">Tencent/ncnn</a></li>
 	<li><a href="https://github.com/shaka-project/shaka-packager" target="_blank" rel="noopener">ShakaPackager</a></li>
 	<li><a href="https://github.com/ossrs/srs" target="_blank" rel="noopener">SRS</a></li>
 	<li><a href="https://nginx.org/en/" target="_blank" rel="noopener">Nginx</a></li>
 	<li>......</li>
</ul>

required features are based on or derives from projects below:

- LGPL
  - [FFmpeg](http://git.videolan.org/?p=ffmpeg.git)
  - [libVLC](http://git.videolan.org/?p=vlc.git)
  - [kxmovie](https://github.com/kolyvan/kxmovie)
  - [soundtouch](http://www.surina.net/soundtouch/sourcecode.html)
- zlib license
  - [SDL](http://www.libsdl.org)
  - [zlib](http://www.zlib.net/)
- BSD-style license
  - [libyuv](https://code.google.com/p/libyuv/)
- ISC license
  - [libyuv/source/x86inc.asm](https://code.google.com/p/libyuv/source/browse/trunk/source/x86inc.asm)

- Apache License 2.0
  - [ExoPlayer](https://github.com/google/ExoPlayer)
  - [tensorflow](https://github.com/tensorflow/tensorflow)

- libxml2 license
  - [libxml2](http://xmlsoft.org/)

- libcurl license
  - [curl](https://curl.se/libcurl/)

- GPL license
  - [libiconv](https://www.gnu.org/software/libiconv/)

- Github license
  - [VoisePlayingIcon](https://github.com/chaohengxing/VoisePlayingIcon/)
 

### License

```
Copyright (c) 2017 Bilibili
Licensed under LGPLv2.1 or later
```

```
Copyright (c) 2021 -  maintainer of kantv project

Licensed under Apachev2.0 or later
```
