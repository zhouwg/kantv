 /*
  * Copyright (c) Project KanTV. 2021-2023
  * Copyright (c) 2024- KanTV Authors
  */
package com.kantvai.kantvplayer.ui.fragment.settings;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.CheckBoxPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceDataStore;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.activities.ShellActivity;
import com.kantvai.kantvplayer.utils.AppConfig;
import com.kantvai.kantvplayer.utils.Constants;
import com.kantvai.kantvplayer.utils.Settings;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.KANTVDRMManager;

public class PlaySettingFragment extends BaseSettingsFragment {
    private static final String TAG = PlaySettingFragment.class.getName();

    private static final String[] pixelFormatArrayValue = new String[]{
            Constants.PlayerConfig.PIXEL_AUTO,
            Constants.PlayerConfig.PIXEL_RGB565,
            Constants.PlayerConfig.PIXEL_RGB888,
            Constants.PlayerConfig.PIXEL_RGBX8888,
            Constants.PlayerConfig.PIXEL_YV12,
            Constants.PlayerConfig.PIXEL_OPENGL_ES2
    };

    private static final String[] playerTypeValue = new String[]{
            String.valueOf(com.kantvai.kantvplayer.player.common.utils.Constants.PLAYERENGINE__FFmpeg),
            String.valueOf(com.kantvai.kantvplayer.player.common.utils.Constants.PLAYERENGINE__Exoplayer)
    };

    private final String[] keySetOfInvisiblePreferenceOnExoPlayerEnable = new String[]{
            "media_code_c",
            "pref.using_media_codec",
            "open_sl_es",
            "pref.using_opensl_es",
            "pixel_format_type",
            "pref.pixel_format"
    };

    private final PlaySettingDataStore dataStore = new PlaySettingDataStore();
    private static ShellActivity mActivity;
    private Context mContext;
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;
    private Settings mSettings;

    private IjkListPreference mWidgetDumpMode;
    private CheckBoxPreference mWidgetBackgroundPlay;
    private CheckBoxPreference mWidgetTEEEnabled;
    private CheckBoxPreference mWidgetVideoDisabled;
    private CheckBoxPreference mWidgetAudioDisabled;
    private CheckBoxPreference mWidgetMediaCodecEnabled;
    private EditTextPreference mWidgetDumpSize;
    private EditTextPreference mWidgetDumpCounts;
    private CheckBoxPreference mWidgetUsingFFmpegCodec;
    private Preference         mWidgetAutoLoadNetwork;

    private EditTextPreference mWidgetNginxServer;
    private EditTextPreference mWidgetApiGatewayServer;

