// ref&author:https://github.com/nihui/ncnn-android-scrfd
// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

package org.ncnn;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.view.Surface;

public class ncnnjava
{
    /**
     * @param mgr
     * @param netid
     * @param modelid
     * @param backend_type 0: NCNN_BACKEND_CPU, 1: NCNN_BACKEND_GPU
     * @param is_live_inference
     * @return
     */
    public native boolean loadModel(AssetManager mgr, int netid, int modelid, int backend_type, boolean is_live_inference);
    public native boolean openCamera(int facing);
    public native boolean closeCamera();
    public native boolean setOutputWindow(Surface surface);


    /**
     * @param ncnnmodelParam   param file of ncnn model
     * @param ncnnmodelBin     bin   file of ncnn model
     * @param userData         ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / ResNet&SqueezeNet&MNIST: image path / TTS: user input
     * @param bitmap
     * @param nBenchType       1: NCNN_RESNET 2: NCNN_SQUEEZENET 3: NCNN_MNIST
     * @param nThreadCounts    1 - 8
     * @param nBackendType     0: NCNN_BACKEND_CPU  1: NCNN_BACKEND_GPU
     * @param nOpType          type of NCNN OP
     * @return
     */
    public static native String ncnn_bench(String ncnnmodelParam, String ncnnmodelBin, String userData, Bitmap bitmap, int nBenchType, int nThreadCounts, int nBackendType, int nOpType);

    static {
        System.loadLibrary("ncnn-jni");
    }
}
