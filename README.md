# KanTV

KanTV("Kan", aka English "watch") , an open source project focus on study and practise device-AI tech in <b>real scenario</b>(such as perform <b>online-TV playback</b> and <b>realtime transcription</b> and <b>online-TV record</b> at the same time) on Android phone:


- Watch online TV and local media by customized ![FFmpeg 6.1](https://github.com/zhouwg/FFmpeg). this project is derived from original ![ijkplayer](https://github.com/zhouwg/kantv/tree/kantv-initial)(that project has stopped maintenance since 2021), with much enhancements and new features, source code of customized FFmpeg 6.1 could be found in <a href="https://github.com/zhouwg/kantv/tree/master/external/ffmpeg-6.1"> external/ffmpeg </a>according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>.

- Record online TV to automatically generate videos(useful for short video creators to generate short video materials but pls respect IPR of original content creator/provider).
- 2D graphic performance benchmark.

- AI subtitle(real-time English subtitle for English online-TV(aka OTT TV) via the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>).

- Well-maintained <b>turn-key / self-contained</b> project for AI experts/researchers(whom mightbe not familiar with <b>regular Android software development</b>) focus on on-device AI R&D activity, some AI R&D activities (AI algorithm validation / AI model validation / performance benchmark in ASR, LLM) could be done by Android Studio IDE + a powerful Android phone easily.

- Built-in [Gemma3-4B](https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main) text-to-text and image-to-text(multimodal) supportive and runs entirely offline(no Internet required)

### Highlight

As far as I/We know, [the implementation of ggml-hexagon in this project](https://github.com/kantv-ai/kantv/blob/master/core/ggml/llamacpp/ggml/src/ggml-hexagon/ggml-hexagon.cpp) probably be the first open-source implementation of ggml-hexagon backend in llama.cpp community for Android phone equipped with Qualcomm's high-end Hexagon NPU(such as Snapdragon 8Gen3/Snapdragon 8Elite), [PR can be found at](https://github.com/ggml-org/llama.cpp/pull/12326) upstream llama.cpp community, details could be found at https://github.com/zhouwg/ggml-hexagon/discussions/18.


### Software architecture of KanTV Android

![kantv-arch-320-240](https://github.com/user-attachments/assets/48e18ace-b667-45f9-8e0f-9faf1427e6bf)

### Building the project

- Clone this repository and build locally, see [how to build](./docs/build.md)
- Download pre-built Android APK from https://github.com/kantv-ai/kantv/releases

### Run Android APK on Android phone
- Android 5.1 --- Android 15 and higher version with <b>ANY</b> mainstream mobile SoC might/should/could be supported.
- Android smartphone equipped with <b>ANY</b> mainstream <b>high-end</b> mobile SoC is highly <b>recommented</b> for realtime AI-subtitle feature otherwise unexpected behavior would happen.
- Android smartphone equipped with one of below Qualcomm mobile SoCs(Qualcomm high-end mobile SoC <b>Snapdragon 8Gen3 and Snapdragon 8Elite</b> are highly recommended) <b>is required</b> for verify/running ggml-hexagon backend on Android phone:
```
    Snapdragon 8 Gen 1
    Snapdragon 8 Gen 1+
    Snapdragon 8 Gen 2
    Snapdragon 8 Gen 3
    Snapdragon 8 Elite
```


### Screenshots
<hr>
here is a short video to demostrate AI subtitle by running the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen3 mobile SoC - <b>fully offline, on-device</b>.

https://github.com/zhouwg/kantv/assets/6889919/2fabcb24-c00b-4289-a06e-05b98ecd22b8

----

a screenshot to demostrate multi-modal inference by running the magic <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen3 mobile SoC  - <b>fully offline, on-device</b>.


![132746253](https://github.com/user-attachments/assets/ce0306c4-5e59-4504-8ebe-b17c178e688b)

----

a screenshot to demostrate ASR inference by running the excellent <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen 3 mobile SoC - <b>fully offline, on-device</b>.

![226086526](https://github.com/user-attachments/assets/1e5d54f7-a2c2-4365-b46f-4c8486156bd4)

----
a screenshot to demostrate download LLM model in APK.

![1213951738](https://github.com/user-attachments/assets/5a0a965e-1752-475e-a2c1-63e6f60a9009)
![1242080159](https://github.com/user-attachments/assets/32586234-4b2c-4d43-b0ab-498c56de44b3)

<details>
  <summary>some other screenshots</summary>
  <ol>

![Image](https://github.com/user-attachments/assets/2d95bd5e-bd02-4810-aa70-a81cc0469fcc)

![Image](https://github.com/user-attachments/assets/025a8ff0-7584-4df2-97a5-f4e655a52e0f)
  </ol>
</details>

### Hot topics

- roadmap: https://github.com/zhouwg/kantv/discussions/262

### Contribution

Be sure to review the [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).

<b>Report issue</b> in various Android-based phone and submit PR to this project is greatly welcomed.
<!--
English is preferred in this project, thanks for cooperation and understanding.
-->

### Docs

- [About ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/18)
- [How to build](./docs/build.md)
- [How to troubleshooting](./docs/FAQ.md)
- <b>[How to integrate proprietary/open source codes to project KanTV for personal/proprietary/commercial R&D activity](./docs/how-to-customize.md)</b>
- [Authors](./AUTHORS)
- [Acknowledgement](./docs/acknowledgement.md)
- [ChangeLog](./release/README.md)

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

### Special Acknowledgement

 <ul>AI inference framework

   <ul>
  <li>
   <a href="https://github.com/ggml-org/ggml">GGML</a>
   </li>


  </ul>

  </ul>

 <ul>AI application engine

  <ul>
  <li>
   ASR engine <a href="https://github.com/ggml-org/whisper.cpp">whisper.cpp</a>
  </li>

   <li>
  LLM engine <a href="https://github.com/ggml-org/llama.cpp">llama.cpp</a>
  </li>

  </ul>

  </ul>



### License

This project is licensed under [the MIT License](./LICENSE).
