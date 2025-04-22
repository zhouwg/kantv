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
package com.kantvai.kantvplayer.ui.fragment.settings;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.CheckBoxPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceDataStore;


import com.kantvai.kantvplayer.ui.activities.ShellActivity;
import com.kantvai.kantvplayer.utils.Settings;
import com.kantvai.kantvplayer.utils.database.DataBaseInfo;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.BuildConfig;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.activities.WebViewActivity;
import com.kantvai.kantvplayer.utils.AppConfig;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import kantvai.media.player.KANTVAssetLoader;
import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVContentDescriptor;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVMediaType;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.KANTVJNIDecryptBuffer;
import kantvai.media.player.KANTVMediaGridItem;
import kantvai.media.player.KANTVDRMManager;

public class SystemSettingFragment extends BaseSettingsFragment {
    private final AppSettingDataStore dataStore = new AppSettingDataStore();

    private static final String TAG = SystemSettingFragment.class.getName();
    private static ShellActivity mActivity;
    private Context mContext;
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;
    private Settings mSettings;

    private List<KANTVContentDescriptor> mContentList = new ArrayList<KANTVContentDescriptor>();

    private KANTVDRM mKANTVDRM = KANTVDRM.getInstance();

    private ProgressDialog mProgressDialog;
    private CheckBoxPreference mWidgetEPGEnabled;
    private EditTextPreference mWidgetEPGServer;
    private CheckBoxPreference mWidgetEPGUpdate;

    private EditTextPreference mWidgetNginxServer;
    private EditTextPreference mWidgetApiGatewayServer;
    private CheckBoxPreference mWidgetTEEEnabled;
    private CheckBoxPreference mWidgetVideoDisabled;
    private CheckBoxPreference mWidgetAudioDisabled;
    private CheckBoxPreference mWidgetMediaCodecEnabled;
    private EditTextPreference mWidgetDumpSize;
    private EditTextPreference mWidgetDumpCounts;
    private CheckBoxPreference mWidgetUsingFFmpegCodec;
    private EditTextPreference mWidgetRecordTime;
    private EditTextPreference mWidgetRecordSize;
    private CheckBoxPreference mWidgetDevMode;

