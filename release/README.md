### ChangeLog (版本更改记录)
- v0.0.1:

    build time(编译时间)：2021-05-19

    project KanTV was launched base on branch dev-baseon-latest-k0.8.8 in my on-going developing hijkplayer project;
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


- v1.0.0  build time(编译时间)：2023-11-14

- v1.0.1  build time(编译时间)：2023-11-16,00:11

- v1.0.6  build time(编译时间)：2023-11-22,10:00

- v1.0.7  build time(编译时间)：2023-11-24,15:38
- v1.0.10 build time(编译时间)：2023-11-27,12:38 这个版本里有个明显的bug:在＂设置＂界面点击更新节目单后，切回主界面可能会(随机)导致APP crash退出重启

- v1.0.12 build time(编译时间)：2023-11-27,20:41 .修正了上述bug(JNI so里的一个bug，不是每次必现，不确定与华为荣耀手机里的Maple是否有关?)，并增加了其他一个功能(安全方面的改进)

- v1.0.14 build time(编译时间)：2023-11-28,20:06

- v1.0.16 build time(编译时间)：2023-11-30,12:08

- v1.1.0  build time(编译时间)：2023-12-04,13:10由v1.0.x系列demo级水平的UI向商业Android应用软件UI水平迈进

- v1.1.6  build time(编译时间)：2023-12-13,16:15初步集成了语音研究页面，为未来ASR实时字幕开发打下基础

- v1.1.8  build time(编译时间)：2023-12-16,10:28 <strong>初步</strong>完成了录制功能PoC(Proof of Concept，概念验证)开发(也就是打通Java UI-&gt;JNI-&gt;SDK数据通道)，修正了自定义节目单功能中的一个bug，修正了一些其他问题.

- v1.1.16 build time(编译时间):2023-12-30,20:40，自2023年12月16号发布v1.1.8 版本之后，经过近期高强度的开发(每天10+小时)，在解决了一些列高/中/低难度的技术问题后与2天的测试后，v1.1.16版本终于发布了.<strong><font color="red">v1.1.16版本最重要的改进是第一次在产品意义上实现了在线电视录制</font>:对于720×576分辨率的在线电视(CGTN中国国际电视台英文频道)使用H264编码录制生成的文件音视频同步的很好；</strong>支持AV1与WEBP编码的文件播放；解决了FFmpeg软解码时可能导致app退出的问题；因为本版本在产品意义上实现了在线电视录制，所以也解决了v1.1.8版本录制生成的文件无声音且录制生成的文件在\"本地视频\"播放界面无缩略图的问题；包含了向商业软件看齐的一些工程化改进．

