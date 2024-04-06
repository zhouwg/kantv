The [GGML(Georgi Gerganov Machine Learning)](https://github.com/ggerganov/ggml) is a very compact(**without much encapsulation**)</b>/highly optimization machine learning library for machine learning beginners. 


GGML is also the solid cornerstone of the amazing [whisper.cpp](https://github.com/ggerganov/whisper.cpp) and the magic [llama.cpp](https://github.com/ggerganov/llama.cpp).  GGML has following features:

- Written in C
- 16-bit float support
- Integer quantization support (4-bit, 5-bit, 8-bit, etc.)
- Automatic differentiation
- ADAM and L-BFGS optimizers
- Optimized for Apple Silicon
- On x86 architectures utilizes AVX / AVX2 intrinsics
- On ppc64 architectures utilizes VSX intrinsics
- <b>No third-party dependencies</b>
- <b>Zero memory allocations during runtime</b>

Some open-source C/C++ AI projects/examples based on GGML:

- [X] Example of GPT-2 inference [examples/gpt-2](https://github.com/ggerganov/ggml/tree/master/examples/gpt-2)
- [X] Example of GPT-J inference [examples/gpt-j](https://github.com/ggerganov/ggml/tree/master/examples/gpt-j)
- [X] Example of Whisper inference [examples/whisper](https://github.com/ggerganov/ggml/tree/master/examples/whisper)
- [X] Example of LLaMA inference [ggerganov/llama.cpp](https://github.com/ggerganov/llama.cpp)
- [X] Example of LLaMA training [ggerganov/llama.cpp/examples/baby-llama](https://github.com/ggerganov/llama.cpp/tree/master/examples/baby-llama)
- [X] Example of Falcon inference [cmp-nct/ggllm.cpp](https://github.com/cmp-nct/ggllm.cpp)
- [X] Example of BLOOM inference [NouamaneTazi/bloomz.cpp](https://github.com/NouamaneTazi/bloomz.cpp)
- [X] Example of RWKV inference [saharNooby/rwkv.cpp](https://github.com/saharNooby/rwkv.cpp)
- [x] Example of SAM inference [examples/sam](https://github.com/ggerganov/ggml/tree/master/examples/sam)
- [X] Example of BERT inference [skeskinen/bert.cpp](https://github.com/skeskinen/bert.cpp)
- [X] Example of BioGPT inference [PABannier/biogpt.cpp](https://github.com/PABannier/biogpt.cpp)
- [X] Example of Encodec inference [PABannier/encodec.cpp](https://github.com/PABannier/encodec.cpp)
- [X] Example of CLIP inference [monatis/clip.cpp](https://github.com/monatis/clip.cpp)
- [X] Example of MiniGPT4 inference [Maknee/minigpt4.cpp](https://github.com/Maknee/minigpt4.cpp)
- [X] Example of ChatGLM inference [li-plus/chatglm.cpp](https://github.com/li-plus/chatglm.cpp)
- [X] Example of <b>Stable Diffusion inference</b> [leejet/stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp)
- [X] Example of Qwen inference [QwenLM/qwen.cpp](https://github.com/QwenLM/qwen.cpp)
- [X] Example of YOLO inference [examples/yolo](https://github.com/ggerganov/ggml/tree/master/examples/yolo)
- [X] Example of ViT inference [staghado/vit.cpp](https://github.com/staghado/vit.cpp)
- [X] Example of multiple LLMs inference [foldl/chatllm.cpp](https://github.com/foldl/chatllm.cpp)


This project is a (<b>personal</b>) workbench for study&practise AI tech in real scenario on Android device, powered by the great GGML(ggml library,whisper.cpp,llama.cpp,stablediffusion.cpp...other C/C++ AI project based on GGML) and FFmpeg


Project maintainer: zhouwg (zhouwg2000@gmail.com,an experienced Android system software programmer,knows nothing about real/hard-core AI technology, but is very interested in the practice of AI technology on mobile/edge device)



```
.
├── ggml
│   ├── build-android-jni-lib.sh (by me)
│   ├── CMakeLists.txt (by me)
│   ├── jni (by me, some source codes was borrowed from examples in ggml/whisper.cpp/llama.cpp/stablediffusion.cpp, see comments in codes or license file accordingly)
│   ├── llamacpp(original author is Georgi Gerganov)
│   ├── qnnsample(customized Qualcomm's QNN sample for PoC:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121)
│   ├── stablediffusioncpp(original author is leejet(https://github.com/leejet/stable-diffusion.cpp))
│   └── whispercpp(original author is Georgi Gerganov)
└── README.md

```
