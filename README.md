# KanTV

KanTV("Kan", aka English "watch") , an open source project focus on study and practise on-device AI technology in <b>real scenario</b>(such as perform <b>online-TV playback</b> and <b>realtime transcription</b> and <b>online-TV record</b> at the same time) on Android phone:


- Watch online TV and local media by customized ![FFmpeg 6.1](https://github.com/kantv-ai/FFmpeg). this project is derived from original ![ijkplayer](https://github.com/kantv-ai/kantv/tree/kantv-initial)(that project has stopped maintenance since 2021), with much enhancements and new features. source code of customized FFmpeg 6.1 could be found in <a href="https://github.com/kantv-ai/kantv/tree/master/external/ffmpeg-6.1"> external/ffmpeg </a>according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>. source code of FFmpeg 6.1's all dependent libraries could be found in <a href="https://github.com/kantv-ai/kantv/tree/master/external/ffmpeg-deps"> external/ffmpeg-deps </a>.

- Watch online TV by customized ![Google Exoplayer 2.15.1](https://github.com/google/ExoPlayer), source code of customized Exoplayer2.15.1 could be found in <a href="https://github.com/kantv-ai/kantv/tree/master/android/kantvplayer-exo2"> android/kantvplayer-exo2 </a>.

- Record online TV to automatically generate videos(useful for short video creators to generate short video materials but pls respect IPR of original content creator/provider).
- 2D graphic performance benchmark.

- AI subtitle(real-time English subtitle for English online-TV(aka OTT TV) via the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>).

- Well-maintained <b>turn-key / self-contained</b> workbench for AI experts/researchers whom focus on highly-value on-device AI R&D activity on Android rather than routine work. some on-device AI R&D activities (AI algorithm validation and AI model validation and performance benchmark with ASR/Text2Image/LLM on Android) could be done via this project easily.

- Well-maintained <b>turn-key / self-contained</b> workbench for AI beginners to learning on-device AI technology on Android.

- Built-in [Gemma3-4B(text-to-text and image-to-text(multimodal))](https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main), [Gemma3-12B](https://huggingface.co/ggml-org/gemma-3-12b-it-GGUF/) , [Qwen1.5-1.8B](https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF), [Qwen2.5-3B](https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF), [Qwen2.5-VL-3B](https://huggingface.co/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF), [Qwen3-4B](https://huggingface.co/Qwen/Qwen3-4B/tree/main), [Qwen3-8B](https://huggingface.co/Qwen/Qwen3-8B) supportive and runs entirely <b>offline(no Internet required)</b>. these LLM models can be downloadded in the Android APK directly without manually preparation. APK's users can compare the <b>real experience</b> of these LLM models on the Android phone.

- Text2Image on Android phone via the amazing [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp).

- Probably be the first [open-source implementation of a specified llama.cpp backend for Qualcomm Hexagon NPU](https://github.com/kantv-ai/kantv/blob/master/core/ggml/llamacpp/ggml/src/ggml-hexagon/ggml-hexagon.cpp) on Android phone.

### Software architecture of KanTV Android

![kantv-arch-320-240](https://github.com/user-attachments/assets/48e18ace-b667-45f9-8e0f-9faf1427e6bf)

### Building the project

- Clone this repository and build locally, see [how to build](./docs/how-to-build.md)
- Download pre-built Android APK from https://github.com/kantv-ai/kantv/releases

### Run Android APK on Android phone
- Android 5.1 --- Android 15 and higher version with <b>ANY</b> mainstream mobile SoC might/should/could be supported.
- Android smartphone equipped with <b>ANY</b> mainstream <b>high-end</b> mobile SoC is highly <b>recommented</b> for realtime AI-subtitle feature otherwise unexpected behavior would happen.
- Android smartphone equipped with one of below Qualcomm mobile SoCs(Qualcomm's state-of-the-art high-end mobile SoC <b>Snapdragon 8Gen3 series and Snapdragon 8Elite series</b> are highly recommended) <b>is required</b> for verify/running ggml-hexagon backend on Android phone:
```
    Snapdragon 8 Gen 1
    Snapdragon 8 Gen 1+
    Snapdragon 8 Gen 2
    Snapdragon 8 Gen 3
    Snapdragon 8 Elite
```


### Screenshots

here is a short video to demostrate realtime AI subtitle by running the great & excellent & amazing<a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Gen3 mobile SoC - <b>fully offline, on-device</b>.

https://github.com/kantv-ai/kantv/assets/6889919/2fabcb24-c00b-4289-a06e-05b98ecd22b8

----

a screenshot to demostrate multi-modal inference by running the magic <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Elite mobile SoC  - <b>fully offline, on-device</b>.

![Image](https://github.com/user-attachments/assets/c406951a-383a-4943-a58d-cda401148f9e)




<details>
  <summary>some other screenshots</summary>
  <ol>

![Image](https://github.com/user-attachments/assets/d9c9bc39-d0d8-4d50-b74d-59152de28d6d)

![Image](https://github.com/user-attachments/assets/025a8ff0-7584-4df2-97a5-f4e655a52e0f)


----

a screenshot to demostrate ASR inference by running the excellent <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Gen3 mobile SoC - <b>fully offline, on-device</b>.

![Image](https://github.com/user-attachments/assets/46856bf2-cc4b-4b0a-9209-d07825fba2e7)


----
a screenshot to demostrate Text-2-Image inference by running the amazaing <a href="https://github.com/leejet/stable-diffusion.cpp"> stable-diffusion.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Elite mobile SoC - <b>fully offline, on-divice</b>.

![713992135](https://github.com/user-attachments/assets/fd6de03a-1f26-45b9-8336-078f928a98b6)

----
a screenshot to demostrate download LLM model in APK.

![1213951738](https://github.com/user-attachments/assets/5a0a965e-1752-475e-a2c1-63e6f60a9009)
![1242080159](https://github.com/user-attachments/assets/32586234-4b2c-4d43-b0ab-498c56de44b3)

  </ol>
</details>




### Docs
- [How to build](./docs/how-to-build.md)
- [How to customize tv.xml for personal needs](./docs/how-to-customize-tv-xml.md)
- [How to troubleshooting issues in APP](./docs/FAQ.md)
- [How to utilize this project for validate/verify a specified LLM model on Android phone](./docs/how-to-use-this-project-for-ai-expert.md)
- [AI-assisted programming via Grok-3](https://github.com/kantv-ai/kantv/blob/4233a85f5e6bf488a5ccf897199ebe4b474e7ae7/core/ggml/llamacpp/ggml/src/ggml-hexagon/ggml-hexagon.cpp#L4164-L4361) and AI-assisted programming via DeepSeek-R1.
- <b>[How to integrate proprietary/open source codes to project KanTV for personal/proprietary/commercial R&D activity](./docs/how-to-customize.md)</b>
- [Authors](./AUTHORS)
- [Acknowledgement](./docs/acknowledgement.md)
- [ChangeLog](./release/README.md)
- [ggml-hexagon:history of ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/18)
- [ggml-hexagon:high-level data path of ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/33)
- [Roadmap](https://github.com/kantv-ai/kantv/discussions/262)

### Contribution

Be sure to review the [opening issues](https://github.com/kantv-ai/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/kantv-ai/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/kantv-ai/kantv/issues/1).

<!--
## Contributors

[![Contributors](http://contrib.nn.ci/api?repo=kantv-ai/kantv)](https://github.com/kantv-ai/kantv/graphs/contributors)
-->


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

  <li>
   Text2Image engine <a href="https://github.com/leejet/stable-diffusion.cpp">stable-diffusion.cpp</a>
  </li>

  </ul>

  </ul>



### License

This project is licensed under [the MIT License](./LICENSE).