- v1.1.18 build time(编译时间):2024-1-5,16:52,
                    <strong>v1.1.18版本最主要的改进是增加了性能测试</strong>，
                    便于对各种宣传资料中的专业术语不太熟悉的Android手机用户，
                    在购买/使用Android手机时从图形处理与视频编码视角直观的评测Android手机性能;
                    同时包含了一些工程化/产品化改进，
                    去掉了程序员思维的一些东西(比如\"语音研究\"等)．
                

- v1.2.0 build time(编译时间):2024-1-14,16:11，v1.2.0相比v1.1.18最大的变化是在性能评测页面增加了手机图形性能测试；同时修正了v1.1.18中native层c/c++代码中的一个非\"防御式编程\"bug导致的一些问题: 
                    性能测试页面在某些手机上来回切换编码格式进行性能测试时，<strong>花屏问题</strong>，以及app<strong>异常退出</strong>问题；
                    １月７日晚上突然发现在某种特殊场景(正在播放央视新闻频道，手机掉到地上，拾起后点击了下屏幕)下app突然莫名其妙异常退出


- v1.2.1 build time(编译时间):2024-2-1，按照Roadmap将自定义节目单从主界面挪到＂个人中心＂页面，主界面类似微信主界面，只保留4个按钮； 
                    在系统设置中增加版本历史；在＂个人中心＂中增加常见问题；在＂个人中心＂增加通过微信赞赏码赞助本软件功能；
                    修正了播放在线电视时的一个bug；<strong>去掉了所有与已经废弃的c/c++写的kantvserver交互的代码</strong>，
                    回到纯客户端工具软件；ASR研究； 修复了一些已知问题

- v1.2.2 build time(编译时间):2024-2-3，首页改为全部是严选外语节目，所有中文节目都移到自定义节目单中；修改自定义节目单功能中的一个bug；
                    修复了一些已知问题

- v1.2.3 build time(编译时间):2024-2-9，修复了一些已知问题

- v1.2.4 build time(编译时间):2024-2-19，去掉了＂个人中心＂中微信赞赏码赞助本软件功能；修复了一些已知问题

- v1.2.5 build time(编译时间):2024-02-27 try to migrate some personal projects to github from Feb 22,2024 after experienced too much in recently months.

- v1.2.6 build time(编译时间):2024-02-29 prepare for migrate kantv to github(add clean-room white-box anti-remove-rename codes in native layer, add ASR in main UI because ASR is important for an open source project(将语音研究又添加到了主界面，因为对于本开源项目尤其是专业AI开发者而言，语音研究比较重要。所以现在主界面又有5个按纽))

- v1.2.7 2024-03-01, 00:30(Beijing Time) prepare for migrate kantv to github(remove SoftwareHistoryActivity.java and CommonQuestionActivity.java because it's highly personalized and not important for an open source project， and then adjust UI accordingly). I hope v1.2.8 could be available in github in next few days(because 8,aka HanZi "发", means lucky in Chinese).

- v1.2.8 2024-03-01, ready to go(open source the latest source code of project KanTV(without native codes currently) in github and this will be the new baseline for personal/community's development activity.

- v1.2.9 2024-03-04,
                 <ul>
                    <li>set English as default UI language(it would be more useful for community)</li>
                    <li> create <a href="https://github.com/cdeos">cde-os</a> org account in github and migrate project KanTV from personal github account to cde-os org account accordingly. so oneday I can assign one friend to continue maintaining it</li>
                    <li> release source code v1.2.9 of KanTV APK before officially start integrating the excellent and amazing whisper.cpp to project KanTV. I have to say that I heard whisper.cpp too late but just try it since March 5,2024</li>
                </ul>

- v1.3.0 2024-03-09,
                 <ul>
                     <li> start integrating whisper.cpp to project kantv. breankdown task in PoC </li>
                     <li> PoC stage-1 is finished and works well as expected </li>
                     <li> PoC stage-2 is finished and works well as expected, it's the first milestone </li>
                 </ul>

- v1.3.1 2024-03-11,
                 <ul>
                     <li> ASR performance improved from 21 secs to 2 secs on Xiaomi 14 by build optimization, it's the second milestone for <a href="https://github.com/cdeos/kantv/issues/64">POC</a></li>
                     <li> add some technical docs to prepared empty directory doc </li>
                     <li> refine regular codes and prepare for coding work of implement real-time English subtitle for online English TV</li>
                 </ul>

- v1.3.2 2024-03-16,
                 <ul>
                     <li> ASR performance improved from 2 secs to 0.8 sec on Xiaomi 14 by special build optimization after study ARM's tech doc </li>
                     <li> coding work of data path: UI <----> JNI <----> whisper.cpp <----> kantv-play <----> kantv-core </li>
                     <li> UI language is now mightbe totally in English for purpose of more easier for open source community </li>
                     <li> audio-only record mode is supported for the first time</li>
                     <li> online TV record and online TV transcription can work at the same time for the first time</li>
                     <li> save audio data to file when transcription was launched for further usage/scenario </li>
                     <li> move some key-point codes to upper layer in customized whisper.cpp for purpose of referenced by AI experts or other programmers</li>
                     <li> implmement TV subtitle for online TV by great whisper.cpp for the first time(NOT realtime)</li>
                 </ul>


### Screenshots(一些屏幕截图）

English UI(英文界面，海外地区默认为英文界面）
-------------------------------------------------------------
![Screenshot_20240301_000503_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/07653f3d-1e7a-4208-a3d8-90b3aecc30b4)
![Screenshot_20240301_000509_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/28d549ba-2fd5-434f-bf7a-b66d82d6dde3)
![990238413](https://github.com/zhouwg/kantv/assets/6889919/44054d57-0149-4d45-8762-46ec80682c66)


![1210108450](https://github.com/cdeos/kantv/assets/6889919/9f82c290-2ed6-444c-98d4-ef840cdd9083)



![Screenshot_20240301_114116_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/10224799-cdf8-46f7-acd0-6df64f0fc674)

![Screenshot_2024_0304_131033](https://github.com/zhouwg/kantv/assets/6889919/6c5bd531-5577-4570-bc87-aa3a87822d6b)

![Screenshot_20240301_000602_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/e3c6b89d-b1cf-42d8-87d0-f4a45074ebba)
![Screenshot_20240301_000609_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/cf3a77ef-1409-4137-8236-487a8de7fe81)

Chinese UI（中文界面，国内默认为中文界面）
-------------------------------------------------------------

![Screenshot_20240301_000319_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/c48ebec5-4942-4e93-9e3a-a8626d553e9e)
![Screenshot_20240301_000324_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/66185ef2-f7e0-4b0f-b01b-ab097167bf8b)
![Screenshot_20240301_000330_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/c0f6a0a0-f150-4126-876a-021465406819)
![1210108450](https://github.com/cdeos/kantv/assets/6889919/9f82c290-2ed6-444c-98d4-ef840cdd9083)
![Screenshot_20240301_114203_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/adbff52c-c0fb-48dd-85c9-4e740c736d7a)
![Screenshot_20240301_114210_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/c48bff82-ed27-47cc-aeee-bc3183707690)
![Screenshot_20240301_000403_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/ebdaa2b6-d177-40c0-9d31-1e583088dd5b)
![Screenshot_20240301_000410_com cdeos kantv](https://github.com/zhouwg/kantv/assets/6889919/c9ec41fc-e3f3-4088-8e6b-5de12145e562)



![371274193](https://github.com/zhouwg/kantv/assets/6889919/b189260b-6011-4c18-a3c0-cd55c2a36b8f)

![1398616229](https://github.com/zhouwg/kantv/assets/6889919/e798b58c-554c-443f-ad76-59f795760ed5)

![1476549041](https://github.com/zhouwg/kantv/assets/6889919/badae664-c864-4d84-87a4-f3d5636c4ff6)


![1109625356](https://github.com/zhouwg/kantv/assets/6889919/829a88d1-a021-4766-a3fc-c65265cc3fdc)


![Screenshot_20240222_111241_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/5762ce61-3e4a-4cbf-988f-65e7910a616d)

![Screenshot_20240222_111248_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/7a8c110d-5f8a-47a8-8018-a5c9d0e1d30a)

![Screenshot_20240222_111326_com cdeos player4tv](https://github.com/zhouwg/kantv/assets/6889919/2794894b-1911-465c-a274-cfff9b116e40)
