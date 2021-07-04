### ChangeLog (版本更改记录)
- v0.0.1:

    kantv project was launched base on branch dev-baseon-latest-k0.8.8 in my on-going developing hijkplayer project;
    基于开发了较长时间的hijkplayer项目的dev-baseon-latest-k0.8.8分支启动kantv项目，用于观看在线电视节目/在线电影/收听在线广播，同时用于软件领域相关技术的学习与研究;

- v0.0.2:

    add development mode;
    增加了开发者模式，用于开发过程中的一些功能调试与分析解决bug;

- v0.0.3:

    add author/maintainer info;
    增加了版本/作者/以及源代码下载地址信息;

- v0.0.4:

    media category for online TV and online Movie;
    增加了在线电视/在线电影/在线广播的分类;
    
    disable progress bar when playback online TV;
    禁止观看在线电视时使用进度条与暂停按钮;
    
    display media name in Media Information dialog;
    在媒体信息对话框中增加了媒体节目信息;

- v0.0.5:

    add UI during long time waiting before the first video frame was rendered;
    在第一帧视频画面显示之前在界面上显示一个状态信息对话框；
    
    add UI during network buffering;
    在观看收听在线电视/在线电影/在线广播时如果网络质量不佳时在界面上显示一个正在缓冲的状态信息对话框;
    
    load EPG infos(aka online TV channels & online Radio programs & online Movie lists) from local XML and then render/layout the UI accordingly;
    从apk内置的xml文件中加载在线电视节目信息列表与在线广播节目信息列表以及在线电影节目信息列表；
    
   
- v0.0.6:

   add animation during playback online radio program;
   在播放在线广播时在界面上显示简单的动画

   UI bug fix;
   一些与UI相关的bugfix;

   add Chinese language for UI;
   在界面增加了中文语言，海外地区界面默认为英文，国内界面默认为中文;
   
   add toggle UI language between Chinese and English;
   在设置中增加了切换界面语言功能;
   
   add option menu highlight and start activity with SINGLE_TOP mode；
   增加了高亮导航菜单；
   
   add "quit" option in toolbar;
   增加了“退出”子菜单，用户可以通过此子菜单直接退出APK;

- v0.0.7:

   UI modification for purpose of improve user's experience;
   UI的一些改动，为了提升用户使用APK的体验;

   remove online movie lists which contain illegal advertisement;
   移除含有非法广告的在线电影;

   update NDK version to r21.4.7075529 LTS for prepare switch to AndroidX, validated on arm64 target only;
   将NDK升级到r21.4.7075529 LTS,为切换到AndroidX做准备;


### Screenshots(一些屏幕截图）

English UI(英文界面，海外地区默认为英文界面）
-------------------------------------------------------------

![Screenshot_20210627_111103_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531948-42e0c080-d73b-11eb-87b2-556e8b8feb13.jpg)
![Screenshot_20210627_111115_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531950-46744780-d73b-11eb-8fda-1266a4c5c740.jpg)

![Screenshot_20210627_111130_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531955-4ffdaf80-d73b-11eb-9aa1-2c8d01c9be49.jpg)

Chinese UI（中文界面，国内默认为中文界面）
-------------------------------------------------------------
![Screenshot_20210627_112635_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531869-c221c480-d73a-11eb-8204-221e56c23d5e.jpg)
![Screenshot_20210627_112651_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531872-c77f0f00-d73a-11eb-89d0-04a7d1490ae1.jpg)


![Screenshot_20210627_111156_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531876-cfd74a00-d73a-11eb-902e-b18cebe69597.jpg)

![Screenshot_20210627_111205_tv danmaku ijk kantv](https://user-images.githubusercontent.com/6889919/123531879-d5349480-d73a-11eb-8234-7d9d808280ab.jpg)

![Screenshot_20210626_222306_tv 9](https://user-images.githubusercontent.com/6889919/123516218-8c92c200-d6cd-11eb-9474-f3767db37b29.jpg)






### Advantage (优点）

- purely client application and purely GREEN application, the released apk MUST BE built from the source code of project kantv to avoid collecting/uploading device info/user's datas and ensure better user experience for apk's users; 纯客户端软件，无任何插件，不会与任何远程服务器交互，不会收集上传用户手机的设备信息以及手机中的文件，只能用于观看收听在线电视/在线广播/在线电影;
- complete（except source code of libdrmclient.so because it contains proprietary IPR） open source project；  接近于完全开源（libdrmclient.so出于知识产权因素无法开源）的开源项目，基于开源社区的各种项目进行二次开发的代码完全100%开源，不会像有些“开源项目”一样将基于开源社区项目进行二次开发的结果封装成二进制的库进行“开源”，便于有开发能力的用户自行编译出完全安全放心的apk或者进行二次开发;
- integrate TensorflowLite and other tools for learning/studying computer vision/deep learning/online media QoS conveniently; 内建了完整的编译系统生成二进制库与最终在手机上运行的应用程序，便于ICT专业人员在计算机视觉/深度学习/在线媒体服务质量等技术领域的研究；
- non-profit open source project，just for giving back to the open source community; 非盈利性开源项目，出于回报开源社区与学习研究目的开源；
- LTS 用户可以在此开源项目提交问题，会尽量解答解决用户提出的与此开源项目相关的技术问题；

### TODO（待完成的任务）

- remove illegal advertisement in some online movie，we must keep compliant in opensource software project; [issue could be found here](https://github.com/zhouwg/kantv/issues/13); PR from expert in deep learning field is greatly welcomed;  为了满足合规要求，用tensorflowlite去掉某些在线电影中的非法广告信息，[相关issue在此](https://github.com/zhouwg/kantv/issues/13)，欢迎深度学习领域的技术专家/专业人员提交PR(Pull Request);
    
-  UI refactoring for Android apk,[issue could be found here](https://github.com/zhouwg/kantv/issues/26),PR from domain expert is greatly welcomed; 重构应用程序的用户界面以提升用户使用应用程序的用户体验,[相关issue在此](https://github.com/zhouwg/kantv/issues/26),欢迎精通UI开发的技术专家/专业人员提交PR(Pull Request);
    
- TBD/others ; 其它的待定任务;