    @Override
    public String getTitle() {
        return mActivity.getBaseContext().getString(R.string.playback_settings);
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        mActivity = ShellActivity.getInstance();
        mAppContext = mActivity.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mContext = mActivity.getBaseContext();
        mSettings.updateUILang(mActivity);

        addPreferencesFromResource(R.xml.settings_play);

        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

        mWidgetBackgroundPlay = (CheckBoxPreference)findPreference("pref.enable_background_play");
        mWidgetTEEEnabled = (CheckBoxPreference)findPreference("pref.using_tee");
        mWidgetAudioDisabled = (CheckBoxPreference)findPreference("pref.disable_audio");
        mWidgetVideoDisabled = (CheckBoxPreference)findPreference("pref.disable_video");
        mWidgetMediaCodecEnabled = (CheckBoxPreference)findPreference("pref.using_media_codec");
        mWidgetDumpSize = (EditTextPreference)findPreference("pref.dump_size");
        mWidgetDumpCounts = (EditTextPreference)findPreference("pref.dump_counts");
        mWidgetUsingFFmpegCodec = (CheckBoxPreference)findPreference("pref.using_ffmpeg_codec");
        mWidgetDumpMode  = (IjkListPreference)findPreference("pref.dump_mode");
        mWidgetAutoLoadNetwork = findPreference("auto_load_network_subtitle");//replaced with AI subtitle from 03-17-2024

        KANTVLog.j(TAG, "dev  mode " + mSettings.getDevMode());

        if (mSettings.getDevMode()) {
            if (mWidgetDumpMode != null)
                mWidgetDumpMode.setEnabled(true);
        } else {
            if (mWidgetDumpMode != null)
                mWidgetDumpMode.setEnabled(false);
        }

        if (mWidgetBackgroundPlay != null) {
            if (mSettings.getEnableBackgroundPlay()) {
                mWidgetBackgroundPlay.setChecked(true);
            } else {
                mWidgetBackgroundPlay.setChecked(false);
            }
        }

        if (KANTVUtils.isRunningOnPhone()) {
            if (mWidgetTEEEnabled != null) {
                mWidgetTEEEnabled.setEnabled(false);
                SharedPreferences.Editor editor = mSharedPreferences.edit();
                String key = mContext.getString(R.string.pref_key_using_tee);
                editor.putBoolean(key, false);
                editor.commit();
            }
        }

        KANTVLog.d(TAG, "player engine:" + mSettings.getPlayerEngine());
        if (mSettings.getDevMode() && (mWidgetAutoLoadNetwork != null)) {
            if (mSettings.getUsingNetworkSubtitle()) {
                mWidgetAutoLoadNetwork.setVisible(true);
            } else {
                mWidgetAutoLoadNetwork.setVisible(false);
            }
        }

        boolean isExoPlayer = false;
        if (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__Exoplayer) {
            if (mWidgetUsingFFmpegCodec != null) {
                mWidgetUsingFFmpegCodec.setEnabled(true);
            }
            isExoPlayer = true;
        }

        if (mSettings.getPlayerEngine() != KANTVUtils.PV_PLAYERENGINE__Exoplayer) {
            isExoPlayer = false;
            if (mWidgetUsingFFmpegCodec != null) {
                mWidgetUsingFFmpegCodec.setEnabled(false);
            }
            if (mWidgetTEEEnabled != null) {
                mWidgetTEEEnabled.setEnabled(false);

                if (KANTVUtils.isRunningOnAmlogicBox()) {
                    if ((mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__AndroidMediaPlayer)
                            || (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__Exoplayer)) {
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        String key = mContext.getString(R.string.pref_key_using_tee);
                        editor.putBoolean(key, true);
                        editor.commit();
                        mWidgetTEEEnabled.setChecked(true);
                    }

                    if (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        String key = mContext.getString(R.string.pref_key_using_tee);
                        editor.putBoolean(key, false);
                        editor.commit();
                        mWidgetTEEEnabled.setChecked(false);
                    }
                }
            }
        }

        if (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
            if (mWidgetMediaCodecEnabled != null) {
                SharedPreferences.Editor editor = mSharedPreferences.edit();
                String key = mContext.getString(R.string.pref_key_using_media_codec);
                editor.putBoolean(key, mSettings.getUsingMediaCodec());
                editor.commit();
                mWidgetMediaCodecEnabled.setEnabled(true);
            }

        }

        for (String key : keySetOfInvisiblePreferenceOnExoPlayerEnable) {
            if (findPreference(key) != null) {
                //findPreference(key).setVisible(!isExoPlayer);
            }
        }

        KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
        if (!mSettings.getDevMode()) {
            PreferenceCategory pcSUBTITLE =  (PreferenceCategory) findPreference("subtitle");
            if (pcSUBTITLE != null) {
                pcSUBTITLE.setVisible(false);
            }

            PreferenceCategory pcDRM =  (PreferenceCategory) findPreference("drm");
            if (pcDRM != null) {
                pcDRM.setVisible(false);
            }

            PreferenceCategory pcDEBUG =  (PreferenceCategory) findPreference("debug");
            if (pcDEBUG != null) {
                pcDEBUG.setVisible(false);
            }
        }


    }

    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        KANTVLog.d(TAG, "preference : " + preference.getKey());
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

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }

    static class PlaySettingDataStore extends PreferenceDataStore {
        @Nullable
        @Override
        public String getString(String key, @Nullable String defValue) {
            switch (key) {
                case "pixel_format_type":
                    return AppConfig.getInstance().getPixelFormat();
                case "player_type":
                    return String.valueOf(AppConfig.getInstance().getPlayerType());
                default:
                    return super.getString(key, defValue);
            }
        }

        @Override
        public void putString(String key, @Nullable String value) {
            switch (key) {
                case "pixel_format_type":
                    for (String type : pixelFormatArrayValue) {
                        if (type.equals(value)) {
                            AppConfig.getInstance().setPixelFormat(value);
                        }
                    }
                    break;
                case "player_type":
                    for (String type : playerTypeValue) {
                        if (type.equals(value)) {
                            AppConfig.getInstance().setPlayerType(Integer.parseInt(type));
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        @Override
        public boolean getBoolean(String key, boolean defValue) {
            switch (key) {
                case "media_code_c":
                    return AppConfig.getInstance().isOpenMediaCodeC();
                case "open_sl_es":
                    return AppConfig.getInstance().isOpenSLES();
                case "surface_renders":
                    return AppConfig.getInstance().isSurfaceRenders();
                case "auto_load_local_subtitle":
                    return AppConfig.getInstance().isAutoLoadLocalSubtitle();
                case "auto_load_network_subtitle":
                    return AppConfig.getInstance().isAutoLoadNetworkSubtitle();
                case "network_subtitle":
                    return AppConfig.getInstance().isUseNetWorkSubtitle();
                default:
                    return super.getBoolean(key, defValue);
            }
        }

        @Override
        public void putBoolean(String key, boolean value) {
            switch (key) {
                case "media_code_c":
                    AppConfig.getInstance().setOpenMediaCodeC(value);
                    break;
                case "open_sl_es":
                    AppConfig.getInstance().setOpenSLES(value);
                    break;
                case "surface_renders":
                    AppConfig.getInstance().setSurfaceRenders(value);
                    break;
                case "auto_load_local_subtitle":
                    AppConfig.getInstance().setAutoLoadLocalSubtitle(value);
                    break;
                case "auto_load_network_subtitle":
                    AppConfig.getInstance().setAutoLoadNetworkSubtitle(value);
                    break;
                case "network_subtitle":
                    AppConfig.getInstance().setUseNetWorkSubtitle(value);
                    break;
                default:
                    break;
            }
        }
    }

    private SharedPreferences.OnSharedPreferenceChangeListener mSharedPreferenceChangeListener =  new SharedPreferences.OnSharedPreferenceChangeListener() {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            KANTVLog.d(TAG, "key : " + key);

            if (key.contains("pref.disable_audio")) {
                KANTVLog.d(TAG, "disable_audio: " + mSettings.getAudioDisabled());
                KANTVUtils.setDisableAudioTrack(mSettings.getAudioDisabled());
                if (mSettings.getAudioDisabled()) {
                    if (mWidgetVideoDisabled != null) {
                        mWidgetVideoDisabled.setEnabled(false);
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        key = mContext.getString(R.string.pref_key_disable_video);
                        editor.putBoolean(key, false);
                        editor.commit();
                        mWidgetVideoDisabled.setChecked(false);
                    }
                    KANTVUtils.setDisableVideoTrack(false);
                } else {
                    mWidgetVideoDisabled.setEnabled(true);
                }
                KANTVLog.d(TAG, "disable_video: " + mSettings.getVideoDisabled());
            }

            if (key.contains("pref.disable_video")) {
                KANTVLog.d(TAG, "disable_video: " + mSettings.getVideoDisabled());
                KANTVUtils.setDisableVideoTrack(mSettings.getVideoDisabled());
                if (mSettings.getVideoDisabled()) {
                    if (mWidgetAudioDisabled != null) {
                        mWidgetAudioDisabled.setEnabled(false);
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        key = mContext.getString(R.string.pref_key_disable_audio);
                        editor.putBoolean(key, false);
                        editor.commit();
                        mWidgetAudioDisabled.setChecked(false);
                    }
                    KANTVUtils.setDisableAudioTrack(false);
                } else {
                    mWidgetAudioDisabled.setEnabled(true);
                }
                KANTVLog.d(TAG, "disable_audio: " + mSettings.getAudioDisabled());
            }

            if (key.contains("pref.startplaypos")) {
                String startPlayPosString = mSettings.getStartPlaypos();
                KANTVLog.j(TAG, "start play pos: " + startPlayPosString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(startPlayPosString);
                if( !isNum.matches() ) {
                    KANTVLog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid start time of playback " + startPlayPosString, Toast.LENGTH_LONG).show();
                } else {
                    int startPlayPos = Integer.valueOf(mSettings.getStartPlaypos());
                    KANTVLog.j(TAG, "start play pos: " + startPlayPos);
                    KANTVUtils.setStartPlayPos(startPlayPos);
                }
            }

            if (key.contains("pref.dump_mode")) {
                KANTVLog.j(TAG, "dump mode " + mSettings.getDumpMode());
                KANTVLog.j(TAG, "dev  mode " + mSettings.getDevMode());
                KANTVLog.j(TAG, "dump duration " + KANTVUtils.getDumpDuration());

                if (KANTVUtils.DUMP_MODE_VIDEO_ES_FILE == mSettings.getDumpMode()) {
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_dump_videoes);
                    editor.putBoolean(key, true);
                    editor.commit();
                    KANTVLog.d(TAG, "dump video es " + mSettings.getEnableDumpVideoES());
                    KANTVUtils.setEnableDumpVideoES(mSettings.getEnableDumpVideoES());
                } else {
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_dump_videoes);
                    editor.putBoolean(key, false);
                    editor.commit();
                    KANTVLog.d(TAG, "dump video es " + mSettings.getEnableDumpVideoES());
                    KANTVUtils.setEnableDumpVideoES(mSettings.getEnableDumpVideoES());
                }

                if (KANTVUtils.DUMP_MODE_AUDIO_ES_FILE == mSettings.getDumpMode()) {
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_dump_audioes);
                    editor.putBoolean(key, true);
                    editor.commit();
                    KANTVLog.d(TAG, "dump audio es " + mSettings.getEnableDumpAudioES());
                    KANTVUtils.setEnableDumpAudioES(mSettings.getEnableDumpAudioES());
                } else {
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_dump_audioes);
                    editor.putBoolean(key, false);
                    editor.commit();
                    KANTVLog.d(TAG, "dump audio es " + mSettings.getEnableDumpAudioES());
                    KANTVUtils.setEnableDumpAudioES(mSettings.getEnableDumpAudioES());
                }

                if (KANTVUtils.DUMP_MODE_PERF_DECRYPT_TERMINAL == mSettings.getDumpMode()) {
                    KANTVUtils.createPerfFile("/tmp/kantvplayer_perf_teedecrypt.dat");
                }

                if (KANTVUtils.DUMP_MODE_DATA_DECRYPT_FILE == mSettings.getDumpMode()) {
                    String filename = " ";
                    String filenameInput = " ";
                    if (KANTVUtils.PV_PLAYERENGINE__Exoplayer == mSettings.getPlayerEngine()) {
                        filename = "/tmp/kantvplayer_decrypt_exoplayer.dat";
                        filenameInput = "/tmp/kantvplayer_input_exoplayer.dat";
                    } else if (KANTVUtils.PV_PLAYERENGINE__FFmpeg == mSettings.getPlayerEngine()) {
                        filename = "/tmp/kantvplayer_decrypt_ffmpeg.dat";
                        filenameInput = "/tmp/kantvplayer_input_ffmpeg.dat";
                    } else {
                        filename = "/tmp/kantvplayer_decrypt_androimediaplayer.dat";
                        filenameInput = "/tmp/kantvplayer_input_androimediaplayer.dat";
                    }
                    KANTVUtils.createPerfFile(filename);
                    KANTVUtils.createPerfFile(filenameInput);
                }
                KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
            }

            if (key.contains("pref.dump_duration")) {
                String durationDumpESString = mSettings.getDumpDuration();
                KANTVLog.d(TAG, "duration of dumped es: " + durationDumpESString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(durationDumpESString);
                if( !isNum.matches() ) {
                    KANTVLog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid time " + durationDumpESString, Toast.LENGTH_LONG).show();
                } else {
                    int dumpDuration = Integer.valueOf(mSettings.getDumpDuration());
                    KANTVLog.d(TAG, "dump duration: " + dumpDuration);
                    KANTVUtils.setDumpDuration(dumpDuration);
                    KANTVLog.d(TAG, "dump mode " + mSettings.getDumpMode());
                    KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                    KANTVLog.d(TAG, "dump duration " + KANTVUtils.getDumpDuration());
                    KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                    KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
                }
            }

            if (key.contains("pref.play_mode")) {
                if (mSettings.getPlayMode() != KANTVUtils.PLAY_MODE_NORMAL) {
                    KANTVUtils.perfGetLoopPlayBeginTime();
                }
            }

            if (key.contains("pref.using_ffmpeg_codec")) {
                KANTVLog.d(TAG, "using ffmpeg codec:" + mSettings.getUsingFFmpegCodec());
                KANTVUtils.setUsingFFmpegCodec(mSettings.getUsingFFmpegCodec());
            }


            if (key.contains("pref.playerEngine")) {
                boolean isExoPlayer = false;
                KANTVLog.d(TAG, "dump mode " + mSettings.getDumpMode());
                KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                KANTVLog.d(TAG, "dump duration " + KANTVUtils.getDumpDuration());
                KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());

                AppConfig.getInstance().setPlayerType(mSettings.getPlayerEngine());

                if (mSettings.getPlayerEngine() != KANTVUtils.PV_PLAYERENGINE__Exoplayer) {
                    KANTVLog.j(TAG, "switch to non-Exoplayer engine");
                    isExoPlayer = false;
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
                        } else {
                            KANTVLog.j(TAG, "not running on amlogic box");
                        }
                    }

                    KANTVDRMManager.stop();
                } else {
                    KANTVLog.j(TAG, "switch to Exoplayer engine");
                    isExoPlayer = true;
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

                KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                if (mSettings.getPlayerEngine() != KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
                    if (mWidgetMediaCodecEnabled != null) {
                        //2023-12-25, comment following codes for development feature of "tv recording"
                        //SharedPreferences.Editor editor = mSharedPreferences.edit();
                        //key = mContext.getString(R.string.pref_key_using_media_codec);
                        //editor.putBoolean(key, true);
                        //editor.commit();
                        //mWidgetMediaCodecEnabled.setChecked(true);
                        //mWidgetMediaCodecEnabled.setEnabled(false);
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

                for (String prefKey : keySetOfInvisiblePreferenceOnExoPlayerEnable) {
                    if (findPreference(prefKey) != null)
                        findPreference(prefKey).setVisible(!isExoPlayer);
                }
            }

            if (key.contains("pref.enable_wiseplay")) {
                KANTVUtils.setEnableMultiDRM(mSettings.getEnableWisePlay());
                KANTVUtils.setEnableWisePlay(mSettings.getEnableWisePlay());
                if (mSettings.getEnableWisePlay()) {
                    KANTVLog.j(TAG, "drm scheme          : " + "wiseplay");
                    KANTVUtils.setDrmScheme(KANTVUtils.DRM_SCHEME_WISEPLAY);
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_using_tee);
                    editor.putBoolean(key, true);
                    editor.commit();
                    KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_TEE_SVP);
                    mWidgetTEEEnabled.setChecked(true);
                } else {
                    KANTVLog.j(TAG, "drm scheme          : " + "chinadrm");
                    KANTVUtils.setDrmScheme(KANTVUtils.DRM_SCHEME_CHINADRM);
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_using_tee);
                    editor.putBoolean(key, false);
                    editor.commit();
                    KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
                    mWidgetTEEEnabled.setChecked(false);
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            KANTVDRMManager.start(mContext);
                        }
                    }).start();
                }
                KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDrmScheme(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
            }

            if (key.contains("pixel_format_type")) {
                String value = mSettings.getPixelFormat();
                KANTVLog.d(TAG, "value: " + value);
                for (String type : pixelFormatArrayValue) {
                    if (type.equals(value)) {
                        KANTVLog.d(TAG, "set pixel format to: " + value);
                        AppConfig.getInstance().setPixelFormat(value);
                    }
                }
            }

            if (key.contains("pref.using_media_codec")) {
                boolean bUsingMediaCodec = mSettings.getUsingMediaCodec();
                KANTVLog.d(TAG, "value: " + bUsingMediaCodec);
                AppConfig.getInstance().setOpenMediaCodeC(bUsingMediaCodec);
            }

            if (key.contains("network_subtitle")) {
                if (mSettings.getUsingNetworkSubtitle()) {
                    mWidgetAutoLoadNetwork.setVisible(true);
                } else {
                    mWidgetAutoLoadNetwork.setVisible(false);
                }
            }

            if (key.contains("surface_renders")) {
                AppConfig.getInstance().setSurfaceRenders(mSettings.getUsingSurfaceRenders());
            }

            if (key.contains("pref.using_opensl_es")) {
                AppConfig.getInstance().setOpenSLES(mSettings.getUsingOpenSLES());
            }

            if (key.contains("auto_load_local_subtitle")) {
                AppConfig.getInstance().setAutoLoadLocalSubtitle(mSettings.getAutoLoadLocalSubtitle());
            }

            if (key.contains( "auto_load_network_subtitle")) {
                AppConfig.getInstance().setAutoLoadNetworkSubtitle(mSettings.getAutoLoadNetworkSubtitle());
            }

            if (key.contains("network_subtitle")) {
                AppConfig.getInstance().setUseNetWorkSubtitle(mSettings.getUsingNetworkSubtitle());
            }

            if (key.contains("pref.lang")) {
                mSettings.updateUILang(mActivity);
            }
        }
    };


    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        String key = preference.getKey();

        if (key.contains("pref.dev_mode")) {
            if (mSharedPreferences.getBoolean("pref.dev_mode", false)) {
                if (mWidgetDumpMode != null)
                    mWidgetDumpMode.setEnabled(true);
                if (mWidgetDumpSize != null)
                    mWidgetDumpSize.setEnabled(true);
                if (mWidgetDumpCounts != null)
                    mWidgetDumpCounts.setEnabled(true);
            } else {
                if (mWidgetDumpMode != null)
                    mWidgetDumpMode.setEnabled(false);
                if (mWidgetDumpSize != null)
                    mWidgetDumpSize.setEnabled(false);
                if (mWidgetDumpCounts != null)
                    mWidgetDumpCounts.setEnabled(false);
            }
        }
        return true;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
