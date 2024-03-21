# KanTV

KanTV("Kan", aka Chinese PinYin "Kan" or Chinese HanZi "看" or English "watch/listen") , an open source project focus on study and practise state-of-the-art AI technology in real application / real complicated scenario(such as online-TV playback and online-TV transcription(real-time subtitle) and online-TV language translation and online-TV video&audio recording works at the same time) on **Android-based device**, derived from original ![ijkplayer](https://github.com/bilibili/ijkplayer) , with much enhancements and new features:

- Watch online TV and local media by customized ![FFmpeg 6.1](https://github.com/cdeos/FFmpeg), source code of customized FFmpeg 6.1 could be found in directory external/ffmpeg or ![here](https://github.com/cdeos/FFmpeg) according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>)

- Record online TV to automatically generate videos (useful for short video creators to generate short video materials but pls respect IPR of original content creator/provider)

- Record online TV's video content for gather video data which might be required of/useful for AI R&D activity

- Record online TV's audio content for gather audio data which might be required of/useful for AI R&D activity

- Set up a custom playlist and then use this software to watch the content of the custom playlist

- Performance benchmark for Android-based mobile phone

- Real-time English subtitle for English online-TV(aka OTT TV) by the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>(<a href="https://github.com/cdeos/kantv/issues/64">PoC finished on Xiaomi 14</a>. Xiaomi 14 or other powerful Android mobile phone is HIGHLY required/recommended for real-time subtitle feature otherwise unexpected behavior would happen)

- UI refactor(closer to real commercial Android application and only English is supported in UI language currently)



Some goals of this project are:

- Well-maintained "workbench" for device-side AI R&D activity on <b>Android</b>-based device

  <ul>

    <li>Well-maintained "workbench" for ASR(Automatic Speech Recognition) researchers who was interested in practise state-of-the-art AI tech(like <a href="https://github.com/ggerganov/whisper.cpp">GGML's whisper.cpp</a>) in real scenario </li>

    <li>Well-maintained "workbench" for software programmer to learning AI technology in real scenario


  </ul>


- Android <b>turn-key project</b> for AI experts(whom mightbe not familiar with regular Android software development), part of AI research activity(algorithm improvement, model training, model generation, algorithm validation, model validation, performance benchmark......) could be done by Android Studio IDE + a powerful Android phone very easily

- Android <b>turn-key project</b> for software programmers who was interested in device-side AI application or customized/secondary software development activity on **Android**-based device

- Watch English online-TV(aka OTT TV) with real-time English subtitle for non-native English speakers learning English(listening and reading)



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

- Android NDK & Android Studio

  download and install Android Studio and Android NDK manually

  [Android Studio 4.2.1 or latest Android Studio](https://developer.android.google.cn/studio)


  [Android NDK-r26c](https://developer.android.com/ndk/downloads)


  then put Android NDK-r26c into /opt/kantv-toolchain accordingly

  ```
  ls /opt/kantv-toolchain/android-ndk-r26c

  ```


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

For Android APP/APK programmers or **AI experts**,

```
git clone https://github.com/cdeos/kantv.git
cd kantv
git checkout master
```

For C/C++ programmers,

```
git clone --recurse-submodules https://github.com/cdeos/kantv.git
cd kantv
git checkout master
```

#### Build all dependent native codes

modify <a href="https://github.com/cdeos/kantv/blob/master/build/envsetup.sh#L46">build/envsetup.sh</a> accordingly before launch build

pay attention <a href="https://github.com/cdeos/kantv/blob/master/external/whispercpp/CMakeLists.txt#L54">here and modify it accordingly</a> if target Android device is NOT Xiaomi 14

```
. build/envsetup.sh

time ./build-all.sh
```



#### Build Android APK

- Build APK from source code by Android Studio IDE manually

  Please attention ![some source codes in ASRResearchFragment.java](https://github.com/cdeos/kantv/blob/master/cdeosplayer/kantv/src/main/java/com/cdeos/kantv/ui/fragment/ASRResearchFragment.java#L159) which affect the running of the ASR demo and the size of the generated APK.

- Latest prebuit APK could be found here [![Github](https://user-images.githubusercontent.com/6889919/122489234-c13db400-d011-11eb-9d8c-8e4b2555dabe.png)](https://github.com/cdeos/kantv/raw/master/release/kantv-latest.apk)(the prebuilt APK sometimes might be not available because generate APK from source code is preferrred).


### Run Android APK on real Android phone

This apk follows the principles of '**minimum permissions**' and '**do not collect unnecessary user data**' or EU's GDPR principle. When installing/using for the first time on an Android phone, only the following two permissions are required：

- Access to storage is required to generate necessary temporary files
- Access to device information is required to obtain current phone network status information, distinguishing whether the current network is Wi-Fi or mobile when playing online TV

<hr>
here is a short video to demostrate AI subtitle by running the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on a Xiaomi 14 device - <b>fully offline, on-device</b>.

https://github.com/cdeos/kantv/assets/6889919/0f79799a-ca56-4b6d-a83b-239c955b0372

----

<details>
  <summary>some English screenshots</summary>
  <ol>
    
 ![1314307315](https://github.com/cdeos/kantv/assets/6889919/3636c650-231c-43ab-a5bf-9612d891c39f)

![Screenshot_20240301_000509_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/28d549ba-2fd5-434f-bf7a-b66d82d6dde3)

![127402405](https://github.com/cdeos/kantv/assets/6889919/e01da442-3eda-4fff-b771-0c485316416c)

![794969608](https://github.com/cdeos/kantv/assets/6889919/8e232f9a-7265-4c66-ba14-ab3df3d6d59b)

![631715309](https://github.com/cdeos/kantv/assets/6889919/86447fa3-1656-40fd-9e38-1d6471cc5478)
![1369354807](https://github.com/cdeos/kantv/assets/6889919/7f38f930-15f7-4e03-ace6-5b9d201ab19e)

![Screenshot_2024_0304_131033](https://github.com/zhouwg/kantv/assets/6889919/6c5bd531-5577-4570-bc87-aa3a87822d6b)

![154248860](https://github.com/cdeos/kantv/assets/6889919/071ac55c-a5d7-4bd6-aece-83cbc8a487ff)

![1118975128](https://github.com/cdeos/kantv/assets/6889919/ef2b256c-02fb-4318-a430-b4cd15ed5b44)
![Screenshot_20240301_000609_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/cf3a77ef-1409-4137-8236-487a8de7fe81)

![1966093505](https://github.com/cdeos/kantv/assets/6889919/44e6d6c7-0ebb-41c0-a576-9f7457e0dd23)

![1179733910](https://github.com/cdeos/kantv/assets/6889919/eb6ff245-3f04-4689-b998-2b6b06dec432)

![2138671817](https://github.com/cdeos/kantv/assets/6889919/513a0676-d2f3-464b-be50-436eabd99715)

![1634808790](https://github.com/cdeos/kantv/assets/6889919/f69ca9a7-5d25-46da-a160-ab00ff059db9)

![991182277](https://github.com/cdeos/kantv/assets/6889919/46306999-973c-4fc4-b294-48025acf9cf5)

  </ol>
</details>


### Hot topics

- improve real-time English subtitle performance with utilize hardware AI engine in Android device

- real-time Chinese subtitle for online English TV on Xiaomi 14(because it contains a very powerful mobile SoC) by great and excellent and amazing ![whisper.cpp](https://github.com/ggerganov/whisper.cpp)

- integrate ![gstreamer](https://github.com/cdeos/gstreamer) to project KanTV(<a href="https://www.videolan.org/vlc/" target="_blank">VLC</a> is also excellent and gstreamer is more complicated than VLC but gstreamer was supported by many semiconductor companies. anyway, they are both born in/come from EU)

- bugfix in UI layer(Java)

- bugfix in JNI layer(C/C++)

- participate in improvement of ![whisper.cpp](https://github.com/ggerganov/whisper.cpp) on Android device and feedback to upstream


### Contribution

Be sure to review the [opening issues](https://github.com/cdeos/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/cdeos/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/cdeos/kantv/issues/1).

Report issue in various Android-based phone or even submit PR to this project is greatly welcomed.

 **English** is preferred in this project(avoid similar comments in this project:<a href="https://github.com/torvalds/linux/pull/818" target="_blank">https://github.com/torvalds/linux/pull/818</a>). thanks for cooperation and understanding.


### Docs

- [How to setup customized KanTV server in local dev env](./docs/how-to-setup-customized-kantvserver-in-local.md)
- [How to create customized playlist for kantv apk](./docs/how-to-create-customized-playlist-in-cloud-server.md)
- [How to integrate proprietary/open source codes to project KanTV for personal/proprietary/commercial R&D activity](https://github.com/cdeos/kantv/issues/74)
- [Acknowledgement](./docs/acknowledgement.md)


### Supportive

- Please do not send e-mail to me for technical question. Public technical discussion on github is preferred.
- feel free to submit issues or new features(focus on Android at the moment), volunteer support would be provided if time permits.

### Sponsor

In Sep 2022, after I left my last employer, I became a full-time personal programmer. started writing some code for solving some technical problems in project KanTV(which was launched on 05/2021 personally) and also for practicing my C/C++/Java programming. Just for fun, I implemented online-TV recording feature on 12/2023 - something I did not expect at all.

With personal time/effort(personal purchase a Dell PC and Xiaomi 14 for software development activity, personal purchase Cloud Server for setup a dedicated proxy to cross the GFW and then access Google accordingly......), the project grew and now I want to seek external resource to help this personal project growing.

I don't have a oversea phone number and I could not create Github Sponsors account accordingly.I only have a Wechat account so I put my personal WeChat reward(aka "赞赏" in Chinese or "donation" in English) QR code here.In other words, sponsorship of this project can ONLY be done through WeChat Pay，thanks for your understanding.

Still, if you do decide to sponsor me, the money will most likely go towards buying various high-end powerful Android phone and pay for Cloud Server, or buy some coffee or buy a meal to potential volunteer programmer to participate in project's development.

Thanks!

### ChangeLog

Changelog could be found <a href="https://github.com/cdeos/kantv/blob/master/release/README.md">here</a>.


### License

```
Copyright (c) 2017 Bilibili
Licensed under LGPLv2.1 or later
```

```
Copyright (c) 2021 - 2023 Project KanTV

Copyright (c) 2024 -  Authors of Project KanTV

Licensed under Apachev2.0 or later
```

### Commercial Use

Project KanTV is licensed under Apachev2.0 or later, so itself is free/open for commercial use.
