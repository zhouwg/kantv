# KanTV

KanTV ("Kan", meaning "watch" in English and "看" in Chinese), an open source project focusing on studying and practicing on-device AI technology in <b>real scenarios</b> (such as performing <b>online-TV playback</b>, <b>realtime transcription</b>, and <b>online-TV recording</b> at the same time) on Android phones:


- Watch online TV and local media using a customized ![FFmpeg 6.1](https://github.com/zhouwg/FFmpeg). This project is derived from the original ![ijkplayer](https://github.com/zhouwg/kantv/tree/kantv-initial) (that project has stopped maintenance since 2021), with many enhancements and new features. Source code of the customized FFmpeg 6.1 can be found in <a href="https://github.com/zhouwg/kantv/tree/master/external/ffmpeg-6.1"> external/ffmpeg </a> according to <a href="https://ffmpeg.org/legal.html">FFmpeg's license</a>. Source code of all FFmpeg 6.1's dependent libraries can be found in <a href="https://github.com/zhouwg/kantv/tree/master/external/ffmpeg-deps"> external/ffmpeg-deps </a>.

- Watch online TV using a customized ![Google ExoPlayer 2.15.1](https://github.com/google/ExoPlayer). Source code of the customized ExoPlayer 2.15.1 can be found in <a href="https://github.com/zhouwg/kantv/tree/master/android/kantvplayer-exo2"> android/kantvplayer-exo2 </a>.

- Record online TV to a local file on the phone.

- 2D graphic performance benchmark.

- AI subtitle (real-time English subtitle for English online-TV (aka OTT TV) via the great & excellent & amazing <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a>).

- Well-maintained <b>turn-key / self-contained</b> workbench for AI experts/researchers who focus on high-value on-device AI R&D activities on Android. Some on-device AI R&D activities (AI algorithm validation, AI model validation, and performance benchmark with ASR/LLM/MTMD (multimodal) on Android) can be done via this project easily.

- Well-maintained <b>turn-key / self-contained</b> workbench for AI beginners to learn on-device AI technology on Android.

- Built-in AI models are supported and run entirely <b>offline (no Internet required)</b>. These supported AI models can be [downloaded in the Android APK directly](./docs/how-to-download-ai-models.md) without manual preparation. APK users can compare the <b>real experience</b> of these AI models on the Android phone. Developers can add other AI models manually in the source code [KANTVAIModelMgr.java](https://github.com/zhouwg/kantv/blob/master/android/kantvplayer-lib/src/main/java/kantvai/ai/KANTVAIModelMgr.java).

  | Model | Type | Capability | Source |
  |-------|------|-----------|--------|
  | ggml-tiny.en-q8_0 | ASR | speech-to-text | [whisper.cpp](https://github.com/ggerganov/whisper.cpp) |
  | [Qwen1.5-1.8B](https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF) | LLM | text-only | Alibaba |
  | [Qwen2.5-3B](https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF) | LLM | text-only | Alibaba |
  | [Gemma3-4B](https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main) | LLM | text + image (MTMD) (known issue: garbled output) | Google |
  | [Gemma-4-E2B](https://huggingface.co/unsloth/gemma-4-E2B-it-GGUF) | LLM | text-only (default) | Google |
  | [SmolVLM2-256M](https://huggingface.co/ggml-org/SmolVLM2-256M-Video-Instruct-GGUF) | LLM | text + image (realtime-video-recognition) (known issue: regression after merging upstream code, video inference broken, only video preview works, WIP) | Huggingface |
  | [Qwen2.5-Omni-3B](https://huggingface.co/ggml-org/Qwen2.5-Omni-3B-GGUF) | LLM | text + audio (MTMD) | Alibaba |

- The [JZ's ggml-hexagon](https://github.com/zhouwg/ggml-hexagon) used in this project is probably the first open-source reference implementation of a specific llama.cpp backend for Qualcomm Hexagon NPU on Android phones. The backend type (Hexagon cDSP vs. generic ggml) is decided at build time, and the DSP-side thread count is automatically clamped based on the target SoC (e.g., 6 threads on Snapdragon 8Elite, 4 threads on Snapdragon 8Gen3). Its PP (prompt processing) and TG (token generation) performance comprehensively surpasses [Qualcomm's official implementation](https://github.com/ggml-org/llama.cpp/tree/master/ggml/src/ggml-hexagon) on Snapdragon 8Elite (aka 8Gen4); benchmark comparisons can be found [here](https://github.com/zhouwg/ggml-hexagon/blob/self-build-jz/docs/backend/jz-ggml-hexagon/ion-mempool-vs-perbuffer-analysis-20260713.md).

### Software architecture of KanTV Android

<img width="803" height="517" alt="kantv-arch" src="https://github.com/user-attachments/assets/59e62514-0c1d-44fb-a4fe-dd8d805e17cb" />


### Building the project

- Clone this repository and build locally, see [how to build](./docs/how-to-build.md)
- Download pre-built Android APK from https://github.com/zhouwg/kantv/releases
- Download pre-built Android APK from Github CI-build: https://github.com/zhouwg/kantv/actions/

### Run Android APK on Android phone
- Android 8.0 (2017.08) --- Android 15 (2024.10) and higher versions with <b>ANY</b> mainstream arm64 mobile SoC.
- An Android smartphone equipped with <b>ANY</b> mainstream <b>high-end</b> mobile SoC is highly <b>recommended</b> for the realtime AI-subtitle feature, otherwise unexpected behavior may occur.
- An Android smartphone equipped with one of the below Qualcomm mobile SoCs is required for verifying/running the ggml-hexagon backend on Android phones:
```
    Snapdragon 8 Gen 2
    Snapdragon 8 Gen 3
    Snapdragon 8 Elite (aka 8 Gen 4)
    Snapdragon 8 Elite Gen 5 (aka 8 Gen 5)
```


### Screenshots

Here is a short video to demonstrate realtime AI subtitle by running the great & excellent & amazing <a href="https://github.com/ggerganov/whisper.cpp"> whisper.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Gen3 mobile SoC - <b>fully offline, on-device</b>.

https://github.com/zhouwg/kantv/assets/6889919/2fabcb24-c00b-4289-a06e-05b98ecd22b8

----

A screenshot to demonstrate multi-modal inference by running the magic <a href="https://github.com/ggerganov/llama.cpp"> llama.cpp </a> on an Android phone equipped with Qualcomm Snapdragon 8Elite mobile SoC  - <b>fully offline, on-device</b>.

![Image](https://github.com/user-attachments/assets/c406951a-383a-4943-a58d-cda401148f9e)

----
A screenshot to demonstrate realtime-video-recognition via [MTMD from llama.cpp](https://github.com/ggml-org/llama.cpp/blob/master/docs/multimodal.md) + a lightweight multimodal model [SmolVLM2-256M from Huggingface](https://huggingface.co/HuggingFaceTB/SmolVLM2-256M-Video-Instruct) on an Android phone equipped with Qualcomm Snapdragon 8Elite mobile SoC  - <b>fully offline, on-device</b>.

![Image](https://github.com/user-attachments/assets/35841e4d-150f-4163-bc58-ada1e9b1a065)


### Docs
- [How to build](./docs/how-to-build.md)
- [How to customize tv.xml for personal needs](./docs/how-to-customize-tv-xml.md)
- [How to download supported AI models in the APK](./docs/how-to-download-ai-models.md)
- [Authors](./AUTHORS)
- [Acknowledgement](./docs/acknowledgement.md)
- [ChangeLog](./release/README.md)
- [ggml-hexagon:history of ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/18)
- [ggml-hexagon:high-level data path of ggml-hexagon](https://github.com/zhouwg/ggml-hexagon/discussions/33)


### Contribution

Reporting issues on Android phones equipped with <b>mainstream</b> mobile SoCs or submitting PRs to this project is greatly appreciated.

I use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking feature requests and issue reports, please see [how to submit an issue in this project](https://github.com/zhouwg/kantv/issues/1).

<!--
comment out this section because some contributors in the upstream project might not want to appear here

### Contributors

[![Contributors](http://contrib.nn.ci/api?repo=zhouwg/kantv)](https://github.com/zhouwg/kantv/graphs/contributors)

-->


### Special Acknowledgement

- Inference engine [GGML](https://github.com/ggml-org/ggml)
- ASR engine [whisper.cpp](https://github.com/ggml-org/whisper.cpp)
- LLM engine [llama.cpp](https://github.com/ggml-org/llama.cpp)
- CV engine [opencv-mobile](https://github.com/nihui/opencv-mobile)
- MTMD (multimodal) engine [MTMD subsystem in llama.cpp](https://github.com/ggml-org/llama.cpp/blob/master/tools/mtmd/README.md)
