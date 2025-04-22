 /*
  * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
  *
  * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
package kantvai.media.player;


import android.content.Context;
import android.content.ContextWrapper;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.provider.ContactsContract;
import android.provider.Settings;
import android.content.SharedPreferences;
import android.util.Log;

import com.alibaba.fastjson.JSON;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;

public class KANTVDRMManager {
    private static final String TAG = KANTVDRMManager.class.getName();
    private static KANTVDRM drmInstance = KANTVDRM.getInstance();
    private static int playbackSessionHandle = -1;
    private static KANTVHttpsURLConnection myHttpsURLConnection = new KANTVHttpsURLConnection();
    private static KANTVDRMConfig config = null;

    private static SharedPreferences drmPreferences = null;

    public static void setGlobalPreference(SharedPreferences prf) {
        drmPreferences = prf;
    }

    public static KANTVDRM getKANTVDRMInstance() {
        return drmInstance;
    }


    public static int start(Context context) {
        KANTVLog.j(TAG, "copy local asset files");
        KANTVUtils.copyAssetFile(context, "config.json", KANTVUtils.getDataPath(context) + "config.json");

        String configString = KANTVUtils.readTextFromFile(KANTVUtils.getDataPath(context) + "config.json");

        if (configString != null) {
            config = JSON.parseObject(configString, KANTVDRMConfig.class);
        }

        startDRM(context);

        return 0;
    }

    public static void stop() {
        stopDRM();
    }

    public static int startDRM(Context context) {
        int result = 0;
        result = drmInstance.ANDROID_DRM_Init(context, KANTVUtils.getDataPath(context), new KANTVDRMClientEventListener() {
            @Override
            public int onLicenseRequest(int context, int requestType, String url, int requestBodyLength, byte[] requestBody) {
                    KANTVDataEntity dataEntity = new KANTVDataEntity();
                    KANTVLog.d(TAG, "send request to: " + url);
                    int result = myHttpsURLConnection.doHttpRequest(requestType, url, requestBodyLength, requestBody, dataEntity);

                    if (dataEntity != null && dataEntity.getData() != null) {
                        KANTVLog.d(TAG, "post response: " + new String(dataEntity.getData()));
                    }

                    if (0 == result) {
                        if (dataEntity != null && dataEntity.getData() != null) {
                            KANTVLog.d(TAG, "response body: " + new String(dataEntity.getData()));
                        }
                        result = drmInstance.ANDROID_DRM_ProcessLicenseResponse(context, dataEntity.getResult(), dataEntity.getDataLength(), dataEntity.getData());
                    }

                return result;
            }
        });

        if (result != 0)
            return result;

        doProvision();

        return 0;
    }

    public static int stopDRM() {
        return drmInstance.ANDROID_DRM_Finalize();
    }

    public static int openPlaybackService() {
        KANTVUtils.setOfflineDRM(false);
        playbackSessionHandle = drmInstance.ANDROID_DRM_OpenSession(new KANTVDRMSessionEventListener() {
            @Override
            public int onEvent(int eventType, int eventDataLength, byte[] eventData) {
                return 0;
            }
        });

        if (playbackSessionHandle < 0) {
            KANTVLog.d(TAG, "ANDROID_DRM_OpenSession failed");
            return -1;
        }

        return 0;
    }

    public static int closePlaybackService() {
        KANTVLog.d(TAG, "enter closePlaybackService");
        KANTVUtils.setOfflineDRM(false);
        drmInstance.ANDROID_DRM_CloseSession(playbackSessionHandle);
        KANTVLog.d(TAG, "leave closePlaybackService");

        return 0;
    }


    public static void setOfflineDrmInfo(int encryptionKeyType, int encryptionESType, byte[] key, byte[] iv) {
        drmInstance.ANDROID_DRM_SetOfflineDrmInfo(playbackSessionHandle, encryptionKeyType, encryptionESType, key, iv);
        KANTVUtils.setOfflineDRM(true);
    }

    public static void setMultiDrmInfo(int drmScheme) {
        drmInstance.ANDROID_DRM_SetMultiDrmInfo(playbackSessionHandle, drmScheme);
    }

    public static int base64Decode(byte[] input, int inputLen, KANTVDataEntity dataEntity)
    {
        return drmInstance.ANDROID_JNI_Base64Decode(input, inputLen, dataEntity);
    }

    public static int base64Encode(byte[] input, int inputLen, KANTVDataEntity dataEntity)
    {
        return drmInstance.ANDROID_JNI_Base64Encode(input, inputLen, dataEntity);
    }

    public static int setDrmInfo(byte[] pmtSection) {
        String watermarkContentID = "12345678";
        int result = drmInstance.ANDROID_DRM_SetDrmInfo(playbackSessionHandle, pmtSection, pmtSection.length, watermarkContentID);
        if (result != 0 && result != -2)
            KANTVLog.d(TAG, "ANDROID_DRM_SetDrmInfo return:" + result);
        return result;
    }

    public static int decrypt(byte[] nalUnits, int length, KANTVVideoDecryptBuffer outputData, ByteBuffer buffer) {
        if (drmInstance.ANDROID_DRM_Decrypt(playbackSessionHandle, nalUnits, length, outputData, buffer) == 0x80000003) {
            KANTVLog.d(TAG, "decrypt return 0x80000003");
            drmInstance.ANDROID_DRM_UpdateDrmInfo(playbackSessionHandle);
        }
        return 0;
    }

    public static int parseData(byte[] nalUnits, int length, KANTVCryptoInfo cryptoInfo) {
        if (0 != drmInstance.ANDROID_DRM_ParseData(playbackSessionHandle, nalUnits, length, cryptoInfo)) {
            KANTVLog.j(TAG, "ANDROID_DRM_ParseData failed");
        }
        return 0;
    }


    private static int doProvision() {
        int result = 0;
        int provisionSessionHandle = drmInstance.ANDROID_DRM_OpenSession(new KANTVDRMSessionEventListener() {
            @Override
            public int onEvent(int eventType, int eventDataLength, byte[] eventData) {
                return 0;
            }
        });

        if (provisionSessionHandle < 0) {
            KANTVLog.j(TAG, "open session failed");
            return -1;
        }

        KANTVDataEntity provisionRequest = new KANTVDataEntity();
        result = drmInstance.ANDROID_DRM_GetProvisionRequest(provisionSessionHandle, provisionRequest);
        if (0 == result) {
            if (provisionRequest != null) {
                KANTVLog.j(TAG, "get provision request succeed");
            } else {
                KANTVLog.j(TAG, "did not get a valid provision request");
                return -1;
            }

            KANTVDataEntity provisionResponse = new KANTVDataEntity();
            if ((config != null) && (config.getProvisionUrl() != null)) {
                KANTVLog.j(TAG, "provision url: " + config.getProvisionUrl());
                result = myHttpsURLConnection.doHttpRequest(2, config.getProvisionUrl(), provisionRequest.getDataLength(), provisionRequest.getData(), provisionResponse);
                if (provisionResponse != null && provisionResponse.getData() != null) {
                    KANTVLog.e(TAG, "get provision response succeed");
                } else {
                    KANTVLog.e(TAG, "get provision response failed");
                    result = -1;
                }
            } else {
                result = -1;
            }

            if (0 == result) {
                drmInstance.ANDROID_DRM_ProcessProvisionResponse(provisionSessionHandle,
                        provisionResponse.getResult(),
                        provisionResponse.getDataLength(),
                        provisionResponse.getData());
            }
        }

        KANTVLog.d(TAG, "provision result: " + result);

        return result;
    }


    private static String uid = "";

    public static String getUid() {
        return uid;
    }

    public static void setUid(String inuid) {
        uid = inuid;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
