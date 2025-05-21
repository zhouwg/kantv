 /*
  * Copyright (c) Project KanTV. 2021-2023
  *
  * Copyright (c) 2024- KanTV Authors
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  */
package kantvai.media.player;

import android.content.Context;

import java.nio.ByteBuffer;


public final class KANTVDRM {
    private static final String TAG = KANTVDRM.class.getName();
    private static KANTVDRM instance = null;
    private static volatile boolean mIsLibLoaded = false;

    private KANTVDRM() {

    }

    public static KANTVDRM getInstance() {
        if (!mIsLibLoaded) {
            KANTVLog.d(TAG, "calling loadLibrary");
            KANTVLibraryLoader.load("kantv-media");
            KANTVLog.d(TAG, "after loadLibrary");
            mIsLibLoaded = true;

            instance = new KANTVDRM();
        } else {
            KANTVLog.d(TAG, "kantv-media.so already loaded");
        }
        return instance;
    }


    public static String getUniqueID() {
        KANTVLog.d(TAG, "enter getUniqueID");
        return KANTVUtils.getWifiMac();
    }

    public static void setEncryptScheme(int encryptScheme) {
        KANTVUtils.setEncryptScheme(encryptScheme);
    }

    public native int ANDROID_JNI_Init(Context ctx, String dataPath);

    public native int ANDROID_JNI_Finalize();

    public native int ANDROID_JNI_DownloadFile(String url, String fileName);

    public native int ANDROID_JNI_SetLocalEMS(String EMSUrl);

    public native int ANDROID_JNI_GetUniqueID();

    public native int ANDROID_JNI_GetDecryptMode();

    public native int ANDROID_JNI_GetEncryptScheme();

    public native void ANDROID_JNI_SetDecryptMode(int decryptMode);

    public native int ANDROID_JNI_GetDrmpluginType();

    public native void ANDROID_JNI_SetDevMode(String assetPath, boolean bDevMode, int dumpMode, int playEngine, int drmScheme, int dumpDuration, int dumpSize, int dumpCounts);

    public native int ANDROID_JNI_HttpPost(String url, String jwt, byte[] requestBody, int requestBodyLength, KANTVDataEntity dataEntity);

    public native int ANDROID_JNI_HttpGet(String url, KANTVDataEntity dataEntity);

    public native int ANDROID_JNI_Base64Decode(byte[] input, int inputLen, KANTVDataEntity dataEntity);

    public native int ANDROID_JNI_Base64Encode(byte[] input, int inputLen, KANTVDataEntity dataEntity);

    public native int ANDROID_JNI_Decrypt(byte[] input, int inputLen, KANTVJNIDecryptBuffer dataEntity, ByteBuffer buffer);

    public native int ANDROID_JNI_GetDiskFreeSize(String path);

    public native int ANDROID_JNI_GetDiskSize(String path);

    public native void ANDROID_JNI_SetRecordConfig(String recordPath, int recordMode, int recordFormat, int recordCodec, int recordDuration, int recordSize);

    public native void ANDROID_JNI_StartBenchmark();

    public native String ANDROID_JNI_GetBenchmarkInfo();

    public native String ANDROID_JNI_Benchmark(int type, int srcW, int srcH, int dstW, int dstH);

    public native void ANDROID_JNI_StopRecord();

    public native void ANDROID_JNI_SetASRConfig(String engineName, String modelPath, int nThreadCounts, int nASRMode);

    public native void ANDROID_JNI_StopASR();

    public native int ANDROID_JNI_GetVideoWidth();

    public native int ANDROID_JNI_GetVideoHeight();

    public native String ANDROID_JNI_GetDeviceClearIDString();

    public native String ANDROID_JNI_GetDeviceIDString();

    public native String ANDROID_JNI_GetSDKVersion();

    public native byte[] ANDROID_JNI_Text2Image(String text);


    public native int ANDROID_DRM_Init(Context ctx, String dataPath, KANTVDRMClientEventListener drmClientEventListener);

    public native int ANDROID_DRM_Finalize();

    public native int ANDROID_DRM_ProcessLicenseResponse(int context, int responseCode, int responseContentLength, byte[] responseContent);

    public native int ANDROID_DRM_GetProvisionRequest(int sessionHandle, KANTVDataEntity provisionRequest);

    public native int ANDROID_DRM_ProcessProvisionResponse(int sessionHandle, int responseCode, int responseContentLength, byte[] responseContent);

    public native int ANDROID_DRM_InitWatermark(int sessionHandle, String registerServerUrl, String watermarkServerUrl, String userID, String siteID);

    public native int ANDROID_DRM_OpenSession(KANTVDRMSessionEventListener drmSessionEventListener);

    public native int ANDROID_DRM_CloseSession(int sessionHandle);

    public native int ANDROID_DRM_SetDrmInfo(int sessionHandle, byte[] drmInfo, int drmInfoLen, String pWatermarkServerUrl);

    public native int ANDROID_DRM_SetOfflineDrmInfo(int sessionHandle, int keyType, int esType, byte[] key, byte[] iv);

    public native int ANDROID_DRM_SetMultiDrmInfo(int sessionHandle, int drmScheme);

    public native int ANDROID_DRM_UpdateDrmInfo(int sessionHandle);

    public native int ANDROID_DRM_Decrypt(int sessionHandle, byte[] input, int inputLen, KANTVVideoDecryptBuffer dataEntity, ByteBuffer buffer);

    public native int ANDROID_DRM_ParseData(int sessionHandle, byte[] input, int inputLen, KANTVCryptoInfo cryptoInfo);

    public native int ANDROID_DRM_IsClearContent();

    public static native int kantv_anti_remove_rename_this_file();

}
