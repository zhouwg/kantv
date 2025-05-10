### ChangeLog
- v0.0.1:

    build time：2021-05-19

    project KanTV was launched base on branch dev-baseon-latest-k0.8.8 in my on-going developing hijkplayer project;

- v0.0.2:

    add development mode for troubleshooting various issues;

- v0.0.3:

    add author/maintainer info;

- v0.0.4:

    media category for online TV and online Movie;

    disable progress bar when playback online TV;

    display media name in Media Information dialog;

- v0.0.5:

    refine UI during long time waiting before the first video frame was rendered;

    refine UI during network buffering;

    load EPG infos(aka online TV channels & online Radio programs & online Movie lists) from local XML and then render/layout the UI accordingly;

- v0.0.6:

   add animation during playback online radio program;

   UI bug fix;

   add Chinese language for UI;

   add toggle UI language between Chinese and English;

   add option menu highlight and start activity with SINGLE_TOP mode；

   add "quit" option in toolbar;

- v0.0.7:

   UI modification for purpose of improve user's experience;

   remove online movie lists which contain illegal advertisement;

   update NDK version to r21.4.7075529 LTS for prepare switch to AndroidX, validated on arm64 target only;


- v1.0.0  build time: 2023-11-14

- v1.0.1  build time: 2023-11-16,00:11

- v1.0.6  build time: 2023-11-22,10:00

- v1.0.7  build time: 2023-11-24,15:38

- v1.0.10 build time: 2023-11-27,12:38

- v1.0.12 build time: 2023-11-27,20:41

- v1.0.14 build time: 2023-11-28,20:06

- v1.0.16 build time: 2023-11-30,12:08

- v1.1.0  build time: 2023-12-04,13:10 breaking changes in UI

- v1.1.6  build time: 2023-12-13,16:15 integrate Mozilla's DeepSpeech

- v1.1.8  build time: 2023-12-16,10:28 <strong>initial implementation </strong> of PoC: "online TV recording"

- v1.1.16 build time: 2023-12-30,20:40 the feature of online TV recording should be matured from now on after many efforts recently.

- v1.1.18 build time: 2024-1-5,16:52

- v1.2.0  build time: 2024-1-14,16:11  add feature of "2D graphic benchmark"


- v1.2.1  build time: 2024-2-1 remove server-side related codes and Android APK is a pure green APK from now on

- v1.2.2  build time: 2024-2-3 fix some known issues

- v1.2.3  build time: 2024-2-9 fix some known issues

- v1.2.4  build time: 2024-2-19，remove reward-qr-code;fix some known issues

- v1.2.5  build time: 2024-02-27 try to migrate some personal projects to github from Feb 22,2024 after experienced too much in recently months.

