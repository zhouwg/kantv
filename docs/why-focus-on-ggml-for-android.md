1.[GGML](https://github.com/ggerganov/ggml) is a very <b>compact</b>/highly optimization pure C/C++ machine learning library. GGML is also the solid cornerstone of the amazing [whisper.cpp](https://github.com/ggerganov/whisper.cpp) and the magic [llama.cpp](https://github.com/ggerganov/llama.cpp). Compared to some well-known machine learning frameworks/libraries (e.g. Google TensorFlow, Microsoft ONNX, Meta PyTorch, Baidu PaddlePaddle......), GGML does <b>not</b> have much/complex/complicated/redundant/… <b>encapsulation</b>, so it's <b>very very very useful/helpful/educational</b> for AI beginners. In general, GGML has following features:

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
- [all in one source file](https://github.com/ggerganov/ggml/blob/master/src/ggml.c) and similar to [imgui](https://github.com/ocornut/imgui/blob/master/imgui.cpp)(this is just personal opinion and I really like it, this is a NEW coding style and the requirements for programmer is relatively high and very helpful for experienced programmer, this coding style may not be acceptable in large commercial IT companies because it violates some <b>principles of modern software engineering</b>)


 There are four "killer/heavyweight" open-source AI application engines based on GGML:

- ASR(Automatic Speech Recognition, or Audio2Text)[whisper.cpp](https://github.com/ggerganov/whisper.cpp)
- LLM [ llama.cpp](https://github.com/ggerganov/llama.cpp)
- TTI(Text2Image) [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp)
- TTS(Text2Speech) [ bark.cpp](https://github.com/PABannier/bark.cpp)


2. <a href="https://github.com/zhouwg/kantv/issues/64">A successful PoC </a>illustrates/proves that ggml can be used in real(or commerical) AI application on Android device or other edge device.


3. AI model trainning and finetuning will be done by AI experts/algorithm engineers. this project is a workbench for learing&practising AI tech in real scenario on Android device, focus on AI inference and AI application on Android.


4. GGML based command line application(which provided in upstream ggml/whispercpp/llamacpp) works fine/well as expected on Linux/Windows/Mac, but it might be not work properly as expected on Android because Android phone could be seen as a special/standard embedding development board. A dedicated ggml/whispercpp/llamacpp derived project focus on Android might be useful for community.


5. Android maintained its position as the leading mobile operating system worldwide in the fourth quarter of 2023 with a market share of 70.1 percent.