    @Override
    public String getTitle() {
        return mActivity.getBaseContext().getString(R.string.system_settings);
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

        addPreferencesFromResource(R.xml.settings_app);

        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

        mWidgetEPGServer = (EditTextPreference) findPreference("pref.kantvserver");
        mWidgetEPGUpdate = (CheckBoxPreference) findPreference("pref.epgupdate");
        mWidgetTEEEnabled = (CheckBoxPreference) findPreference("pref.using_tee");
        mWidgetAudioDisabled = (CheckBoxPreference) findPreference("pref.disable_audio");
        mWidgetVideoDisabled = (CheckBoxPreference) findPreference("pref.disable_video");
        mWidgetMediaCodecEnabled = (CheckBoxPreference) findPreference("pref.using_media_codec");
        mWidgetDumpSize = (EditTextPreference) findPreference("pref.dump_size");
        mWidgetDumpCounts = (EditTextPreference) findPreference("pref.dump_counts");
        mWidgetUsingFFmpegCodec = (CheckBoxPreference) findPreference("pref.using_ffmpeg_codec");
        mWidgetNginxServer = (EditTextPreference) findPreference("pref.nginx");
        mWidgetApiGatewayServer = (EditTextPreference) findPreference("pref.apigateway");
        mWidgetRecordSize = (EditTextPreference) findPreference("pref.record_size");
        mWidgetRecordTime = (EditTextPreference) findPreference("pref.record_time");
        mWidgetDevMode = (CheckBoxPreference) findPreference("pref.dev_mode");

        mWidgetEPGServer.setText(KANTVUtils.getKANTVServer());
        KANTVLog.j(TAG, "dev mode:" + mSettings.getDevMode());
        KANTVLog.d(TAG, "customized EPG Server Url:" + KANTVUtils.getKANTVServer());
        KANTVLog.d(TAG, "epg enabled: " + mSettings.getEPGEnabled());
        KANTVLog.j(TAG, "close_splash_page: " + mSettings.isCloseSplashPage());
        KANTVLog.j(TAG, "close_splash_page: " + AppConfig.getInstance().isCloseSplashPage());

        if (mSettings.getDevMode()) {
            if (mWidgetDevMode != null)
                mWidgetDevMode.setChecked(true);
        } else {
            if (mWidgetDevMode != null)
                mWidgetDevMode.setChecked(false);
        }
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Preference cachePath = findPreference("pref.cleanupcache");
        cachePath.setOnPreferenceClickListener(preference -> {
            String appCachePath = KANTVUtils.getDataPath(mContext);
            KANTVLog.j(TAG, "app cache path is:" + appCachePath);
            File file = new File(appCachePath + "kantv.bin");
            if (file.exists()) {
                KANTVLog.j(TAG, "cache path exist:" + file.getAbsolutePath());
                file.delete();
            } else {
                KANTVLog.j(TAG, "cache path not exist:" + file.getAbsolutePath());
            }

            File dbFile = new File(KANTVUtils.getDataPath(mContext) + DataBaseInfo.DATABASE_NAME);
            if (dbFile.exists()) {
                KANTVLog.j(TAG, "db path exist:" + dbFile.getAbsolutePath());
                DataBaseManager.getInstance().closeDatabase();
                dbFile.delete();
            } else {
                KANTVLog.j(TAG, "db path not exist:" + dbFile.getAbsolutePath());
            }

            String qnnlibPath = KANTVAssetLoader.getDataPath(mContext) + "qnnlib";
            File destFile = new File(qnnlibPath);
            if (destFile.exists()) {
                KANTVLog.j(TAG, "dir: " + qnnlibPath + " exist");
                KANTVUtils.deleteFolder(qnnlibPath);
            } else {
                KANTVLog.j(TAG, "dir: " + qnnlibPath + " not exist");
            }

            String asrAudioFileName = KANTVUtils.getDataPath(mContext) + "audio.wav";
            String asrModelFileName = KANTVUtils.getDataPath(mContext) + "deepspeech-0.9.3-models.tflite";
            File asrAudioFile = new File(asrAudioFileName);
            File asrModelFile = new File(asrModelFileName);
            if (asrAudioFile.exists()) {
                asrAudioFile.delete();
            }
            if (asrModelFile.exists()) {
                asrModelFile.delete();
            }

            String ttsModelFileName1 = KANTVUtils.getDataPath(mContext) + "mb_melgan_csmsc_arm.nb";
            String ttsModelFileName2 = KANTVUtils.getDataPath(mContext) + "fastspeech2_csmsc_arm.nb";
            File ttsModelFile1 = new File(ttsModelFileName1);
            File ttsModelFile2 = new File(ttsModelFileName2);
            if (ttsModelFile1.exists()) {
                ttsModelFile1.delete();
            }
            if (ttsModelFile2.exists()) {
                ttsModelFile2.delete();
            }

            Toast.makeText(mContext, "clean up cache done", Toast.LENGTH_LONG).show();
            return true;
        });

        Preference txtVersion = (Preference) findPreference("pref.version");
        Preference txtVersionSummary = (Preference) findPreference("pref.version");

        if (txtVersion != null) {
            txtVersion.setTitle(mContext.getString(R.string.version) + ":" + KANTVUtils.getKANTVAPKVersion());
            txtVersion.setSummary(mContext.getString(R.string.buildTime) + ":" + BuildConfig.BUILD_TIME);
        }


        if (findPreference("aboutkantv") != null) {
            findPreference("aboutkantv").setOnPreferenceClickListener(preference -> {
                Intent intent_about = new Intent(view.getContext(), WebViewActivity.class);
                intent_about.putExtra("title", "about KanTV");
                intent_about.putExtra("link", "http://" + KANTVUtils.getKANTVMasterServer() + "/aboutkantv.html");
                startActivity(intent_about);
                return true;
            });
        }

        if (findPreference("aboutauthorinfo") != null) {
            findPreference("aboutauthorinfo").setOnPreferenceClickListener(preference -> {
                Intent intent_about = new Intent(view.getContext(), WebViewActivity.class);
                intent_about.putExtra("title", "official website");
                intent_about.putExtra("link", "http://www.kantvai.com");
                startActivity(intent_about);
                return true;
            });
        }
    }


    static class AppSettingDataStore extends PreferenceDataStore {
        @SuppressWarnings("PMD.UncommentedEmptyMethodBody")
        @Override
        public void putString(String key, @Nullable String value) {
            KANTVLog.j(TAG, "key : " + key);

        }

        @Override
        public void putBoolean(String key, boolean value) {
            KANTVLog.j(TAG, "key : " + key);
            if (key.equals("pref.close_splash_page")) {
                AppConfig.getInstance().setCloseSplashPage(value);
            }
        }

