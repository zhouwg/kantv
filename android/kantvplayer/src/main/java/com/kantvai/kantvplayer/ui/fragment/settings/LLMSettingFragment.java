 /*
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
 package com.kantvai.kantvplayer.ui.fragment.settings;

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


 import com.kantvai.kantvplayer.ui.activities.ShellActivity;
 import com.kantvai.kantvplayer.utils.Settings;
 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.ui.activities.WebViewActivity;

 import java.io.File;

 import kantvai.ai.KANTVAIModelMgr;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;


 public class LLMSettingFragment extends BaseSettingsFragment {
     private static final String TAG = LLMSettingFragment.class.getName();
     private static ShellActivity mActivity;
     private Context mContext;
     private Context mAppContext;
     private SharedPreferences mSharedPreferences;
     private Settings mSettings;


     @Override
     public String getTitle() {
         return mActivity.getBaseContext().getString(R.string.llm_settings);
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

         addPreferencesFromResource(R.xml.settings_llm);
         mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

         KANTVLog.g(TAG, "getHexagonEnabled:" + mSettings.getHexagonEnabled());
         if (!mSettings.getHexagonEnabled()) {
             if (findPreference("pref.backend") != null) {
                 findPreference("pref.backend").setEnabled(false);
             }
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
             KANTVLog.g(TAG, "key : " + key);
             if (
                 (key.contains("pref.backend"))
                 || (key.contains("pref.llmthreadcounts"))
                 || (key.contains("pref.llmmodel"))
             ) {
                 KANTVLog.g(TAG, "LLM backend: " + mSettings.getLLMbackend());
                 KANTVLog.g(TAG, "LLM threadCounts " + mSettings.getLLMThreadCounts());
                 KANTVLog.g(TAG, "LLM model: " + mSettings.getLLMModel());
                 KANTVLog.g(TAG, "LLM model name: " + KANTVAIModelMgr.getInstance().getName(mSettings.getLLMModel()));
                 String modelPath = KANTVUtils.getSDCardDataPath() + KANTVAIModelMgr.getInstance().getName(mSettings.getLLMModel());
                 KANTVLog.g(TAG, "modelPath:" + modelPath);
                 //KANTVUtils.setASRConfig("whispercpp", modelPath, mSettings.getASRThreadCounts(), mSettings.getASRMode());
             }
         }
     };


     @Override
     public boolean onPreferenceTreeClick(Preference preference) {
         String key = preference.getKey();
         KANTVLog.g(TAG, "key : " + key);
         if (preference instanceof CheckBoxPreference) {
             KANTVLog.d(TAG, "preference : " + preference.getKey() + ", status:" + mSharedPreferences.getBoolean(key, false));
         }

         if (key.contains("pref.downloadLLMmodel")) {
             KANTVLog.j(TAG, "download LLM model");
             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

             String userChooseModelName = KANTVAIModelMgr.getInstance().getName(mSettings.getLLMModel());
             KANTVLog.g(TAG, "LLM model name of user choose:" + userChooseModelName);
             File llmModelFile = new File(KANTVUtils.getSDCardDataPath() + userChooseModelName);
             KANTVLog.g(TAG, "llmModeFile:" + llmModelFile.getAbsolutePath());

             //TODO:add support download other LLM models
             int userSelectIndex = mSettings.getLLMModel();
             KANTVAIModelMgr AIModelMgr = KANTVAIModelMgr.getInstance();
             if ((userSelectIndex == AIModelMgr.getLLMModelIndex("Gemma3-4B"))
               || (userSelectIndex == AIModelMgr.getLLMModelIndex("Gemma3-12B"))
             ) {
                 //do nothing
             } else {
                 KANTVUtils.showMsgBox(mActivity, "currently only support download Gemma3-4B(model size 4.13 GiB, mmproj size 851 MiB)" +
                         " or Gemma3-12B(model size 6.8 GiB, mmproj size 815 MiB");
                 return true;
             }
             KANTVLog.g(TAG, "model url:" +  KANTVAIModelMgr.getInstance().getModelUrl(mSettings.getLLMModel()));
             KANTVLog.g(TAG, "mmproj url:"+  KANTVAIModelMgr.getInstance().getMMProjUrl(mSettings.getLLMModel()));

             if (llmModelFile.exists()) {
                 KANTVLog.g(TAG, "LLM model file already exist: " + KANTVUtils.getDataPath() + userChooseModelName);
                 //Toast.makeText(mContext, "LLM model file already exist: " + KANTVUtils.getDataPath() + userChooseModelFileName, Toast.LENGTH_SHORT).show();
                 KANTVUtils.showMsgBox(mActivity, "LLM model file already exist: " + KANTVUtils.getDataPath() + userChooseModelName);
                 return true;
             }

             if ((llmModelFile == null) || (!llmModelFile.exists())) {
                 DownloadModel manager = new DownloadModel(mActivity);
                 manager.setTitle("begin download LLM model");
                 manager.setModelName("LLM");
                 manager.setLLMModelName("GGML",  userChooseModelName,
                         KANTVAIModelMgr.getInstance().getMMProjName(mSettings.getLLMModel()),
                         KANTVAIModelMgr.getInstance().getModelUrl(mSettings.getLLMModel()),
                         KANTVAIModelMgr.getInstance().getMMProjUrl(mSettings.getLLMModel()));
                 manager.setLLMModelSize(KANTVAIModelMgr.getInstance().getModelSize(mSettings.getLLMModel()),
                         KANTVAIModelMgr.getInstance().getMMProjSize(mSettings.getLLMModel()));
                 manager.showUpdateDialog();
             }
         }

         return true;
     }


     public static native int kantv_anti_remove_rename_this_file();
 }
