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
        <title> CNA(Channel News Asia) </title>
        <link href="https://d2e1asnsl7br7b.cloudfront.net/7782e205e72f43aeb4a48ec97f66ebbe/index_5.m3u8" poster="cna.png" urltype="hls" />
    </entry>

    <entry>
        <title> test1 </title>
        <link href="  https://english-livebkws.cgtn.com/live/encgtn.m3u8" poster="test.png" urltype="hls" />
    </entry>

    <entry>
        <title> test2 </title>
        <link href="  https://english-livebkws.cgtn.com/live/encgtn.m3u8"  urltype="hls" />
    </entry>

    <entry>
        <title> test3 </title>
        <link href="  https://english-livebkws.cgtn.com/live/encgtn.m3u8" />
    </entry>

</feed>
```

- step 3: upload tv.xml to phone
```
adb push /sdcard/tv.xml
```