        @Override
        public boolean getBoolean(String key, boolean defValue) {
            KANTVLog.j(TAG, "key : " + key);
            if (key.equals("pref.close_splash_page")) {
                return AppConfig.getInstance().isCloseSplashPage();
            }
            return super.getBoolean(key, defValue);
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
            KANTVLog.j(TAG, "key : " + key);

            if (key.contains("pref.lang")) {
                mSettings.updateUILang(mActivity);
            }

            if (key.contains("pref.epgenabled")) {
                KANTVLog.j(TAG, "epg enabled: " + mSharedPreferences.getBoolean("pref.pref.epgenabled", true));
            }

            if (key.contains("pref.dev_mode")) {
                KANTVLog.d(TAG, "dump mode " + mSettings.getDumpMode());
                KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                KANTVLog.d(TAG, "dump duration " + KANTVUtils.getDumpDuration());
                KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
            }

            if (key.contains("pref.playerEngine")) {
                KANTVLog.d(TAG, "dump mode " + mSettings.getDumpMode());
                KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                KANTVLog.d(TAG, "dump duration " + KANTVUtils.getDumpDuration());
                KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
                KANTVUtils.setPlayEngine(mSettings.getPlayerEngine());

                KANTVLog.d(TAG, "playerEngine: " + mSettings.getPlayerEngine());
                if (mSettings.getPlayerEngine() != KANTVUtils.PV_PLAYERENGINE__Exoplayer) {
                    KANTVLog.j(TAG, "switch to non-Exoplayer engine");
                    if (mWidgetUsingFFmpegCodec != null) {
                        mWidgetUsingFFmpegCodec.setEnabled(false);
                    }

                    if (mWidgetTEEEnabled != null) {
                        mWidgetTEEEnabled.setEnabled(false);
                        if (KANTVUtils.isRunningOnAmlogicBox()) {
                            if ((mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__AndroidMediaPlayer)
                                    || (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__Exoplayer)) {
                                SharedPreferences.Editor editor = mSharedPreferences.edit();
                                key = mContext.getString(R.string.pref_key_using_tee);
                                editor.putBoolean(key, true);
                                editor.commit();
                                mWidgetTEEEnabled.setChecked(true);
                            }
                            if (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
                                SharedPreferences.Editor editor = mSharedPreferences.edit();
                                key = mContext.getString(R.string.pref_key_using_tee);
                                editor.putBoolean(key, false);
                                editor.commit();
                                mWidgetTEEEnabled.setChecked(false);
                            }
                        }
                    }
                    KANTVDRMManager.stop();
                } else {
                    KANTVLog.j(TAG, "switch to Exoplayer engine");
                    if (mWidgetUsingFFmpegCodec != null) {
                        mWidgetUsingFFmpegCodec.setEnabled(true);
                    }

                    if ((mWidgetTEEEnabled != null) && (KANTVUtils.isRunningOnAmlogicBox())) {
                        mWidgetTEEEnabled.setEnabled(true);
                    }

                    {
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                KANTVDRMManager.start(mActivity.getBaseContext());
                            }
                        }).start();
                    }
                }


