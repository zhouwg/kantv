how to use whisper.cpp and ffmpeg to add subtitle to video

[validation ok on Ubuntu 20.04]


1.background

syntax of FFmpeg subtitle filter

(ref: https://www.jianshu.com/p/c63a57713945, a Chinese doc)
```
Alignment=2
MarginV=5
01.Name
02.Fontname
03.Fontsize
04.PrimaryColour    BBGGRR. color of subtitle
05.SecondaryColour  BBGGRR
06.OutlineColour    BBGGRR
07.BackColour       BBGGRR
08.Bold            -1 -- bold, 0 -- regular
09.Italic          -1 -- italic, 0 -- regular
10.Underline      [-1 or 0]
11.Strikeout      [-1 or 0]
12.ScaleX
13.ScaleY
14.Spacing
15.Angle
16.BorderStyle
17.Outline        0, 1, 2, 3, 4.
18.Shadow         0, 1, 2, 3, 4.
19.Alignment      1, 2, 3
20.MarginL
21.MarginR
22.MarginV
```

2. extract audio content from video

```
./ffmpeg  -i /home/weiguo/kantv-record-MP4-H264-20240308-142616.mp4 -ac 2 -ar 16000 kantv-record-MP4-H264-20240308-142616.wav
```

3. generate srt subtitle file by powerful whisper.cpp

```
./main  -m /home/weiguo/whisper.cpp/models/ggml-base.bin  -f ./kantv-record-MP4-H264-20240308-142616.wav -osrt

cp kantv-record-MP4-H264-20240308-142616.wav.srt 1.srt

```

4. add subtitle to video by poweful FFmpeg
```
./ffmpeg  -i /home/weiguo/kantv-record-MP4-H264-20240308-142616.mp4 -vf "subtitles=1.srt:force_style='Fontname=simhei,Fontsize=25,PrimaryColour=&HFF00,Alignment=2,MarginV=70'" output-srt.mp4
```

5.BTW, the FFmpeg and the main(a command tool in whisper.cpp) here are both generated from scratch source code

follow the steps in toplevel [README.md](https://github.com/zhouwg/kantv/blob/master/README.md) and generate FFmpeg & main from source code:
```
 . build/envsetup.sh
 lunch 2
 ./build-all linux
```

have fun with the great FFmpeg and the great whisper.cpp
