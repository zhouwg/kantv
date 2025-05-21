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
 import androidx.preference.ListPreference;
 import androidx.preference.Preference;
 import androidx.preference.PreferenceCategory;
 import androidx.preference.SeekBarPreference;


 import com.kantvai.kantvplayer.ui.activities.ShellActivity;
 import com.kantvai.kantvplayer.utils.Settings;
 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.ui.activities.WebViewActivity;

 import java.io.File;

 import kantvai.ai.KANTVAIModel;
 import kantvai.ai.KANTVAIModelMgr;
 import kantvai.ai.KANTVAIUtils;
 import kantvai.ai.ggmljava;
 import kantvai.media.player.KANTVLibraryLoader;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;


 public class LLMSettingFragment extends BaseSettingsFragment {
     private static final String TAG = LLMSettingFragment.class.getName();
     private static ShellActivity mActivity;
     private Context mContext;
     private Context mAppContext;
     private SharedPreferences mSharedPreferences;
     private Settings mSettings;

     private SeekBarPreference mSeekBarTemperature;
     private SeekBarPreference mSeekBarTopP;


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

         mSeekBarTemperature = findPreference("pref.temperature");
         mSeekBarTopP = findPreference("pref.top-p");

         PreferenceCategory category = findPreference("llm-models");
         if (category != null) {
             IjkListPreference preference = findPreference("pref.llmmodel");
             if (preference != null) {
                 //May/08/2025, decoupling UI and data: dynamically initialize UI data of LLM models info from KANTVAIModelMgr.java
                 String[] arrayLLMModelName = KANTVAIModelMgr.getInstance().getAllLLMModelNickName();
                 int nLLMModelCounts = KANTVAIModelMgr.getInstance().getLLMModelCounts();
                 assert(arrayLLMModelName.length == nLLMModelCounts);
                 String[] arrayModelIndex = new String[nLLMModelCounts];
                 for (int i = 0; i < nLLMModelCounts; i++) {
                     arrayModelIndex[i] = String.valueOf(i);
                 }
                 preference.setEntries(arrayLLMModelName);
                 preference.setEntryValues(arrayModelIndex);
                 preference.setEntrySummaries(arrayLLMModelName);
             } else {
                 KANTVLog.g(TAG, "can't find preference");
             }
         } else {
             KANTVLog.g(TAG, "can't find category");
         }

         KANTVLog.g(TAG, "getHexagonEnabled:" + mSettings.getHexagonEnabled());
         if (!mSettings.getHexagonEnabled()) {
             if (findPreference("pref.backend") != null) {
                 findPreference("pref.backend").setEnabled(false);
             }
         }

         int value = mSharedPreferences.getInt("pref.temperature", 40); //default temperature is 0.8, 40.0 / 50.0 = 0.8
         float realvalue = (float) (value / 50.0);
         KANTVLog.g(TAG, "pref.temperature:" + realvalue);
         mSeekBarTemperature.setSummary(String.valueOf(realvalue));
         mSeekBarTemperature.setValue(value);
         KANTVAIUtils.setLLMTemperature(realvalue);

         value = mSharedPreferences.getInt("pref.top-p", 90);//default top-p is 0.9, 90.0 / 100.0 = 0.9
         realvalue = (float) (value / 100.0);
         KANTVLog.g(TAG, "pref.top-p:" + realvalue);
         mSeekBarTopP.setSummary(String.valueOf(realvalue));
         mSeekBarTopP.setValue(value);
         KANTVAIUtils.setLLMTopP(realvalue);
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
                 try {
                     KANTVLog.g(TAG, "LLM backend: " + mSettings.getLLMbackend());
                     KANTVLog.g(TAG, "LLM threadCounts " + mSettings.getLLMThreadCounts());
                     KANTVLog.g(TAG, "LLM model: " + mSettings.getLLMModel());
                     KANTVLog.g(TAG, "LLM model name: " + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel()));
                     String modelPath = KANTVUtils.getSDCardDataPath() + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel());
                     KANTVLog.g(TAG, "modelPath:" + modelPath);
                 } catch (Exception ex) {
                     KANTVLog.g(TAG, "error: " + ex.toString());
                     KANTVUtils.showMsgBox(mActivity, "error: " + ex.toString());
                 }
             }

             if (key.contains("pref.temperature")) {
                 int value = mSharedPreferences.getInt("pref.temperature", 40);
                 float realvalue = (float) (value / 50.0);
                 KANTVLog.g(TAG, "pref.temperature:" + realvalue);
                 mSeekBarTemperature.setSummary(String.valueOf(realvalue));
                 KANTVAIUtils.setLLMTemperature(realvalue);
             }

             if (key.contains("pref.top-p")) {
                 int value = mSharedPreferences.getInt("pref.top-p", 90);
                 float realvalue = (float) (value / 100.0);
                 KANTVLog.g(TAG, "pref.top-p:" + realvalue);
                 mSeekBarTopP.setSummary(String.valueOf(realvalue));
                 KANTVAIUtils.setLLMTopP(realvalue);
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

         if (preference instanceof SeekBarPreference) {
             KANTVLog.g(TAG, "preference : " + preference.getKey() + ", status:" + mSharedPreferences.getInt("pref.temperature", 40));
             return true;
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
                 //FIXME: better approach to check whether the AI model has downloaded successfully
                 if (realModelSize - llmModelFile.length() > KANTVAIUtils.DOWNLOAD_SIZE_CHECK_RANGE) {
                     KANTVLog.g(TAG, "it seems this model is partial downloaded, remove it");
                     llmModelFile.delete();
                 }
                 realModelSize = AIModelMgr.getMMProjmodelSize(userSelectIndex);
                 //FIXME: better approach to check whether the AI model has downloaded successfully
                 if (realModelSize - mmprojModelFile.length() > KANTVAIUtils.DOWNLOAD_SIZE_CHECK_RANGE) {
                     KANTVLog.g(TAG, "it seems this model is partial downloaded, remove it");
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
                 //FIXME: better approach to check whether the AI model has downloaded successfully
                 if (realModelSize - llmModelFile.length() > KANTVAIUtils.DOWNLOAD_SIZE_CHECK_RANGE) {
                     KANTVLog.g(TAG, "it seems this model is partial downloaded, remove it");
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