- v1.2.6  build time: 2024-02-29 prepare for migrate kantv to github(add clean-room white-box anti-remove-rename codes in native layer, add ASR in main UI because ASR is important for an open source project

- v1.2.7 2024-03-01, 00:30 prepare for migrate kantv to github(remove SoftwareHistoryActivity.java and CommonQuestionActivity.java because it's highly personalized and not important for an open source project， and then adjust UI accordingly). I hope v1.2.8 could be available in github in next few days(because 8 means lucky in Chinese).

- v1.2.8 2024-03-01, ready to go(open source the latest source code of project KanTV(without native codes currently) in github and this will be the new baseline for personal/community's development activity.

- v1.2.9 2024-03-04,
                 <ul>
                    <li>set English as default UI language</li>
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
                     <li> ASR performance improved from 21 secs to 2 secs on Xiaomi 14 by build optimization</li>
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
                     <li> rename kantv_anti_tamper to kantv_anti_remove_rename_this_file to avoid confusion or misunderstanding</li>
                     <li> <a href="https://github.com/zhouwg/kantv/issues/64">implmement AI English subtitle for English online TV by great whisper.cpp </a>for the first time(NOT real "real-time" and bugfix is required)</li>
                 </ul>

- v1.3.3 2024-03-18,
                 <ul>
                    <li>switch to Project Whispercpp-Android successfully according to roadmap</li>
                    <li>this is the new baseline for new Project KanTV</li>
                 </ul>

- v1.3.4 2024-03-20,
                 <ul>
                    <li>whispercpp configuration in UI and applied in online-TV transcription dynamically</li>
                    <li>pre-load GGML model and initialize ASR-subsystem as early as possible for performance consideration</li>
                    <li>regular cleanup/refine files/codes/README</li>
                 </ul>

- v1.3.5 2024-03-25,
                 <ul>
                    <li>import customized FFmpeg6.1 to project KanTV--step1</li>
                    <li>sync source code of whisper.cpp with upstream </li>
                    <li>add ff_terminal and ff_encode to examples </li>
                    <li>refine doc in readme or source file </li>
                    <li>better performance with better stability after finetune with new method which introduced in https://github.com/ggerganov/whisper.cpp/issues/1951 </li>
                    <li>prepare for step2 of import customized FFmpeg6.1 by new software architecuture</li>
                    <li>move "EPG" to "Person Center" and add "LLM Research" to main UI </li>
                 </ul>

- v1.3.6 2024-03-29,
                 <ul>
                    <li>integrate ggml's magic llama.cpp to kantv</li>
                    <li>unify JNI layer of whisper.cpp and llama.cpp as ggml-jni ------ step1</li>
                    <li>use ggml-jni to validate/verify llama-2-7b,qwen1_5-1_8b, baichuan2-7b, gemma-2b models on Xiaomi 14</li>
                    <li>regular cleanup/refine files/codes/README</li>
                 </ul>

- v1.3.7 2024-05-16, milestone branch
                 <ul>
                    <li><a href="https://github.com/zhouwg/kantv/issues/121">PoC:QNN backend for ggml</a></li>
                    <li>Build:add command-line mode and docker mode</li>
                    <li>ggml-jni:add mnist-ggml example</li>
                 </ul>

- v1.3.8 2024-05-24,
                 <ul>
                    <li>import Tencent ncnn inference framework</li>
                    <li>add ncnn-jni</li>
                    <li>refine codes</li>
                    <li>refine docs</li>
                 </ul>

- v1.3.9 2024-05-26, backup branch before update latest source code of ggml/whispercpp/llamacpp from upstream
                 <ul>
                    <li>ggml-jni:add code skeleton for minicpm-v(a GPT-4V style multimodal LLM MiniCPM-Llama3-V 2.5) and minicp-v inference crash on Xiaomi 14 because latest llama.cpp source code is required for minicpm-v</li>
                 </ul>


- v1.3.10 2024-05-29,
                 <ul>
                    <li><a href="https://github.com/zhouwg/kantv/pull/203">update latest source code of ggml/whispercpp/llamacpp from upstream</a></li>
                    <li><a href="https://github.com/zhouwg/kantv/pull/204">troubleshooting MiniCPM-V inferenenc</a> in Android APK and enable it works fine</li>
                    <li><a href="https://github.com/zhouwg/kantv/pull/206">add code skeleton for yolo-v10 for NCNN</a> </li>
                    <li>fix ui issue when user cancel time-consuming LLM bench task in ui layer</li>
                    <li>update source code of test-backend-ops.cpp from upstream</li>
                    <li>refine codes</li>
                    <li>refine docs</li>
                    <li>prepare for refine ggml qnn backend(MS's onnxruntime might be referenced)</li>
                 </ul>

- v1.3.11 2024-06-04,
                 <ul>
                    <li>refine ggml qnn backend(ggml-qnn.cpp) and update PR in upstream accordingly</li>
                    <li><a href="https://github.com/zhouwg/kantv/pull/216">refine ggml backend subsystem</a>, provide a general approach for mixed inference between Qualcomm's CPU&GPU / CPU&NPU easily</li>
                    <li>provide a dedicated Android command line UT program for verify ggml qnn backend in Android command line mode and update PR in upstream accordingly, it's similar to UT in Android APK which implemented before PR in upstream on 04-24-2024</li>
                    <li>remove dependencies of Android command line UT program in upstream PR</li>
                    <li>add a more proven Android UT case(whisper.cpp using qnn backend) in Android command line mode, it's similar to UT in Android APK which implemented before PR in upstream on 04-24-2024</li>
                    <li>fix a long-term/stupid bug in ggml-qnn.cpp</li>
                    <li>fix bugs(caused by assertion failure in ggml.c) in ggml-jni</li>
                 </ul>

- v1.5.0-pre 2025-01-29, cleanup project to align upstream llama.cpp

- v1.5.0, 2025-02-11,
                    <ul>
                        <li>upgrade Qualcommon QNN SDK to 2.31.0.250130</li>
                        <li><a href="https://github.com/zhouwg/kantv/issues/246">the refined ggml-qnn backend</a> is released and works pretty good with whisper.cpp and llama.cpp on Xiaomi14</li>
                        <li>minimize APP's permission to avoid user's security&privacy concern</li>
                        <li>there are three known memory leak issus in this implmentation</li>
                    </ul>

- v1.6.0, 2025-02-13,
                    <ul>
                        <li>bug-fix of <a href="https://github.com/zhouwg/kantv/issues/246">the refined implmentation of ggml-qnn</a></li>
                        <li>create <a href="https://github.com/kantv-ai">kantv.ai</a> and ready for the second PR to the upstream llama.cpp community</li>
                        <li>update doc</a>
                    </ul>

- v1.6.1, 2025-03-09,
                    <ul>
                        <li>sync code from https://github.com/kantv-ai/ggml-qnn</li>
                    </ul>

- v1.6.2, 2025-03-20,
                    <ul>
                        <li>upgrade Qualcommon QNN SDK to 2.32.0.250228</li>
                        <li>upgrade llama.cpp to upstream</li>
                        <li>sync ggml-qnn implementation from https://github.com/kantv-ai/ggml-qnn</li>
                    </ul>

- v1.6.3-pre, 2025-04-03,
                    <ul>
                        <li>import ggml-hexagon implementation from https://github.com/zhouwg/ggml-hexagon</li>
                        <li>remove QNN-CPU/QNN-GPU/QNN-NPU accordingly</li>
                        <li>remove dependency of QNN runtime libs</li>
                        <li>upgrade llama.cpp to upstream</li>
                        <li>Hexagon NPU couldn't work as expected in APP because of an unknown issue:https://github.com/zhouwg/kantv/issues/269</li>
                    </ul>

- v1.6.3, 2025-04-22,
                    <ul>
                        <li>sync llama.cpp with upstream</li>
                        <li>fix a LLM inference issue on Androd phone</li>
                        <li>refine the entire project</li>
                        <li>Hexagon NPU couldn't work as expected in APP because of an unknown issue:https://github.com/zhouwg/kantv/issues/269</li>
                    </ul>
- v1.6.4, 2025-04-24,
                    <ul>
                        <li>fix issue #269: cDSP cannot works in a standard Android APP</li>
                        <li>sync source code of ggml-hexagon.cpp between project kantv and project ggml-hexagon</li>
                        <li>maintain only one version of ggml-hexagon.cpp and make work flow more clear and easy</li>
                        <li>release v1.6.4</li>
                    </ul>
- v1.6.5, 2025-04-26,
                    <ul>
                       <li>fix a very long-term issue of "2D graphic benchmark does not work properly on Android phone": https://github.com/kantv-ai/kantv/issues/163</li>
                       <li>fix a stability issue of "AI-subtitle can't works" which introduced in https://github.com/kantv-ai/kantv/pull/281
                       <li>release v1.6.5</li>
                    </ul>
- v1.6.6, 2025-04-28,
                    <ul>
                       <li>fix a very long-term stability issue of <a href="https://github.com/kantv-ai/kantv/pull/288">APP would crash during toggle back and forth in different UI frequently when LLM inference is running</a>
                       <li>change main license from Apache2.0 to MIT</li>
                       <li>refine <a href="https://github.com/kantv-ai/kantv/blob/master/docs/how-to-customize.md">how to integrate proprietary / open-source codes to project KanTV for personal/proprietary/commercial R&D activity</a></li>
                       <li>refine toplevel README.md</li>
                       <li>write <a href="https://www.kantvai.com/posts/Convert-safetensors-to-gguf.html">Convert safetensors to GGUF</a></li>
                       <li>release v1.6.6</li>
                    </ul>
- v1.6.7, 2025-05-01,
                    <ul>
                       <li>change project's license from Apache2.0 to MIT</li>
                       <li>try Qwen3-4B, Qwen3-8B, DeepSeek-R1-Distill-Qwen-7B, Gemma3-4B, Gemma3-12B on Android phone</li>
                       <li>[make DeepSeek-R1-Distill-Qwen-1.5B can works fine](https://github.com/kantv-ai/kantv/issues/287) on Android phone</li>
                       <li>[multi-modal inference supportive through Gemma3](https://github.com/kantv-ai/kantv/pull/295) on Android phone</li>
                       <li>refine and simplify UI code</li>
                    </ul>
- v1.6.8, 2025-05-02,
                    <ul>
                       <li>add LLM setting</li>
                       <li>add download LLM models in APK</li>
                       <li>make tv.xml can be edited by APK's user(whom doesn't have tech background)</li>
                       <li>refine and simplify toplevel README.md</li>
                    </ul>
- v1.6.9, 2025-05-10,
                    <ul>
                       <li>integrate stablediffusion.cpp for text-2-image on Android phone</li>
                       <li>make stable-diffusion inference can work(not correct) on Hexagon-cDSP with ggml-hexagon</li>

                       <li>enable another playback engine --- a customized Google Exoplayer2.15.1</li>
                       <li>decoupling UI and data, refine UI and logic of download LLM models in APK</li>
                       <li>refine JNI and try to improve stability of llava inference and stablediffusion inference in some corner cases</li>
                       <li>refine docs</li>
                       <li>refine ggml-hexagon.cpp and sync to the PR in upstream llama.cpp community</li>
                    </ul>
