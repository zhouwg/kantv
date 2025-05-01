# KanTV

KanTV("Kan", aka English "watch") , an open source project focus on study and practise device-AI technology in <b>real scenario</b>(such as online-TV playback and <b>realtime transcription</b> through the amazing <b>whisper.cpp</b> and online-TV recording at the same time) on Android phone:


- Watch online TV and local media by customized ![FFmpeg 6.1](https://github.com/zhouwg/FFmpeg). this project is derived from original ![ijkplayer](https://github.com/zhouwg/kantv/tree/kantv-initial)(because that project has stopped maintenance since 2021), with much enhancements and new features, source code of customized FFmpeg 6.1 could be found in <a href="https://github.com/zhouwg/kantv/tree/master/external/ffmpeg-6.1"> external/ffmpeg </a>according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>. Developers or domain tech experts can set up [a customized playlist](./android/kantvplayer/src/main/assets/tv.xml) and then use this software to watch the content of the customized playlist for R&D activity.

- Record online TV to automatically generate videos(useful for short video creators to generate short video materials but pls respect IPR of original content creator/provider).

- AI subtitle(real-time English subtitle for English online-TV(aka OTT TV) by the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>).

- 2D graphic performance benchmark.


- Well-maintained <b>turn-key / self-contained</b> project for AI researchers(whom mightbe not familiar with <b>regular Android software development</b>)/developers/beginners focus on on-device AI learning / R&D activity, some AI R&D activities (AI algorithm validation / AI model validation / performance benchmark in ASR, LLM) could be done by Android Studio IDE + a powerful Android phone easily. many features and techs here can be used in a real commercial software.

- Built-in [Gemma3-4B](https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main) text-to-text and image-to-text(multimodal) supportive and runs entirely offline(no Internet required)



<!--
generally speaking, this is project for developers and AI experts.
In the all, generally speaking,
- this is <b>project for Linux/Android developers</b>. If you thoroughly understand all the code in this project (native C/C++ and Java)
  - you will find a good job in the CN(age should be smaller then 35 because of well-known facts) with a monthly salary of more than RMB30,000
  - you will find a good job in the US with a monthly salary of more than USD8,000
  - you will find a good job in the EU with a monthly salary of more than EUR5,000

- this is <b>project for AI experts</b>: focus on highly-valuable things rather than routine work or learning Linux/Android programming.
-->


### Highlight

As far as I/We know, probably be <a href="https://github.com/zhouwg/ggml-hexagon/discussions/18"> the first open-source implementation </a> of ggml-hexagon backend in llama.cpp community for Android phone equipped with Qualcomm's high-end Hexagon NPU(such as Snapdragon 8Gen3/Snapdragon 8Elite), [PR can be found at](https://github.com/ggml-org/llama.cpp/pull/12326) upstream llama.cpp community.


### Software architecture of KanTV Android

![kantv-arch-320-240](https://github.com/user-attachments/assets/48e18ace-b667-45f9-8e0f-9faf1427e6bf)

### Building the project

- Clone this repository and build locally, see [how to build](./docs/build.md)
- Download pre-built Android APK from https://github.com/kantv-ai/kantv/releases

### Run Android APK on Android phone
- Android smartphone equipped with one of below Qualcomm mobile SoCs(<b>Qualcomm high-end mobile SoC</b> Snapdragon 8Gen3 and Snapdragon 8Elite are highly recommended) is <b>required</b> for verify/running ggml-hexagon backend on Android phone:
```
    Snapdragon 8 Gen 1
    Snapdragon 8 Gen 1+
    Snapdragon 8 Gen 2
    Snapdragon 8 Gen 3
    Snapdragon 8 Elite
```
- Android smartphone equipped with <b>ANY</b> mainstream <b>high-end</b> mobile SoC is highly <b>recommented</b> for realtime AI-subtitle feature otherwise unexpected behavior would happen

### Screenshots
<hr>
here is a short video to demostrate AI subtitle by running the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen3 mobile SoC - <b>fully offline, on-device</b>.

https://github.com/zhouwg/kantv/assets/6889919/2fabcb24-c00b-4289-a06e-05b98ecd22b8

----

a screenshot to demostrate multi-modal inference by running the magic <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen3 mobile SoC  - <b>fully offline, on-device</b>.


![1429485556](https://github.com/user-attachments/assets/84d9fed1-e250-4212-8eff-104f08110875)

----
a screenshot to demostrate LLM inference by running the magic <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen3 mobile SoC  - <b>fully offline, on-device</b>.

![1351701335](https://github.com/user-attachments/assets/fc30d262-def2-4b77-973c-b71b33080535)


----

a screenshot to demostrate ASR inference by running the excellent <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8 Gen 3 mobile SoC - <b>fully offline, on-device</b>.

![705759462](https://github.com/user-attachments/assets/df1ed1ed-294e-4691-bbd1-b8a6f7ff6f8c)

----
a screenshot to demostrate download LLM model in APK.

![Image](https://github.com/user-attachments/assets/b07f8560-9221-4136-b773-4b0874de294a)

<details>
  <summary>some other screenshots</summary>
  <ol>

![Image](https://github.com/user-attachments/assets/2d95bd5e-bd02-4810-aa70-a81cc0469fcc)

![Image](https://github.com/user-attachments/assets/025a8ff0-7584-4df2-97a5-f4e655a52e0f)
  </ol>
</details>

### Hot topics

- roadmap: https://github.com/zhouwg/kantv/discussions/262
- improve the [ggml-hexagon for Qualcomm Hexagon NPU](https://github.com/zhouwg/ggml-hexagon) in real scenario on Android phone

### Contribution

Be sure to review the [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).

Report issue in various Android-based phone and <b>submit PR to this project is greatly welcomed</b>.

<!--
 **English** is preferred in this project(avoid similar comments in this project:<a href="https://github.com/torvalds/linux/pull/818" target="_blank">https://github.com/torvalds/linux/pull/818</a>). thanks for cooperation and understanding.
-->

### Docs

- [About ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/18)
- [How to build](./docs/build.md)
- [How to troubleshooting](./docs/FAQ.md)
- <b>[How to integrate proprietary/open source codes to project KanTV for personal/proprietary/commercial R&D activity](./docs/how-to-customize.md)</b>
- [Authors](./AUTHORS)
- [Acknowledgement](./docs/acknowledgement.md)
- [ChangeLog](./release/README.md)


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
