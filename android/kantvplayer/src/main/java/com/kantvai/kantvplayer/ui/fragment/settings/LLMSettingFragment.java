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
import java.util.Locale;

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

    private LLMSettingHeaderPreference mHeaderPreference;


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
                int nLLMModelCounts = KANTVAIModelMgr.getInstance().getLLMModelCounts();
                int nonLLMCount = KANTVAIModelMgr.getInstance().getNonLLMModelCounts();
                String[] arrayModelIndex = new String[nLLMModelCounts];
                String[] arrayLLMModelName = new String[nLLMModelCounts];
                for (int i = 0; i < nLLMModelCounts; i++) {
                    arrayModelIndex[i] = String.valueOf(i);
                    KANTVAIModel m = KANTVAIModelMgr.getInstance().getKANTVAIModelFromIndex(i + nonLLMCount);
                    // Append the modality tag so the user can tell which model
                    // supports image / audio input straight from the dropdown,
                    // without having to open the docs. The on-disk filename
                    // (m.getName()) is unaffected and still shown in the
                    // header line.
                    String nick = (m != null && m.getNickname() != null) ? m.getNickname() : ("model-" + i);
                    String tag  = (m != null) ? " " + m.getModalityTag() : "";
                    arrayLLMModelName[i] = nick + tag;
                }
                preference.setEntries(arrayLLMModelName);
                preference.setEntryValues(arrayModelIndex);
                preference.setEntrySummaries(arrayLLMModelName);

                // Force the summary to be set: read the stored value (or the
                // xml default "3" for new installs), validate it against the
                // dynamic list, fall back to a valid index if it is out of
                // range. Without this explicit setValue, IjkListPreference.
                // syncSummary() bails out when getEntryIndex() returns -1
                // (which happens for the legacy "6" stored value), and the
                // dropdown ends up showing "Choose LLM model" with no
                // model-name hint underneath.
                int storedLlmIndex = -1;
                try {
                    storedLlmIndex = mSettings.getLLMModel();
                } catch (Exception ex) {
                    KANTVLog.g(TAG, "getLLMModel failed: " + ex.toString());
                }
                int effectiveLlmIndex;
                if (storedLlmIndex >= 0 && storedLlmIndex < nLLMModelCounts) {
                    effectiveLlmIndex = storedLlmIndex;
                } else {
                    // Stored value is out of range - heal it. Gemma-4-E2B
                    // (LLM index 3) is the KANTVAIModelMgr-side default; clamp
                    // to a safe value if the list is smaller than expected.
                    effectiveLlmIndex = Math.min(3, nLLMModelCounts - 1);
                    if (effectiveLlmIndex < 0) {
                        effectiveLlmIndex = 0;
                    }
                    KANTVLog.g(TAG, "stored LLM index " + storedLlmIndex
                            + " is out of range (count=" + nLLMModelCounts
                            + "), healing to " + effectiveLlmIndex);
                }
                preference.setValue(String.valueOf(effectiveLlmIndex));
                // setValue() above persists via DialogPreference.persistString(),
                // so subsequent reads via Settings.getLLMModel() return the
                // healed value. This also keeps SharedPreferences in sync with
                // whatever the dropdown currently shows.
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

        // Populate the custom header (logo + device info + currently selected / loaded LLM model).
        populateHeader();
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
        // Re-populate the header when the user navigates back, so the loaded-model
        // status is always up-to-date.
        populateHeader();
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
             if (key.contains("pref.backend")) {
                 try {
                     KANTVLog.g(TAG, "LLM backend: " + mSettings.getLLMbackend());
                 } catch (Exception ex) {
                     KANTVLog.g(TAG, "error: " + ex.toString());
                     KANTVUtils.showMsgBox(mActivity, "error: " + ex.toString());
                 }
             }

             if (key.contains("pref.llmmodel")) {
                try {
                    KANTVLog.g(TAG, "LLM model: " + mSettings.getLLMModel());
                    KANTVLog.g(TAG, "LLM model name: " + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel()));
                    String modelPath = KANTVUtils.getSDCardDataPath() + KANTVAIModelMgr.getInstance().getModelName(mSettings.getLLMModel());
                    KANTVLog.g(TAG, "modelPath:" + modelPath);
                } catch (Exception ex) {
                    KANTVLog.g(TAG, "error: " + ex.toString());
                    KANTVUtils.showMsgBox(mActivity, "error: " + ex.toString());
                }
                // Refresh the header so the selected-model line + resident-status line
                // reflect the new choice.
                populateHeader();
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

             if (key.contains("pref.hfendpoint")) {
                 int value = mSettings.getHFEndpoint();
                 KANTVLog.g(TAG, "hfendpoint:" + value);
                 KANTVAIUtils.setHFEndpoint(value);
             }
         }
     };


     /**
     * Populate the custom header at the top of the preference list with:
     *   - device info (phone, Android version, arch, backend, memory)
     *   - currently selected LLM model (nickname, file name, file size on disk)
     *   - resident status (Loaded in memory / Not loaded)
     *
     * The native query for the actually loaded model returns empty for now
     * (the LLM/MTMD benchmark path is one-shot per call, not yet resident);
     * once the native singleton refactor lands, this method will start
     * showing "Loaded" once a model is hot in memory.
     *
     * Defensive: every external call (KANTVAIModelMgr, ggmljava, JNI) is wrapped in
     * try-catch so a stale or missing model in KANTVAIModelMgr never crashes the
     * whole fragment - the header just shows an "<unavailable>" line instead.
     */
    private void populateHeader() {
        try {
            if (mHeaderPreference == null) {
                mHeaderPreference = (LLMSettingHeaderPreference) findPreference("pref.llm.header");
            }
            if (mHeaderPreference == null) {
                KANTVLog.g(TAG, "header preference not found");
                return;
            }

            // 1) device info
            try {
                mHeaderPreference.setDeviceInfo(KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_LLM));
            } catch (Throwable t) {
                mHeaderPreference.setDeviceInfo("Device   : <unavailable: " + t.getClass().getSimpleName() + ">");
            }

            // 2) currently selected LLM model
            String modelInfo = "LLM model : <not selected>";
            try {
                int llmIndex = mSettings.getLLMModel();
                int llmCount = KANTVAIModelMgr.getInstance().getLLMModelCounts();
                if (llmIndex >= 0 && llmIndex < llmCount) {
                    KANTVAIModel m = KANTVAIModelMgr.getInstance().getKANTVAIModelFromIndex(llmIndex + KANTVAIModelMgr.getInstance().getNonLLMModelCounts());
                    if (m == null) {
                        modelInfo = "LLM model : <index " + llmIndex + " out of range, total " + llmCount + ">";
                    } else {
                        String modelName = m.getName();
                        File modelFile = new File(KANTVUtils.getSDCardDataPath() + modelName);
                        String sizeStr = "<missing>";
                        if (modelFile.exists()) {
                            long bytes = modelFile.length();
                            if (bytes > (1L << 30)) {
                                sizeStr = String.format(Locale.US, "%.2f GB", bytes / (double) (1L << 30));
                            } else if (bytes > (1L << 20)) {
                                sizeStr = String.format(Locale.US, "%.1f MB", bytes / (double) (1L << 20));
                            } else if (bytes > (1L << 10)) {
                                sizeStr = String.format(Locale.US, "%.1f KB", bytes / (double) (1L << 10));
                            } else {
                                sizeStr = bytes + " B";
                            }
                        }
                        String mmprojName = m.getMMProjName();
                        String mmprojLine = "";
                        if (mmprojName != null && !mmprojName.isEmpty()) {
                            File mmprojFile = new File(KANTVUtils.getSDCardDataPath() + mmprojName);
                            String mmprojSize = mmprojFile.exists()
                                    ? String.format(Locale.US, "%.1f MB", mmprojFile.length() / (double) (1L << 20))
                                    : "<missing>";
                            mmprojLine = "\nmmproj      : " + mmprojName + "  (" + mmprojSize + ")";
                        }
                        modelInfo = "LLM model : " + modelName + "  (" + sizeStr + ")" + mmprojLine;
                    }
                } else {
                    modelInfo = "LLM model : <index " + llmIndex + " invalid; please select one from the list below>";
                }
            } catch (Throwable t) {
                modelInfo = "LLM model : <unavailable: " + t.getClass().getSimpleName() + ">";
                KANTVLog.g(TAG, "populateHeader model info failed: " + t.toString());
            }
            mHeaderPreference.setModelInfo(modelInfo);

            // 3) resident status: query the native side. Right now the LLM/MTMD
            // benchmark path does not expose this, so the line says "Not resident
            // (reloaded per benchmark)" to set user expectations honestly.
            // The model name is included so the user knows which model the
            // Resident line refers to.
            String residentStatus;
            try {
                String loadedPath = ggmljava.llm_get_loaded_model_path();
                if (loadedPath != null && !loadedPath.isEmpty()) {
                    residentStatus = "Resident   : " + loadedPath + "  (loaded)";
                } else {
                    // Try to attach the currently selected model's name to the line.
                    String tail = "";
                    try {
                        int llmIndex = mSettings.getLLMModel();
                        int llmCount = KANTVAIModelMgr.getInstance().getLLMModelCounts();
                        if (llmIndex >= 0 && llmIndex < llmCount) {
                            KANTVAIModel m = KANTVAIModelMgr.getInstance().getKANTVAIModelFromIndex(llmIndex + KANTVAIModelMgr.getInstance().getNonLLMModelCounts());
                            if (m != null) {
                                tail = "  (" + m.getName() + ")";
                            }
                        }
                    } catch (Throwable ignore) {
                        // best-effort: leave tail empty
                    }
                    residentStatus = "Resident   : not loaded (reloaded per benchmark)" + tail;
                }
            } catch (Throwable t) {
                residentStatus = "Resident   : <native API not available>";
            }
            mHeaderPreference.setResidentStatus(residentStatus);
        } catch (Throwable t) {
            // Last-resort: never let populateHeader crash the fragment.
            KANTVLog.g(TAG, "populateHeader failed: " + t.toString());
        }
    }


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
                 if (llmModelFile.exists() && mmprojModelFile.exists()) {
                     KANTVLog.g(TAG, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelName);
                     KANTVUtils.showMsgBox(mActivity, "LLM model file already exist: " + KANTVUtils.getSDCardDataPath() + userChooseModelName);
                     return true;
                 }
             } else {
                 if (llmModelFile.exists()) {
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
             manager.showUpdateDialog();
         }

         return true;
     }


     public static native int kantv_anti_remove_rename_this_file();
 }
