# KanTV

KanTV("Kan", aka Chinese PinYin "Kan" or Chinese HanZi "看" or English "watch/listen") , an open source project focus on study and practise state-of-the-art AI technology in <b>real application / real scenario</b>(such as online-TV playback and online-TV transcription(real-time subtitle) and online-TV language translation and online-TV video&audio recording works at the same time) on **mobile device**, derived from original ![ijkplayer](https://github.com/zhouwg/kantv/tree/kantv-initial) , with much enhancements and new features:

- Watch online TV and local media by customized ![FFmpeg 6.1](https://github.com/zhouwg/FFmpeg), source code of customized FFmpeg 6.1 could be found in <a href="https://github.com/zhouwg/kantv/tree/master/external/ffmpeg-6.1"> external/ffmpeg </a>according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>

- Record online TV to automatically generate videos (useful for short video creators to generate short video materials but pls respect IPR of original content creator/provider); record online TV's video / audio content for gather video / audio data which might be required of/useful for AI R&D activity

- ASR(Automatic Speech Recognition, a subfiled of AI) study by the great <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>

- LLM(Large Language Model, a subfiled of AI) study by the great <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> 

- SD(Text to Image by Stable Diffusion, a subfiled of AI) study by the amazing <a href="https://github.com/leejet/stable-diffusion.cpp">stablediffusion.cpp </a> 


- Real-time English subtitle for English online-TV(aka OTT TV) by the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>(<a href="https://github.com/zhouwg/kantv/issues/64">PoC finished on Xiaomi 14</a>. Xiaomi 14 or other powerful Android mobile phone is HIGHLY required/recommended for real-time subtitle feature otherwise unexpected behavior would happen)

- Run/experience LLM(such as llama-2-7b, baichuan2-7b, qwen1_5-1_8b, gemma-2b) on Xiaomi 14 using the amazing <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a>

- Set up a customized playlist and then use this software to watch the content of the customized playlist for R&D activity


- UI refactor(closer to real commercial Android application and only English is supported in UI language currently)



Some goals of this project are:

- Well-maintained "workbench" for ASR(Automatic Speech Recognition) researchers who was interested in practise state-of-the-art AI tech(like [whisper.cpp](https://github.com/ggerganov/whisper.cpp)) in real scenario on mobile device(focus on Android currently)

- Well-maintained "workbench" for LLM(Large Language Model) researchers who was interested in practise state-of-the-art AI tech(like [llama.cpp](https://github.com/ggerganov/llama.cpp)) in real scenario on mobile device(focus on Android currently)

- Android <b>turn-key project</b> for AI experts/researchers(whom mightbe not familiar with <b>regular Android software development</b>) focus on device-side AI R&D activity, part of AI R&D activity(algorithm improvement, model training, model generation, algorithm validation, model validation, performance benchmark......) could be done by Android Studio IDE + a powerful Android phone very easily


### Software architecture of KanTV Android

(this is proposal and depend on https://github.com/zhouwg/kantv/issues/121)

![kantv-android-arch](https://github.com/zhouwg/kantv/assets/6889919/3d850ce3-f8b5-4fc7-9ce7-cc8300717b45)

### How to build project

<details>
  <summary><b>Prerequisites</b></summary>

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




#### Fetch source codes

```

git clone https://github.com/zhouwg/kantv.git

cd kantv

git checkout master

cd kantv

```

#### Configure local development environment

 - download android-ndk-r26c to prebuilts/toolchain, skip this step if android-ndk-r26c is already exist

```
. build/envsetup.sh

./build/prebuild-download.sh

```


 - modify <a href="https://github.com/zhouwg/kantv/blob/master/build/envsetup.sh#L88">build/envsetup.sh</a> accordingly before launch build

 - modify <a href="https://github.com/zhouwg/kantv/blob/master/core/ggml/CMakeLists.txt#L16">ggml/CMakeLists.txt</a> accordingly if build-target is kantv-android and running Android device is NOT Xiaomi 14


#### Build native codes

```

. build/envsetup.sh


```

![Screenshot from 2024-04-07 09-45-04](https://github.com/zhouwg/kantv/assets/6889919/44a1f614-902c-48c1-babc-a73511c3a0f6)


#### Build Android APK

- Option 1: Build APK from source code by Android Studio IDE manually

- Option 2: Build APK from command line

        . build/envsetup.sh
        lunch 1
        ./build-all.sh android

  Please attention ![some source codes in ASRResearchFragment.java](https://github.com/zhouwg/kantv/blob/master/cdeosplayer/kantv/src/main/java/com/cdeos/kantv/ui/fragment/ASRResearchFragment.java#L156) which affect the running of the ASR demo and the size of the generated APK.

- Latest prebuit APK could be found here [![Github](https://user-images.githubusercontent.com/6889919/122489234-c13db400-d011-11eb-9d8c-8e4b2555dabe.png)](https://github.com/zhouwg/kantv/raw/master/release/kantv-latest.apk)(the prebuilt APK sometimes might be not available because generate APK from source code is preferrred).


### Run Android APK on real Android phone

This Android APK works well on any <b>mainstream</b> Qualcomm mobile SoC based Android phone.

The UI Layer of Project KanTV(this Android APK) follows the principles of '**minimum permissions**' and '**do not collect unnecessary user data**' or EU's GDPR principle. When installing/using for the first time on an Android phone, only the following two permissions are required：

- Access to storage is required to generate necessary temporary files
- Access to device information is required to obtain current phone network status information, distinguishing whether the current network is Wi-Fi or mobile when playing online TV

<hr>
here is a short video to demostrate AI subtitle by running the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on a Xiaomi 14 device - <b>fully offline, on-device</b>.

https://github.com/zhouwg/kantv/assets/6889919/0f79799a-ca56-4b6d-a83b-239c955b0372

<hr>
better performance with better stability after finetune(sometimes whisper.cpp will produce meaningless repeat tokens) with new method which introduced in https://github.com/ggerganov/whisper.cpp/issues/1951

https://github.com/zhouwg/kantv/assets/6889919/2fabcb24-c00b-4289-a06e-05b98ecd22b8

----

![1697162123](https://github.com/zhouwg/kantv/assets/6889919/d6b9ab54-ff27-43f7-b169-25c614ca3280)

<details>
  <summary>some other screenshots</summary>
  <ol>

![784269893](https://github.com/zhouwg/kantv/assets/6889919/8fe74b2a-21bc-452c-a6bb-5fb7fb2a567a)
![205726588](https://github.com/zhouwg/kantv/assets/6889919/16411854-c67b-4975-9ca1-fabcfe95a62b)
![1904016769](https://github.com/zhouwg/kantv/assets/6889919/a6b14cb1-8e3c-436d-89f1-b0c7adeaf00a)

![1377769652](https://github.com/zhouwg/kantv/assets/6889919/c9e56c16-2821-4cef-ab2c-bc43a36af05a)

![1778831978](https://github.com/zhouwg/kantv/assets/6889919/92774cbc-c716-4819-a0c1-6bc0ae495d1d)

  

![Screenshot_2024_0304_131033](https://github.com/zhouwg/kantv/assets/6889919/6c5bd531-5577-4570-bc87-aa3a87822d6b)

![154248860](https://github.com/zhouwg/kantv/assets/6889919/071ac55c-a5d7-4bd6-aece-83cbc8a487ff)

![1118975128](https://github.com/zhouwg/kantv/assets/6889919/ef2b256c-02fb-4318-a430-b4cd15ed5b44)
![Screenshot_20240301_000609_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/cf3a77ef-1409-4137-8236-487a8de7fe81)

![1966093505](https://github.com/zhouwg/kantv/assets/6889919/44e6d6c7-0ebb-41c0-a576-9f7457e0dd23)

![1179733910](https://github.com/zhouwg/kantv/assets/6889919/eb6ff245-3f04-4689-b998-2b6b06dec432)

![2138671817](https://github.com/zhouwg/kantv/assets/6889919/513a0676-d2f3-464b-be50-436eabd99715)

![1634808790](https://github.com/zhouwg/kantv/assets/6889919/f69ca9a7-5d25-46da-a160-ab00ff059db9)

![991182277](https://github.com/zhouwg/kantv/assets/6889919/46306999-973c-4fc4-b294-48025acf9cf5)

  </ol>
</details>


### Hot topics

- <a href="https://github.com/zhouwg/kantv/issues/121">add Qualcomm mobile SoC native backend for GGML</a>

- improve <b>quality</b> of real-time English subtitle which powered by great and excellent and amazing ![whisper.cpp](https://github.com/ggerganov/whisper.cpp)

- real-time Chinese subtitle for online English TV by great and excellent and amazing ![whisper.cpp](https://github.com/ggerganov/whisper.cpp)

- bugfix in UI layer(Java)

- bugfix in native layer(C/C++)

- participate in improvement of ![whisper.cpp](https://github.com/ggerganov/whisper.cpp) on Android device and feedback to upstream


### Contribution

Be sure to review the [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).

Report issue in various Android-based phone or even submit PR to this project is greatly welcomed.

 **English** is preferred in this project(avoid similar comments in this project:<a href="https://github.com/torvalds/linux/pull/818" target="_blank">https://github.com/torvalds/linux/pull/818</a>). thanks for cooperation and understanding.


### Docs

- [How to setup customized KanTV server in local dev env](./docs/how-to-setup-customized-kantvserver-in-local.md)
- [How to create customized playlist for kantv apk](./docs/how-to-create-customized-playlist-in-cloud-server.md)
- [How to integrate proprietary/open source codes to project KanTV for personal/proprietary/commercial R&D activity](https://github.com/zhouwg/kantv/issues/74)
- [How to use whisper.cpp and ffmpeg to add subtitle to video](./docs/how-to-use-whispercpp-ffmpeg-add-subtitle-to-video.md)
- [Acknowledgement](./docs/acknowledgement.md)
- [ChangeLog](./release/README.md)
- [F.A.Q](./docs/FAQ.md)


### Special Acknowledgement

the AI part of this project is <b>heavily depend on</b> [ggml](https://github.com/ggerganov/ggml) and [whisper.cpp](https://github.com/ggerganov/whisper.cpp) and [llama.cpp](https://github.com/ggerganov/llama.cpp) by [Georgi Gerganov](https://github.com/ggerganov).

### License

```
Copyright (c) 2021 - 2023 Project KanTV

Copyright (c) 2024 -  Authors of Project KanTV

Licensed under Apachev2.0 or later
```
