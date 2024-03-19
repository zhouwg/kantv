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


 public class ASRSettingFragment extends BaseSettingsFragment {
     private static final String TAG = ASRSettingFragment.class.getName();
     private static ShellActivity mActivity;
     private Context mContext;
     private Context mAppContext;
     private SharedPreferences mSharedPreferences;
     private Settings mSettings;


     @Override
     public String getTitle() {
         return mActivity.getBaseContext().getString(R.string.asr_settings);
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

         addPreferencesFromResource(R.xml.settings_asr);
         mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

         CDELog.j(TAG, "dev mode:" + mSettings.getDevMode());
         //NPU or dedicated hardware AI engine in Android device not supported currently
         if (!mSettings.getDevMode()) {
             if (findPreference("pref.voiceapi") != null) {
                 findPreference("pref.voiceapi").setEnabled(true);
             }
         }


     }

     @Override
     public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
         super.onViewCreated(view, savedInstanceState);

         if (findPreference("pref.voiceapi") != null) {
             findPreference("pref.voiceapi").setOnPreferenceClickListener(preference -> {
                 CDEUtils.showMsgBox(mActivity, "This feature not implemented currently");
                 return false;
             });
         }
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
             if (key.contains("pref.asrmode")) {
                 CDELog.j(TAG, "asrmode: " + mSettings.getASRMode());
                 CDELog.j(TAG, "asrthreadCounts " + mSettings.getASRThreadCounts());
                 CDELog.j(TAG, "GGML mode: " + mSettings.getGGMLMode());
                 CDELog.j(TAG, "GGML mode name: " + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()));
                 String modelPath = CDEUtils.getDataPath() + "ggml-" + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()) + ".bin";
                 CDELog.j(TAG, "modelPath:" + modelPath);
                 CDEUtils.setASRConfig("whispercpp", modelPath, mSettings.getASRThreadCounts() + 1, mSettings.getASRMode());
             }

             if (key.contains("pref.asrthreadcounts")) {
                 CDELog.j(TAG, "asrmode: " + mSettings.getASRMode());
                 CDELog.j(TAG, "asrthreadCounts " + mSettings.getASRThreadCounts() + 1);
                 CDELog.j(TAG, "GGML mode: " + mSettings.getGGMLMode());
                 CDELog.j(TAG, "GGML mode name: " + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()));
                 String modelPath = CDEUtils.getDataPath() + "ggml-" + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()) + ".bin";
                 CDELog.j(TAG, "modelPath:" + modelPath);
                 CDEUtils.setASRConfig("whispercpp", modelPath, mSettings.getASRThreadCounts() + 1, mSettings.getASRMode());
             }


             if (key.contains("pref.ggmlmodel")) {
                 CDELog.j(TAG, "GGML mode: " + mSettings.getGGMLMode());
                 CDELog.j(TAG, "GGML mode name: " + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()));
                 CDELog.j(TAG, "asrmode: " + mSettings.getASRMode());
                 CDELog.j(TAG, "asrthreadCounts " + mSettings.getASRThreadCounts());
                 CDELog.j(TAG, "GGML mode: " + mSettings.getGGMLMode());
                 CDELog.j(TAG, "GGML mode name: " + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()));
                 String modelPath = CDEUtils.getDataPath() + "ggml-" + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()) + ".bin";
                 CDELog.j(TAG, "modelPath:" + modelPath);
                 CDEUtils.setASRConfig("whispercpp", modelPath, mSettings.getASRThreadCounts() + 1, mSettings.getASRMode());
             }
         }
     };


     @Override
     public boolean onPreferenceTreeClick(Preference preference) {
         String key = preference.getKey();
         CDELog.j(TAG, "key : " + key);
         if (preference instanceof CheckBoxPreference) {
             CDELog.d(TAG, "preference : " + preference.getKey() + ", status:" + mSharedPreferences.getBoolean(key, false));
         }


         //focus on GGML's model
         if (key.contains("pref.downloadGGMLmodel")) {
             CDELog.j(TAG, "download GGML model");
             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

             String asrAudioFileName = CDEUtils.getDataPath() + "jfk.wav";
             //String asrModelFileName = CDEUtils.getDataPath(mContext) + "ggml-tiny.en-q8_0.bin";
             String userChooseModelName = CDEUtils.getGGMLModeString(mSettings.getGGMLMode());
             CDELog.j(TAG, "ggml model name of user choose:" + userChooseModelName);
             String userChooseModelFileName = "ggml-" + userChooseModelName + ".bin";
             File asrAudioFile = new File(asrAudioFileName);
             File asrModelFile = new File(CDEUtils.getDataPath() + userChooseModelFileName);
             CDELog.j(TAG, "asrAudioFile:" + asrAudioFile.getAbsolutePath());
             CDELog.j(TAG, "asrModeFile:" + asrModelFile.getAbsolutePath());
             if (
                     (asrAudioFile == null) || (!asrAudioFile.exists())
                             || (asrModelFile == null) || (!asrModelFile.exists())
             ) {
                 DownloadModel manager = new DownloadModel(mActivity);
                 manager.setTitle("begin download GGML model");
                 manager.setModeName("ASR");
                 manager.setModeName("GGML", "jfk.wav", userChooseModelFileName);
                 manager.showUpdateDialog();
             } else {
                 CDELog.j(TAG, "GGML model file already exist: " + CDEUtils.getDataPath() + userChooseModelFileName);
                 Toast.makeText(mContext, "GGML model file already exist: " + CDEUtils.getDataPath() + userChooseModelFileName, Toast.LENGTH_SHORT).show();
                 CDEUtils.showMsgBox(mActivity, "GGML model file already exist: " + CDEUtils.getDataPath() + userChooseModelFileName);
             }


         }

         return true;
     }


     public static native int kantv_anti_remove_rename_this_file();
 }
