# llama.cpp for QNN

- [Background](#background)
- [News](#news)
- [OS](#os)
- [Hardware](#hardware)
- [Android](#android)
- [Windows over ARM](#windows)
- [Q&A](#qa)
- [TODO](#todo)

## Background

Android maintained its position as the leading mobile operating system worldwide in the fourth quarter of 2023 with <b><a  href="https://www.statista.com/statistics/272698/global-market-share-held-by-mobile-operating-systems-since-2009/">a market share of 70.1 percent </a></b> . Qualcomm is No.1 mobile SoC semiconductor company in our planet currently.


**QNN**(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK is verified to work with the following versions of the ML frameworks:

<ul>
<li>TensorFlow: tf-1.15.0, or tf-2.10.1 </li>
<li>TFLite: tflite-2.3.0 </li>
<li> PyTorch: torch-1.13.1</li>
<li> ONNX: onnx-1.11.0 </li>
</ul>


The Qualcomm® AI Engine Direct architecture is designed to be modular and allows for clean separation in the software for different hardware cores/accelerators such as the CPU, GPU and DSP that are designated as backends. Learn more about Qualcomm® AI Engine Direct backends here.

![Screenshot from 2024-04-14 11-42-14](https://github.com/zhouwg/kantv/assets/6889919/5d8de93a-7b02-4d6b-8b7f-19d2f829dd4d)

The Qualcomm® AI Engine Direct backends for different hardware cores/accelerators are compiled into individual core-specific libraries that come packaged with the SDK.


One of the key highlights of Qualcomm® AI Engine Direct is that it provides a unified API to delegate operations such as graph creation and execution across all hardware accelerator backends. This allows users to treat Qualcomm® AI Engine Direct as a hardware abstraction API and port applications easily to different cores.


The Qualcomm® AI Engine Direct API is designed to support an efficient execution model with capabilities such as graph optimizations to be taken care of internally. At the same time however, it leaves out broader functionality such as model parsing and network partitioning to higher level frameworks.

Qualcomm® AI Engine Direct API and the associated software stack provides all the constructs required by an application to construct, optimize and execute network models on the desired hardware accelerator core. Key constructs are illustrated by the Qualcomm AI Engine Direct Components - High Level View diagram.


![qnn-arch](https://github.com/zhouwg/kantv/assets/6889919/4f4881a6-9a91-4477-aeb2-193591375d75)



### Llama.cpp + QNN

The llama.cpp QNN backend is intented to support **Qualcomm mobile SoC** firstly.


## News

- 2024.5.28
  - re-launch activity of <a href="https://github.com/ggerganov/llama.cpp/pull/6869">PR in upstream ggml community</a>
    
- 2024.4.26
  - refine PR according to coding stye and pricinples of upstream ggml community
  - add command line test using <a href="https://github.com/ggerganov/llama.cpp/blob/master/tests/test-backend-ops.cpp">test-backend-ops.cpp</a>
  - refine PR according comments from reviewer
- 2024.4.24
  - a very beginning <a href="https://github.com/ggerganov/llama.cpp/pull/6869">PR to upstream ggml community</a>
  - data path works fine as expected with whisper.cpp and llama.cpp using QNN backend and verified on both low-end and high-end Android phones based on Qualcomm mobile SoC
  - Support OPs
    - GGML_OP_ADD
    - GGML_OP_MUL
    - GGML_OP_MUL_MAT

- 2024.3.29
  - launch <a href="https://github.com/zhouwg/kantv/issues/121">PoC:add QNN backend for Qualcomm mobile SoC</a>

## OS

| OS                | Status  | Verified                           |
|-------------------|---------|------------------------------------|
| Android           | Support | Android 10, Android 14             |
| Windows over ARM  | TBD     | TBD                                |


## Hardware

### Qualcomm mobile SoC based Android phone

**Verified devices**

| Qualcom mobile SoC                      | Status  | Verified Vendor                       |
|-----------------------------------------|---------|---------------------------------------|
| Qualcomm SM8650-AB Snapdragon 8 Gen 3   | Support | Xiaomi 14                             |
| Qualcomm low-end mobile SoC Series      | Support | Vivo                                  |

### Qualcomm SoC based Windows

TBD

## Android

### 1. Setup Environment

Any **mainstream** Android phone based on Qualcomm's mobile SoC should be supported by llama.cpp + QNN. Qualcomm SM8650-AB Snapdragon 8 Gen 3 based Android phone is preferred.

### 2. Run QNN backend in command line mode on Android phone

- for QNN backend developers, download and install QNN SDK from Qualcomm offcial website

```
  https://qpm.qualcomm.com/#/main/tools/details/qualcomm_ai_engine_direct

  https://developer.qualcomm.com/software/hexagon-dsp-sdk/tools

```

  the default installation path is /opt/qcom/aistack/qnn/2.20.0.240223/


- for programmers, using test-backend-ops.cpp to verify QNN backend on Qualcomm mobile SoC based Android phone

```
  cd core/ggml/llamacpp/tests/ggml-qnn/

  ./build-ggml-qnn.sh

  ./run-ggml-qnn.sh

```

### 3. Run QNN backend in Android APK on Android phone

pls refer to <a href="./README.md">README.md</a>


![504893116](https://github.com/zhouwg/kantv/assets/6889919/51f0b277-eca4-4938-86f5-415dbf5897e7)


## Windows

TBD

## Q&A

TBD

### **GitHub contribution**:
Please add the **[ggml-qnn]** prefix/tag in issues/PRs titles to help the community check/address them without delay.

## TODO

- only support FP32 / FP16 and the input and output tensors must be of the <b>same data type</b>

- lack of [implementation of other GGML-OPs using QNN API](./core/ggml/llamacpp/ggml-qnn.cpp#L3560)

- multithreading not working with QNN GPU&HTP (aka DSP) backend

- QNN's RPC feature(which useful for QNN HTP(aka DSP) backend) not used

- multi QNN backend(CPU/GPU/DSP) simultaneously not support
