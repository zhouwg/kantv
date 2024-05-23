1.[GGML](https://github.com/ggerganov/ggml) is a very <b>compact</b>/highly optimization pure C/C++ machine learning library. GGML is also the solid cornerstone of the amazing [whisper.cpp](https://github.com/ggerganov/whisper.cpp) and the magic [llama.cpp](https://github.com/ggerganov/llama.cpp). Compared to some well-known machine learning frameworks/libraries (e.g. Google TensorFlow, Microsoft ONNX, Meta PyTorch, Baidu PaddlePaddle......), GGML does <b>not</b> have much/complex/complicated/redundant/… <b>encapsulation</b>, so it's <b>very very very useful/helpful/educational</b> for AI beginners. <!--By studying the internals of GGML, you will know what real AI is and what is behind LLM models and how these AI models work in less than 1 month(less than 2 weeks if your IQ is above 130 ------ the baseline IQ to enter [China's top university](https://en.wikipedia.org/wiki/Project_985)(China has a population of 1.4 billion and there are only about 50 top domestic universities), less than 1 week if your IQ is above 150 ------ original author of [FFmpeg](https://ffmpeg.org/), original author of [TensorFlow](https://github.com/tensorflow/tensorflow), original author of [TVM](https://github.com/apache/tvm), original author of [Caffe](https://github.com/BVLC/caffe), original author of [GGML](https://github.com/ggerganov/ggml)). Another thing, tracking code and coding with the GGML API in real task is a good way to study the internals of GGML.--> In general, GGML has following features:

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


2. [NCNN](https://github.com/Tencent/ncnn) is a high-performance neural network inference computing framework <b>optimized for mobile platforms</b>. NCNN is deeply considerate about deployment and uses on mobile phones from the beginning of design.NCNN does <b>not have third-party dependencies</b>. It is cross-platform and runs faster than all known open-source frameworks on mobile phone cpu. Developers can easily deploy deep learning algorithm models to the mobile platform by using efficient ncnn implementation, creating intelligent APPs, and bringing artificial intelligence to your fingertips. NCNN is currently being used in many Tencent applications <b>with 1.3b+ users</b></b>, such as QQ, Qzone, WeChat, Pitu, ... and as well known Tecent is one of the most advanced/powerful IT gaint in China/our planet.  so it's <b>very very very useful/helpful/practical</b> for commerical programmers/developers/applications. In general, NCNN has following feaures:

- Supports convolutional neural networks, supports multiple input and multi-branch structure, can calculate part of the branch
- <b>No third-party library dependencies</b>
- Pure C++ implementation, cross-platform, supports Android, iOS and so on
- ARM NEON assembly level of careful optimization, calculation speed is extremely high
- Sophisticated memory management and data structure design, <b>very low memory footprint</b>
- Supports multi-core parallel computing acceleration, ARM big.LITTLE CPU scheduling optimization
- Supports GPU acceleration via the next-generation low-overhead Vulkan API
- Extensible model design, supports 8bit quantization and half-precision floating point storage, can import caffe/pytorch/mxnet/onnx/darknet/keras/tensorflow(mlir) models
- Support direct <b>memory zero copy</b> reference load network model
- Can be registered with custom layer implementation and extended

  There are some "killer/heavyweight" open-source AI application engines based on NCNN:

- ASR(Automatic Speech Recognition, or Audio2Text)[sherpa-ncnn](https://github.com/k2-fsa/sherpa-ncnn)
- ......


3. GGML and NCNN both support Android & iOS which maintained their position as the leading mobile operating systems worldwide with a market share of 90+ percent. Android phone could be seen as a standard embedding development board/device and it's available everywhere.


4. I'm good at Android software developent(from UI to framework to kernel) but know something about iOS application software development. my resource and capabilitiy is limited.