                if (mSettings.getPlayerEngine() != KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
                    if (mWidgetMediaCodecEnabled != null) {
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        key = mContext.getString(R.string.pref_key_using_media_codec);
                        editor.putBoolean(key, true);
                        editor.commit();
                        mWidgetMediaCodecEnabled.setChecked(true);
                        mWidgetMediaCodecEnabled.setEnabled(false);
                    }
                } else {
                    if (mWidgetMediaCodecEnabled != null) {
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        key = mContext.getString(R.string.pref_key_using_media_codec);
                        editor.putBoolean(key, mSettings.getUsingMediaCodec());
                        editor.commit();
                        mWidgetMediaCodecEnabled.setChecked(mSettings.getUsingMediaCodec());
                        mWidgetMediaCodecEnabled.setEnabled(true);
                    }
                }
            }

            KANTVLog.j(TAG, "key: " + key);
            if (key.contains("pref.kantvserver")) {
                String serverUrl = mSettings.getKANTVServer();
                KANTVLog.d(TAG, "new kantv server url: " + mWidgetEPGServer.getText());
                //TODO
                String regex = "^([hH][tT]{2}[pP]:/*|[hH][tT]{2}[pP][sS]:/*|[fF][tT][pP]:/*)(([A-Za-z0-9-~]+).)+([A-Za-z0-9-~\\/])+(\\:{0,1}(([A-Za-z0-9-~]+\\={0,1})([A-Za-z0-9-~]*)\\&{0,1})*)$";
                Pattern pattern = Pattern.compile(regex);
                //if (pattern.matcher(serverUrl).matches()) {
                if (true) {
                    KANTVUtils.updateKANTVServer(serverUrl);
                    KANTVLog.j(TAG, "new kantvserver url:" + mSettings.getKANTVServer());
                    KANTVLog.j(TAG, "new kantvserver url:" + KANTVUtils.getKANTVServer());
                    KANTVLog.j(TAG, "new kantv um server:" + KANTVUtils.getKanTVUMServer());
                    Toast.makeText(mContext, mActivity.getBaseContext().getString(R.string.serverUpdateOK) + ":" + serverUrl, Toast.LENGTH_LONG).show();
                } else {
                    Toast.makeText(mContext, mActivity.getBaseContext().getString(R.string.serverUpdateFailed) + ":" + serverUrl, Toast.LENGTH_LONG).show();
                }
            }
        }
    };


    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        String key = preference.getKey();
        KANTVLog.j(TAG, "key : " + key);
        if (preference instanceof CheckBoxPreference) {
            KANTVLog.d(TAG, "preference : " + preference.getKey() + ", status:" + mSharedPreferences.getBoolean(key, false));
        }

        if (key.contains("close_splash_page")) {
            KANTVLog.j(TAG, "close_splash_page: " + mSharedPreferences.getBoolean("close_splash_page", false));
            KANTVLog.j(TAG, "close_splash_page: " + AppConfig.getInstance().isCloseSplashPage());
            if (mSharedPreferences.getBoolean("close_splash_page", false))
                AppConfig.getInstance().setCloseSplashPage(true);
            else
                AppConfig.getInstance().setCloseSplashPage(false);

            KANTVLog.j(TAG, "close_splash_page: " + AppConfig.getInstance().isCloseSplashPage());
        }

        if (key.contains("pref.dev_mode")) {
            KANTVLog.j(TAG, "development mode:" + mSettings.getDevMode());
            KANTVUtils.setExpertMode(mSettings.getDevMode());
        }


        if (key.contains("pref.kantvserver")) {
            KANTVLog.j(TAG, "kantv server address: " + mWidgetEPGServer.getText());
        }

        if (key.contains("pref.epgupdate")) {
            String status = getResources().getString(R.string.epg_updating);
            mWidgetEPGUpdate.setEnabled(false);
            Toast.makeText(mContext, mContext.getString(R.string.epg_updating), Toast.LENGTH_SHORT).show();
            launchEPGUpdateThread();
        }

        if (key.contains("pref.checkupdate"))  {
            KANTVLog.j(TAG, "check new version");
            WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
            attributes.screenBrightness = 1.0f;
            mActivity.getWindow().setAttributes(attributes);
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            SoftwareUpgrade swUpgrade=new SoftwareUpgrade(mActivity);
            swUpgrade.checkUpdateInfo();
        }


        if (key.contains("pref.playerEngine")) {
            KANTVLog.d(TAG, "playerEngine: " + mSettings.getPlayerEngine());
        }

        if (key.contains("pref.exitapp")) {
            KANTVLog.d(TAG, "exit apk");
            KANTVUtils.exitAPK(mActivity);
        }

        return true;
    }


    private boolean downloadEPGData(KANTVMediaType mMediaType) {
        boolean result = false;
        KANTVLog.j(TAG, "enter EPG update");
        if (!KANTVUtils.getReleaseMode()) {
            if (!mSettings.getKANTVServer().equals(KANTVUtils.getKANTVServer())) {
                KANTVLog.d(TAG, "KANTVUtils.getKANTVServer() != mSettings.getKANTVServer(), adjust it manually");
                KANTVUtils.setKANTVServer(mSettings.getKANTVServer());
            }
        }

        String epgServerUrl;
        epgServerUrl = "http://" + KANTVUtils.getKANTVMasterServer();
        KANTVLog.j(TAG, "KANTV master EPG Server: " + epgServerUrl);
        String xmlEPGURI;
        String xmlAppURI = "kantv.bin";
        byte[] realPayload = null;
        KANTVLog.d(TAG, "xml uri: " + xmlAppURI);
        xmlEPGURI = KANTVUtils.getKANTVMainEPGUrl();
        KANTVLog.j(TAG, "xmlEPGURI: " + xmlEPGURI);
        SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss");
        Date date = new Date(System.currentTimeMillis());
        String xmlLocalName = xmlAppURI.substring(0, xmlAppURI.indexOf(".")) + ".bin";
        String xmlLocalBackupName = xmlAppURI.substring(0, xmlAppURI.indexOf(".")) + "-backup.bin";
        KANTVLog.j(TAG, "xmlLocalName: " + xmlLocalName);
        KANTVLog.j(TAG, "xmlLocalBackupName: " + xmlLocalBackupName);
        File xmlLocalFile = new File(KANTVUtils.getDataPath(mContext), xmlLocalName);
        File xmlLocalBackupFile = new File(KANTVUtils.getDataPath(mContext), xmlLocalBackupName);
        KANTVLog.j(TAG, "epg Local url: " + xmlLocalFile.getAbsolutePath());
        KANTVLog.j(TAG, "epg LocalBackup url: " + xmlLocalBackupFile.getAbsolutePath());
        int downloadResult = 0;
        KANTVJNIDecryptBuffer dataEntity = KANTVJNIDecryptBuffer.getInstance();
        int nPaddingLen = 0;
        try {
            if (!xmlLocalFile.exists()) {
                KANTVLog.j(TAG, "file not exist: " + xmlLocalFile.getAbsolutePath());
                downloadResult = mKANTVDRM.ANDROID_JNI_DownloadFile(xmlEPGURI, xmlLocalFile.getAbsolutePath());
                KANTVLog.j(TAG, "download file " + xmlLocalFile.getAbsolutePath() + " result:" + downloadResult);
                if (0 == downloadResult) {
                    KANTVLog.j(TAG, "epg download successfully");
                    String xmlLoaddedFileName = xmlLocalFile.getAbsolutePath();
                    KANTVLog.j(TAG, "load epg from file: " + xmlLoaddedFileName + " which download from remote server:" + xmlEPGURI);
                    {
                        long beginTime = 0;
                        long endTime = 0;
                        beginTime = System.currentTimeMillis();
                        KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
                        File file = new File(xmlLoaddedFileName);
                        int fileLength = (int) file.length();
                        KANTVLog.j(TAG, "encrypted file len:" + fileLength);
                        byte[] fileContent = new byte[fileLength];
                        FileInputStream in = new FileInputStream(file);
                        in.read(fileContent);
                        in.close();
                        KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                        if (fileLength < 2) {
                            KANTVLog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                            return false;
                        }
                        byte[] paddingInfo = new byte[2];
                        paddingInfo[0] = 0;
                        paddingInfo[1] = 0;
                        System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                        KANTVLog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                        KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                        KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                        nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                        KANTVLog.j(TAG, "padding len:" + nPaddingLen);

                        byte[] payload = new byte[fileLength - 2];
                        System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);

                        KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                        mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                        KANTVLog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());

                        if (dataEntity.getData() != null) {
                            KANTVLog.j(TAG, "decrypted ok");
                        } else {
                            KANTVLog.j(TAG, "decrypted failed");
                        }
                        KANTVLog.j(TAG, "load encryted epg data ok now");
                        endTime = System.currentTimeMillis();
                        KANTVLog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                        realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                        System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                        if (!KANTVUtils.getReleaseMode()) {
                            KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "download_tv_decrypt_debug.xml");
                        }

                    }
                    mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
                    if (mContentList != null)
                        KANTVLog.j(TAG, "content counts:" + mContentList.size());
                    if ((mContentList == null) || (0 == mContentList.size())) {
                        KANTVLog.j(TAG, "download xml:" + xmlLoaddedFileName + " is invalid");
                        result = false;
                        return result;
                    } else {
                        result = true;
                    }
                } else {
                    KANTVLog.j(TAG, "epg xml download failed");
                    result = false;
                    return result;
                }
            } else {
                KANTVLog.j(TAG, "backup exist file: " + xmlLocalFile.getAbsolutePath());
                KANTVUtils.copyFile(xmlLocalFile, xmlLocalBackupFile);
                downloadResult = mKANTVDRM.ANDROID_JNI_DownloadFile(xmlEPGURI, xmlLocalFile.getAbsolutePath());
                KANTVLog.j(TAG, "download file " + xmlLocalFile.getAbsolutePath() + " result:" + downloadResult);
                if (0 == downloadResult) {
                    KANTVLog.j(TAG, "epg xml download successfully");
                    String xmlLoaddedFileName = xmlLocalFile.getAbsolutePath();
                    KANTVLog.j(TAG, "load epg from xml file: " + xmlLoaddedFileName + " which download from remote server:" + xmlEPGURI);
                    {
                        long beginTime = 0;
                        long endTime = 0;
                        beginTime = System.currentTimeMillis();
                        KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
                        File file = new File(xmlLoaddedFileName);
                        int fileLength = (int) file.length();
                        KANTVLog.j(TAG, "encrypted file len:" + fileLength);

                        byte[] fileContent = new byte[fileLength];
                        FileInputStream in = new FileInputStream(file);
                        in.read(fileContent);
                        in.close();
                        KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                        if (fileLength < 2) {
                            KANTVLog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                            return false;
                        }
                        byte[] paddingInfo = new byte[2];
                        paddingInfo[0] = 0;
                        paddingInfo[1] = 0;
                        System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                        KANTVLog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                        KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                        KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                        nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                        KANTVLog.j(TAG, "padding len:" + nPaddingLen);

                        byte[] payload = new byte[fileLength - 2];
                        System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                        KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                        mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());

                        KANTVLog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());
                        if (dataEntity.getData() != null) {
                            KANTVLog.j(TAG, "decrypted ok");
                        } else {
                            KANTVLog.j(TAG, "decrypted failed");
                        }
                        KANTVLog.j(TAG, "load encryted epg data ok now");
                        endTime = System.currentTimeMillis();
                        KANTVLog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                        realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                        System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                        if (!KANTVUtils.getReleaseMode()) {
                            KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "download_tv_decrypt_debug.xml");
                        }
                    }
                    if (realPayload == null) {
                        KANTVLog.j(TAG, "it shouldn't happen, pls check");
                        result = false;
                        return result;
                    }
                    mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
                    if (mContentList != null)
                        KANTVLog.j(TAG, "content counts:" + mContentList.size());
                    if ((mContentList == null) || (0 == mContentList.size())) {
                        KANTVLog.j(TAG, "download xml:" + xmlLoaddedFileName + " is invalid");
                        KANTVUtils.copyFile(xmlLocalBackupFile, xmlLocalFile);
                        result = false;
                        return result;
                    } else {
                        result = true;
                    }
                } else {
                    KANTVLog.j(TAG, "epg xml download failed");
                    KANTVUtils.copyFile(xmlLocalBackupFile, xmlLocalFile);
                    KANTVLog.j(TAG, "return false;");
                    result = false;
                    return result;
                }
            }
        } catch (IOException ex) {
            KANTVLog.j(TAG, "failed to copy file");
            result = false;
            return result;
        }
        KANTVLog.j(TAG, "leave EPG update");
        KANTVUtils.setEpgUpdatedStatus(mMediaType, result);

        return result;
    }

    private void launchEPGUpdateThread() {
        new Thread(new Runnable() {
            //@Override
            public void run() {
                {
                    boolean bEPGUpdatedOK = downloadEPGData(KANTVMediaType.MEDIA_TV);
                    stopUIBuffering(bEPGUpdatedOK);
                    if (false) {
                        String info = null;
                        if (bEPGUpdatedOK) {
                            info = mContext.getString(R.string.epgUpdateOK);
                        } else {
                            info = mContext.getString(R.string.epgUpdateFailed) + ":" + KANTVUtils.getKANTVMasterServer();
                        }
                        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
                        builder.setTitle(mActivity.getString(R.string.epg_updating));
                        builder.setMessage(info);
                        builder.setCancelable(true);
                        builder.setNegativeButton(R.string.OK,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        dialog.dismiss();
                                    }
                                });
                        builder.show();
                    }
                }
            }
        }).start();
    }

    private void startUIBuffering(String status) {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog == null) {
                    mProgressDialog = new ProgressDialog(mActivity);
                    mProgressDialog.setMessage(status);
                    mProgressDialog.setIndeterminate(true);
                    mProgressDialog.setCancelable(true);
                    mProgressDialog.setCanceledOnTouchOutside(false);

                    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialogInterface) {
                            if (mProgressDialog != null) {
                                mProgressDialog.dismiss();
                                mProgressDialog = null;
                            }
                        }
                    });
                    mProgressDialog.show();
                }
            }
        });
    }


    private void stopUIBuffering(boolean bEPGUpdatedOK) {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (bEPGUpdatedOK) {
                    Toast.makeText(mContext, mContext.getString(R.string.epgUpdateOK), Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(mContext, mContext.getString(R.string.epgUpdateFailed) + " KTV Server Url:" + mSettings.getKANTVServer(), Toast.LENGTH_SHORT).show();
                }
                mWidgetEPGUpdate.setEnabled(true);
            }
        });
    }

    public static native int kantv_anti_remove_rename_this_file();
}
