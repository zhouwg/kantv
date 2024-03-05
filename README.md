# KanTV

KanTV("Kan", aka Chinese PinYin "Kan" or Chinese HanZi "看" or English "watch/listen") , an open source project focus on Kan(aka "Watch/Listen" in English) online TV for **Android-based device**，derived from original ![ijkplayer](https://github.com/bilibili/ijkplayer) , with many enhancements:

- Watch online TV(by customized ![FFmpeg](https://github.com/cdeos/FFmpeg) and Exoplayer with updated version:FFmpeg 6.1, Exoplayer 2.15, source code of customized FFmpeg could be found ![here](https://github.com/cdeos/FFmpeg) according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>, source code of customized Exoplayer(based on my ![previous study in Exoplayer](https://github.com/google/ExoPlayer/pull/2921)) could be found within source code of this project)

- Record online TV to automatically generate videos (usable for short video creators to generate short video materials but pls respect IPR)

- Watch local media (movies, videos, music, etc.) on Android-based mobile phone

- Set up a custom playlist and then use this software to watch the content of the custom playlist

- Performance benchmark for Android-based mobile phone

- VoD & Live encrypted content(by customized <a href="https://github.com/shaka-project/shaka-packager" target="_blank">Google ShakaPackager</a>) playback with ChinaDRM(similar to <a href="http://www.widevine.com/" target="_blank">Google Widevine</a> or <a href="https://www.microsoft.com/playready/" target="_blank">Microsoft's PlayReady</a>) server

- VoD encrypted content(by customized <a href="https://github.com/shaka-project/shaka-packager" target="_blank">Google ShakaPackager</a>) playback with <a href="https://developer.huawei.com/consumer/en/hms/huawei-wiseplay" target="_blank">Huawei's WisePlay</a> (similar to <a href="http://www.widevine.com/" target="_blank">Google Widevine</a>) server

- VoD & Live encrypted content(by customized <a href="https://github.com/shaka-project/shaka-packager" target="_blank">Google ShakaPackager</a>) playback with customized <a href="https://ossrs.net/lts/en-us/" target="_blank">SRS</a>

- UI refactor

- ......

The goal of this project is:

- Android **turn-key project** for AI experts/ASR researchers and software developers who was interested in device-side AI application
- Well-maintained open source and free online-TV player for Android-based mobile phone(or extended for Android-based tablet/TV)


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

- bazel
  
  download ![bazel-3.1.0](https://github.com/bazelbuild/bazel/releases?page=5) and install bazel manually

```
  wget https://github.com/bazelbuild/bazel/releases/download/3.1.0/bazel-3.1.0-linux-x86_64
```
```
  sudo ./bazel-3.1.0-installer-linux-x86_64.sh
```

- Android NDK & Android Studio

  download and install Android Studio and Android NDK manually
  
  [Android Studio 4.2.1](https://developer.android.google.cn/studio)
  
  [Android NDK-r18b](https://developer.android.com/ndk/downloads)
  
  [Android NDK-r21e](https://developer.android.com/ndk/downloads)

bazel and Android NDK aren't used currently but put them here for further usage in the future.Of course, **these two steps could be skipped** currently.


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

#### Fetch source codes

For KanTV Android APK developers,

```
git clone https://github.com/cdeos/kantv.git
cd kantv
git checkout master
```

For other developers,

```
git clone --recurse-submodules https://github.com/cdeos/kantv.git
cd kantv
git checkout master
```


#### Build Android APK

- Build APK from source code by Android Studio IDE manually(the size of the generated APK is about 38M).

  Please attention ![some source codes in ASRFragment.java](https://github.com/cdeos/kantv/blob/master/cdeosplayer/kantv/src/main/java/com/cdeos/kantv/ui/fragment/ASRFragment.java#L131) which affect the running of the ASR demo and the size of the generated APK.

- Latest prebuit APK could be found here [![Github](https://user-images.githubusercontent.com/6889919/122489234-c13db400-d011-11eb-9d8c-8e4b2555dabe.png)](https://github.com/cdeos/kantv/raw/master/release/kantv-latest.apk)(the size of the prebuilt APK is about 90M because it contains **dependent model file of DeepSpeech** for purpose of make ASR demo happy).


### Run Android APK on real Android phone

This apk follows the principles of '**minimum permissions**' and '**do not collect unnecessary user data**' or EU's GDPR principle. When installing/using for the first time on an Android phone, only the following two permissions are required：

- Access to storage is required to generate necessary temporary files
- Access to device information is required to obtain current phone network status information, distinguishing whether the current network is Wi-Fi or mobile when playing online TV

The following is some English snapshots.

![Screenshot_20240301_000503_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/07653f3d-1e7a-4208-a3d8-90b3aecc30b4)
![Screenshot_20240301_000509_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/28d549ba-2fd5-434f-bf7a-b66d82d6dde3)
![990238413](https://github.com/zhouwg/kantv/assets/6889919/44054d57-0149-4d45-8762-46ec80682c66)



![Screenshot_20240301_114059_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/b0171435-44a5-48bf-9b59-a4b5fbcaa39f)
![Screenshot_20240301_114116_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/10224799-cdf8-46f7-acd0-6df64f0fc674)
![Screenshot_2024_0304_131033](https://github.com/zhouwg/kantv/assets/6889919/6c5bd531-5577-4570-bc87-aa3a87822d6b)



![Screenshot_20240301_000602_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/e3c6b89d-b1cf-42d8-87d0-f4a45074ebba)
![Screenshot_20240301_000609_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/cf3a77ef-1409-4137-8236-487a8de7fe81)


Other English and Chinese snapshots could be found in ![release directory](https://github.com/cdeos/kantv/tree/kantv/release).


### ChangeLog

Changelog could be found <a href="https://github.com/cdeos/kantv/blob/kantv/release/README.md">here</a>.


### Roadmap

- real-time English subtitle with online TV by excellent and amazing ![whisper.cpp](https://github.com/ggerganov/whisper.cpp) on Xiaomi 14(because it contains a very powerful mobile SoC)

- real-time Chinese subtitle with online TV by excellent and amazing ![whisper.cpp](https://github.com/ggerganov/whisper.cpp) on Xiaomi 14(because it contains a very powerful mobile SoC)

- integrate ![gstreamer](https://github.com/cdeos/gstreamer) to project KanTV(<a href="https://www.videolan.org/vlc/" target="_blank">VLC</a> is also excellent and gstreamer is more complicated than VLC but gstreamer was supported by many semiconductor companies. anyway, they are both born in/come from EU)

- UI refactor and “align to" UI in China's most popular and successful app WeChat(learn from WeChat)

- ...


### How to setup customized KanTV server in local development env

The computing power and network bandwidth of default kantvserver is very low due to insufficient fund, so setup a local End-2-End development env is strongly recommended.

 - setup a http server(by apache or nginx) in local development env

 - modify kant server address in app

![1370107702](https://github.com/zhouwg/kantv/assets/6889919/1e994269-28be-4513-9f74-3973269b8832)

 - upload required files to local http server like this(dependent files for DeepSpeech could be found <a href="https://github.com/mozilla/DeepSpeech/releases/tag/v0.9.3">here</a>)
```
   apk ->                              http(s)://local_http_server/kantv/apk/kantv-latest.apk
   apk version ->                      http(s)://local_http_server/kantv/apk/kantv-version.txt
   audio.wav  ->                       http(s)://local_http_server/kantv/deepspeech/audio.wav
   deepspeech-0.9.3-models.tflite ->   http(s)://local_http_server/kantv/deepspeech/deepspeech-0.9.3-models.tflite
   deepspeech-0.9.3-models.scorer ->   http(s)://local_http_server/kantv/deepspeech/deepspeech-0.9.3-models.scorer
  

   something for whisper.cpp      ->   http(s)://local_http_server/kantv/whisper/something

   something for paddlespeech     ->   http(s)://local_http_server/kantv/paddlespeech/something
```

### How to create customized playlist for kantv apk

- create **test.m3u**(recommend name and it's hardcoded in source code) like this:

```
  #EXTM3U
  #EXTINF:-1,hls
  http(s)://local_http_server/kantv/media/test.hls
  #EXTINF:-1,dash
  http(s)://local_http_server/kantv/media/test.dash
  #EXTINF:-1,rtmp
  http(s)://local_http_server/kantv/media/test.rtmp
  #EXTINF:-1,webrtc
  http(s)://local_http_server/kantv/media/test.rtc
  #EXTINF:-1,hevc(h265)
  http(s)://local_http_server/kantv/media/test.hevc
  #EXTINF:-1,h266
  http(s)://local_http_server/kantv/media/test.h266
  #EXTINF:-1,av1
  http(s)://local_http_server/kantv/media/test.av1
  #EXTINF:-1,testvideo-1 (pls attention following path is start with /)
  /test.mp4
  #EXTINF:-1,testvideo-2
  /video/test.ts

```

or just fetch favourite playlist from <a href="https://github.com/iptv-org/iptv">IPTV</a> and rename it to test.m3u(pls attention that users/developers from Mainland China should review <a href="https://github.com/cdeos/kantv/issues/27">this issue</a>)

 - upload test.m3u to local http server like this

```
 test.m3u                  ->   http(s)://local_http_server/kantv/epg/test.m3u
```


### How to integrate proprietary codes with project KanTV for proprietary R&D activity

For AI expert who want to integrate **proprietary codes**(which contains IPR and consists of Java/JNI/native...) to customized/derived project of KanTV, Please refer to this opening issue <a href="https://github.com/cdeos/kantv/issues/74">How to integrate proprietary codes for proprietary R&D activity</a>


### Support

- Please do not send e-mail to me for technical question. Public technical discussion on github is preferred.
- feel free to submit issues or new features(focus on Android at the moment), volunteer support would be provided if time permits.


### Contribution

Be sure to review the [opening issues](https://github.com/cdeos/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/cdeos/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/cdeos/kantv/issues/1).

Report issue in various Android-based phone or even submit PR to this project is great welcomed.

 **English** is preferred in this project. thanks for cooperation and understanding.

### Acknowledgement

Many/sincerely thanks to all contributors of the great open source community, especially all original authors and all contributors of the great Linux & Android & FFmpeg and other excellent projects. 

Project KanTV has used/tried following open-source projects:
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
 	<li>......</li>
</ul>
 
### License

```
Copyright (c) 2017 Bilibili
Licensed under LGPLv2.1 or later
```

```
Copyright (c) 2021 -  Authors of project KanTV

Licensed under Apachev2.0 or later
```

### Commercial Use
Project KanTV is licensed under Apachev2.0 or later, so itself is free/open for commercial use.
