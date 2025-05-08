 /*
  * Copyright (c) 2024- KanTV Authors
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

 import kantvai.ai.KANTVAIModel;
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

         //TODO: dynamically load LLM models info from KANTVAIModelMgr.java to avoid hardcode LLM models in xml file
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
                 KANTVLog.g(TAG, "LLM model name: " + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel()));
                 String modelPath = KANTVUtils.getSDCardDataPath() + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel());
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

             String userChooseModelName = KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel());
             KANTVLog.g(TAG, "LLM model name of user choose:" + userChooseModelName);
             File llmModelFile = new File(KANTVUtils.getSDCardDataPath() + userChooseModelName);
             KANTVLog.g(TAG, "llmModeFile:" + llmModelFile.getAbsolutePath());


             String mmprojModelName = KANTVAIModelMgr.getInstance().getMMProjmodelName(mSettings.getLLMModel());
             KANTVLog.g(TAG, "mmproj name:" + mmprojModelName);
             File mmprojModelFile = null;
             if (mmprojModelName != null) {
                 mmprojModelFile = new File(KANTVUtils.getSDCardDataPath() + mmprojModelName);
                 KANTVLog.g(TAG, "mmproj name:" + mmprojModelFile.getAbsolutePath());
             }

             int userSelectIndex = mSettings.getLLMModel();
             KANTVLog.g(TAG, "userSelectIndex = " + userSelectIndex);
             KANTVAIModelMgr AIModelMgr = KANTVAIModelMgr.getInstance();
             if (!AIModelMgr.isDownloadAble(userSelectIndex)) {
                 KANTVUtils.showMsgBox(mActivity, "currently don't support download LLM model:"
                         + AIModelMgr.getModelName(userSelectIndex) + " from " + AIModelMgr.getModelUrl(userSelectIndex));
                 return true;
             }
             KANTVLog.g(TAG, "model url:" + KANTVAIModelMgr.getInstance().getModelUrl(mSettings.getLLMModel()));
             KANTVLog.g(TAG, "mmproj model url:" + KANTVAIModelMgr.getInstance().getMMProjmodelUrl(mSettings.getLLMModel()));

             if (mmprojModelName != null) {
                 long realModelSize = AIModelMgr.getModelSize(userSelectIndex);
                 if (realModelSize - llmModelFile.length() > (700 * 1024 * 1024)) {
                     KANTVLog.g(TAG, "not completed model, remove it");
                     llmModelFile.delete();
                 }
                 realModelSize = AIModelMgr.getMMProjmodelSize(userSelectIndex);
                 if (realModelSize - mmprojModelFile.length() > (700 * 1024 * 1024)) {
                     KANTVLog.g(TAG, "not completed model, remove it");
                     mmprojModelFile.delete();
                 }
                 if (llmModelFile.exists() && mmprojModelFile.exists()) {
                     KANTVLog.g(TAG, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelName);
                     //Toast.makeText(mContext, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelFileName, Toast.LENGTH_SHORT).show();
                     KANTVUtils.showMsgBox(mActivity, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelName);
                     return true;
                 }
             } else {
                 long realModelSize = AIModelMgr.getModelSize(userSelectIndex);
                 if (realModelSize - llmModelFile.length() > (700 * 1024 * 1024)) {
                     KANTVLog.g(TAG, "not completed model, remove it");
                     llmModelFile.delete();
                 }

                 if (llmModelFile.exists()) {
                     //Toast.makeText(mContext, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelFileName, Toast.LENGTH_SHORT).show();
                     KANTVUtils.showMsgBox(mActivity, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelName);
                     return true;
                 }
             }

             DownloadModel manager = new DownloadModel(mActivity);
             manager.setTitle("begin download LLM model");
             manager.setModelName("LLM");
             manager.setLLMModelName("GGML", userChooseModelName,
                     KANTVAIModelMgr.getInstance().getMMProjmodelName(mSettings.getLLMModel()),
                     KANTVAIModelMgr.getInstance().getModelUrl(mSettings.getLLMModel()),
                     KANTVAIModelMgr.getInstance().getMMProjmodelUrl(mSettings.getLLMModel()));
             manager.setLLMModelSize(KANTVAIModelMgr.getInstance().getModelSize(mSettings.getLLMModel()),
                     KANTVAIModelMgr.getInstance().getMMProjmodelSize(mSettings.getLLMModel()));
             manager.showUpdateDialog();
         }

         return true;
     }


     public static native int kantv_anti_remove_rename_this_file();
 }
