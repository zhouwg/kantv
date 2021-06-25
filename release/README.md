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
    
   

### Screenshot(一些屏幕截图）
![Screenshot_20210625_124431_tv](https://user-images.githubusercontent.com/6889919/123371050-86a8be00-d5b3-11eb-9b33-130ac1c7d00d.jpg)


![Screenshot_20210624_190144_kantv](https://user-images.githubusercontent.com/6889919/123252738-32ec9500-d51f-11eb-9eca-0f0ab49bfafe.jpg)
![Screenshot_20210624_190206_kantv](https://user-images.githubusercontent.com/6889919/123252753-35e78580-d51f-11eb-900d-dfdb5ffffc49.jpg)
![Screenshot_20210624_190243_kantv](https://user-images.githubusercontent.com/6889919/123252766-3849df80-d51f-11eb-91eb-75061259fe0e.jpg)

### Advantage (优点）

- purely client application and purely green application; 纯客户端软件，无任何插件，不会与任何远程服务器交互，不会收集上传用户手机的设备信息以及手机中的文件，只能用于观看收听在线电视/在线广播/在线电影;
- complete（except source code of libdrmclient.so because it contains proprietary IPR） open source project；  接近于完全开源（libdrmclient.so出于知识产权因素无法开源）的开源项目，基于开源社区的各种项目进行二次开发的代码完全100%开源，不会像有些“开源项目”一样将基于开源社区项目进行二次开发的结果封装成二进制的库进行“开源”，便于有开发能力的用户自行编译出完全安全放心的apk或者进行二次开发;
- integrate TensorflowLite and other tools for learning/studying computer vision/deep learning/online media QoS conveniently; 内建了完整的编译系统生成二进制库与最终在手机上运行的应用程序，便于ICT专业人员在计算机视觉/深度学习/在线媒体服务质量等技术领域的研究；
- non-profit open source project，just for giving back to the open source community; 非盈利性开源项目，出于回报开源社区与学习研究目的开源；
- LTS 用户可以在此开源项目提交问题，会尽量解答解决用户提出的与此开源项目相关的技术问题；

### TODO（待完成的任务）

- remove illegal advertisement in some online movie，we must keep compliant in opensource software project; [issue could be found here](https://github.com/zhouwg/kantv/issues/13); PR from expert in deep learning field is greatly welcomed;  为了满足合规要求，用tensorflowlite去掉某些在线电影中的非法广告信息，[相关issue在此](https://github.com/zhouwg/kantv/issues/13)，欢迎深度学习领域的技术专家/专业人员提交PR(Pull Request);
    
-  UI refactoring for Android apk,[issue could be found here](https://github.com/zhouwg/kantv/issues/26),PR from domain expert is greatly welcomed; 重构应用程序的用户界面以提升用户使用应用程序的用户体验,[相关issue在此](https://github.com/zhouwg/kantv/issues/26),欢迎精通UI开发的技术专家/专业人员提交PR(Pull Request);
    
- TBD/others ; 其它的待定任务;
