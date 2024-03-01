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
package com.cdeos.kantv.ui.fragment.settings;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.CheckBoxPreference;
import androidx.preference.Preference;


import com.cdeos.kantv.ui.activities.ShellActivity;
import com.cdeos.kantv.utils.Settings;
import com.cdeos.kantv.R;
import com.cdeos.kantv.ui.activities.WebViewActivity;

import java.io.File;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;


public class VoiceSettingFragment extends BaseSettingsFragment {
    private static final String TAG = VoiceSettingFragment.class.getName();
    private static ShellActivity mActivity;
    private Context mContext;
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;
    private Settings mSettings;


    @Override
    public String getTitle() {
        return "语音设置";
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
    }


    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        mActivity = ShellActivity.getInstance();
        mAppContext = mActivity.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mContext = mActivity.getBaseContext();
        mSettings.updateUILang(mActivity);

        addPreferencesFromResource(R.xml.settings_voice);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

        CDELog.j(TAG, "dev mode:" + mSettings.getDevMode());
        if (!mSettings.getDevMode()) {
            findPreference("pref.voiceapi").setEnabled(true);
        }
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }


    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        super.onDisplayPreferenceDialog(preference);
    }

    @Override
    public void onResume() {
        mSharedPreferences.registerOnSharedPreferenceChangeListener(mSharedPreferenceChangeListener);
        super.onResume();
    }

    @Override
    public void onPause() {
        mSharedPreferences.unregisterOnSharedPreferenceChangeListener(mSharedPreferenceChangeListener);
        super.onPause();
    }


    private SharedPreferences.OnSharedPreferenceChangeListener mSharedPreferenceChangeListener = new SharedPreferences.OnSharedPreferenceChangeListener() {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            CDELog.j(TAG, "key : " + key);
            CDELog.j(TAG, "key: " + key);

        }
    };


    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        String key = preference.getKey();
        CDELog.j(TAG, "key : " + key);
        if (preference instanceof CheckBoxPreference) {
            CDELog.d(TAG, "preference : " + preference.getKey() + ", status:" + mSharedPreferences.getBoolean(key, false));
        }

        if (key.contains("downloadASRmodel")) {
            CDELog.j(TAG, "download ASR model");
            WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
            attributes.screenBrightness = 1.0f;
            mActivity.getWindow().setAttributes(attributes);
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            String asrAudioFileName = CDEUtils.getDataPath(mContext) + "audio.wav";
            String asrModelFileName = CDEUtils.getDataPath(mContext) + "deepspeech-0.9.3-models.tflite";
            File asrAudioFile = new File(asrAudioFileName);
            File asrModelFile = new File(asrModelFileName);
            if (
                    (asrAudioFile == null) || (!asrAudioFile.exists())
                            || (asrModelFile == null) || (!asrModelFile.exists())
            ) {
                DownloadModel manager = new DownloadModel(mActivity);
                manager.setTitle("开始下载ASR模型");
                manager.setModeName("ASR");
                manager.setModeName("deepspeech", "audio.wav", "deepspeech-0.9.3-models.tflite");
                manager.showUpdateDialog();
            } else {
                Toast.makeText(mContext, "ASR模型文件已经存在", Toast.LENGTH_SHORT).show();
            }


        }

        if (key.contains("downloadTTSmodel")) {
            CDELog.j(TAG, "download TTS model");
            WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
            attributes.screenBrightness = 1.0f;
            mActivity.getWindow().setAttributes(attributes);
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            String ttsModelFileName1 = CDEUtils.getDataPath(mContext) + "mb_melgan_csmsc_arm.nb";
            String ttsModelFileName2 = CDEUtils.getDataPath(mContext) + "fastspeech2_csmsc_arm.nb";
            File ttsModelFile1 = new File(ttsModelFileName1);
            File ttsModelFile2 = new File(ttsModelFileName2);
            if (
                    (ttsModelFile1 == null) || (!ttsModelFile1.exists())
                            || (ttsModelFile2 == null) || (!ttsModelFile2.exists())
            ) {

                DownloadModel manager = new DownloadModel(mActivity);
                manager.setTitle("开始下载TTS模型");
                manager.setModeName("TTS");
                manager.setModeName("paddlespeech", "mb_melgan_csmsc_arm.nb", "fastspeech2_csmsc_arm.nb");
                manager.showUpdateDialog();
            } else {
                Toast.makeText(mContext, "TTS模型文件已经存在", Toast.LENGTH_SHORT).show();
            }

        }
        return true;
    }
}
