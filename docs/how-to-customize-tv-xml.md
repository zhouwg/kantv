### How to customize tv.xml
- step 1: download tv.xml from phone
```
adb pull /sdcard/tv.xml
```
- step 2: edit tv.xml

```
<?xml version="1.0" encoding="utf-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
     <entry>
        <title> CGTN </title>
        <link href="https://english-livebkws.cgtn.com/live/encgtn.m3u8" poster="cgtn.png" urltype="hls" />
    </entry>

    <entry>
        <title> CNA(Channel News Asia) </title>
        <link href="https://d2e1asnsl7br7b.cloudfront.net/7782e205e72f43aeb4a48ec97f66ebbe/index_5.m3u8" poster="cna.png" protected="no" urltype="hls" />
    </entry>

    <entry>
        <title> CNN</title>
        <link href="https://turnerlive.warnermediacdn.com/hls/live/586495/cnngo/cnn_slate/VIDEO_0_3564000.m3u8" poster="cnn_us.png"  urltype="hls" />
    </entry>

    <entry>
        <title> CBN News National </title>
        <link href="https://bcovlive-a.akamaihd.net/re8d9f611ee4a490a9bb59e52db91414d/us-east-1/734546207001/playlist.m3u8" poster="cbn_news.png" urltype="hls" />
    </entry>


    <!-- begin add customized online-TV program -->

    <entry>
        <title> test1 </title>
        <link href="  https://english-livebkws.cgtn.com/live/encgtn.m3u8" />
    </entry>

    <entry>
        <title> test4test4test4test4 </title>
        <link href="  https://english-livebkws.cgtn.com/live/encgtn.m3u8"  />
    </entry>

    <!-- end add customized online-TV program -->

</feed>
```

- step 3: upload tv.xml to phone
```
adb push /sdcard/tv.xml
```

- step 4: re-launch Android APP

![Image](https://github.com/user-attachments/assets/71a6c0d2-2948-4595-aff1-686410cdb4a1)
