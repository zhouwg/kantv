/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
 * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
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

package com.kantvai.kantvplayer.utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.preference.PreferenceManager;

import androidx.appcompat.app.AppCompatActivity;

import android.util.DisplayMetrics;

import java.util.Locale;

import com.kantvai.kantvplayer.R;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


public class Settings {
    private static final String TAG = Settings.class.getName();
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;

    public static final int KANTV_UILANG_CN = 1;
    public static final int KANTV_UILANG_EN = 0;

    public Settings(Context context) {
        mAppContext = context.getApplicationContext();
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);
    }

    public boolean getEnableBackgroundPlay() {
        String key = mAppContext.getString(R.string.pref_key_enable_background_play);
        return mSharedPreferences.getBoolean(key, false);
    }

    public int getPlayerEngine() {
        String key = mAppContext.getString(R.string.pref_key_playerengine);
        String value = mSharedPreferences.getString(key, "0"); //default is ffmpeg
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 0;
        }
    }

    public int getASRMode() {
        String key = mAppContext.getString(R.string.pref_key_asrmode);
        String value = mSharedPreferences.getString(key, "0"); //normal transcription
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 0;
        }
    }

    public int getASRThreadCounts() {
        String key = mAppContext.getString(R.string.pref_key_asrthreadcounts);
        String value = mSharedPreferences.getString(key, "3"); // actual thread counts is 3 + 1 = 4
        try {
            return Integer.valueOf(value).intValue() + 1;
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 4;
        }
    }


    public int getASRModel() {
        String key = mAppContext.getString(R.string.pref_key_asrmodel);
        String value = mSharedPreferences.getString(key, "3"); //tiny.en-q8_0
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 3;
        }
    }


    public int getUILang() {
        String key = "pref.lang";
        String value = mSharedPreferences.getString(key, "0");
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public void updateUILang(AppCompatActivity activity) {
        int current_lang = getUILang();
        KANTVLog.d(TAG, "user's selection lang: " + current_lang);
        String lang = "en";
        if (KANTV_UILANG_CN == current_lang)
            lang = "zh";
        else if (KANTV_UILANG_EN == current_lang)
            lang = "en";
        else
            lang = "en";

        Locale myLocale;
        Resources res = activity.getResources();
        String country = res.getConfiguration().locale.getCountry();
        KANTVLog.d(TAG, "current country: " + country);
        DisplayMetrics dm = res.getDisplayMetrics();
        Configuration conf = res.getConfiguration();
        if (KANTV_UILANG_CN == current_lang) {
            if ((country == null) || country.isEmpty()) {
                KANTVLog.j(TAG, "can't get current country, set lang to english");
                myLocale = new Locale("en");
            } else if (country.contains("CN")) {
                KANTVLog.j(TAG, "set lang to chinese");
                myLocale = new Locale("zh");
            } else {
                KANTVLog.j(TAG, "set lang to english");
                myLocale = new Locale("en"); //UI language only support Chinese and English at the moment
            }
        } else {
            KANTVLog.d(TAG, "lang: " + lang);
            myLocale = new Locale(lang);
        }
        conf.locale = myLocale;
        res.updateConfiguration(conf, dm);
    }

    public boolean getUsingMediaCodec() {
        String key = mAppContext.getString(R.string.pref_key_using_media_codec);
        return mSharedPreferences.getBoolean(key, false);//改为false便于ffmpeg录制
    }

    public boolean getUsingFFmpegCodec() {
        String key = mAppContext.getString(R.string.pref_key_using_ffmpeg_codec);
        return mSharedPreferences.getBoolean(key, true);
    }

    public boolean getUsingMediaCodecAutoRotate() {
        String key = mAppContext.getString(R.string.pref_key_using_media_codec_auto_rotate);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getMediaCodecHandleResolutionChange() {
        String key = mAppContext.getString(R.string.pref_key_media_codec_handle_resolution_change);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getUsingOpenSLES() {
        String key = mAppContext.getString(R.string.pref_key_using_opensl_es);
        return mSharedPreferences.getBoolean(key, false);
    }

    public String getPixelFormat() {
        String key = mAppContext.getString(R.string.pref_key_pixel_format);
        return mSharedPreferences.getString(key, "");
    }

    public boolean getUsingSurfaceRenders() {
        String key = "surface_renders";
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getEnableNoView() {
        String key = mAppContext.getString(R.string.pref_key_enable_no_view);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getEnableSurfaceView() {
        String key = mAppContext.getString(R.string.pref_key_enable_surface_view);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getEnableTextureView() {
        String key = mAppContext.getString(R.string.pref_key_enable_texture_view);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getEnableDetachedSurfaceTextureView() {
        String key = mAppContext.getString(R.string.pref_key_enable_detached_surface_texture);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getUsingMediaDataSource() {
        String key = mAppContext.getString(R.string.pref_key_using_mediadatasource);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getRtspUseTcp() {
        String key = mAppContext.getString(R.string.pref_key_rtsp_tcp);
        return mSharedPreferences.getBoolean(key, true);
    }

    public boolean getContinuedPlayback() {
        String key = mAppContext.getString(R.string.pref_key_continued_playback);

        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getTEEEnabled() {
        String key = mAppContext.getString(R.string.pref_key_using_tee);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getAudioDisabled() {
        String key = mAppContext.getString(R.string.pref_key_disable_audio);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getVideoDisabled() {
        String key = mAppContext.getString(R.string.pref_key_disable_video);
        return mSharedPreferences.getBoolean(key, false);
    }

    public String getStartPlaypos() {
        String key = mAppContext.getString(R.string.pref_key_startplaypos);
        return mSharedPreferences.getString(key, "0");
    }

    public boolean getEPGEnabled() {
        String key = mAppContext.getString(R.string.pref_key_epgenabled);
        return mSharedPreferences.getBoolean(key, true);
    }

    public String getKANTVServer() {
        String key = mAppContext.getString(R.string.pref_key_kantvserver);
        return mSharedPreferences.getString(key, KANTVUtils.getKANTVServer());
    }

    public String getNginxServerUrl() {
        String key = mAppContext.getString(R.string.pref_key_nginx);
        return mSharedPreferences.getString(key, KANTVUtils.getNginxServerUrl());
    }

    public String getApiGatewayServerUrl() {
        String key = mAppContext.getString(R.string.pref_key_apigateway);
        return mSharedPreferences.getString(key, KANTVUtils.getApiGatewayServerUrl());
    }

    public boolean getDevMode() {
        String key = mAppContext.getString(R.string.pref_key_dev_mode);
        return mSharedPreferences.getBoolean(key, false);
    }

    public int getDumpMode() {
        String key = mAppContext.getString(R.string.pref_key_dump_mode);
        String value = mSharedPreferences.getString(key, "0");
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public int getRecordFormat() {
        String key = mAppContext.getString(R.string.pref_key_recordformat);
        String value = mSharedPreferences.getString(key, "0"); //ts
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }
    public int getRecordCodec() {
        String key = mAppContext.getString(R.string.pref_key_recordcodec);
        String value = mSharedPreferences.getString(key, "0"); //h264
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }
    public int getRecordMode() {
        String key = mAppContext.getString(R.string.pref_key_recordmode);
        String value = mSharedPreferences.getString(key, "0"); //default is both video & audio
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public int getRecordBenchmark() {
        String key = "pref.recordBenchmark";
        String value = mSharedPreferences.getString(key, "10");
        try {
            return Integer.valueOf(value).intValue();
        }  catch (NumberFormatException e) {
            return 0;
        }
    }

    public boolean getEnableRecordVideoES() {
        String key = mAppContext.getString(R.string.pref_key_record_videoes);
        return mSharedPreferences.getBoolean(key, true);
    }

    public boolean getEnableRecordAudioES() {
        String key = mAppContext.getString(R.string.pref_key_record_audioes);
        return mSharedPreferences.getBoolean(key, false);
    }

    public String getRecordDurationString() {
        String key = mAppContext.getString(R.string.pref_key_record_duration);
        String defaultDuration = String.valueOf(KANTVUtils.getRecordDuration());
        String value = mSharedPreferences.getString(key, defaultDuration); //per minutes
        return value;
    }

    public int getRecordDuration() {
        String key = mAppContext.getString(R.string.pref_key_record_duration);
        String defaultDuration = String.valueOf(KANTVUtils.getRecordDuration());
        String value = mSharedPreferences.getString(key, defaultDuration); //per minutes
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 1; //default 1 minutes
        }
    }

    public String getRecordSizeString() {
        String key = mAppContext.getString(R.string.pref_key_record_size);
        String defaultSize = String.valueOf(KANTVUtils.getRecordSize());
        String value = mSharedPreferences.getString(key, defaultSize); //per Mbytes
        return value;
    }

    public int getRecordSize() {
        String key = mAppContext.getString(R.string.pref_key_record_size);
        String defaultSize = String.valueOf(KANTVUtils.getRecordSize());
        String value = mSharedPreferences.getString(key, defaultSize); //per Mbytes
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 5; //default 5Mb
        }
    }

    public String getThresholddisksizeString() {
        String key = mAppContext.getString(R.string.pref_key_record_thresholddisksize);
        String thresholdSize = String.valueOf(KANTVUtils.getDiskThresholdFreeSize());
        String value = mSharedPreferences.getString(key, thresholdSize); //per Mbytes
        return value;
    }

    public int getThresholddisksize() {
        String key = mAppContext.getString(R.string.pref_key_record_thresholddisksize);
        String thresholdSize = String.valueOf(KANTVUtils.getDiskThresholdFreeSize());
        String value = mSharedPreferences.getString(key, thresholdSize); //per Mbytes
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 500; //default 500Mb
        }
    }

    public int getPlayMode() {
        String key = mAppContext.getString(R.string.pref_key_play_mode);
        String value = mSharedPreferences.getString(key, "0");
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public boolean getEnableDumpVideoES() {
        String key = mAppContext.getString(R.string.pref_key_dump_videoes);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getEnableDumpAudioES() {
        String key = mAppContext.getString(R.string.pref_key_dump_audioes);
        return mSharedPreferences.getBoolean(key, false);
    }

    public String getDumpDuration() {
        String key = mAppContext.getString(R.string.pref_key_dump_duration);
        return mSharedPreferences.getString(key, "10"); //per seconds
    }

    public String getDumpSize() {
        String key = mAppContext.getString(R.string.pref_key_dump_size);
        return mSharedPreferences.getString(key, "1024"); //per kbytes
    }

    public String getDumpCounts() {
        String key = mAppContext.getString(R.string.pref_key_dump_counts);
        return mSharedPreferences.getString(key, "1000");
    }

    public String getLastDirectory() {
        String key = mAppContext.getString(R.string.pref_key_last_directory);
        return mSharedPreferences.getString(key, "/sdcard");
    }

    public void setLastDirectory(String path) {
        String key = mAppContext.getString(R.string.pref_key_last_directory);
        mSharedPreferences.edit().putString(key, path).apply();
    }

    public String getRTMPServerUrl() {
        String key = mAppContext.getString(R.string.pref_key_rtmpserver);
        return mSharedPreferences.getString(key, "rtmp://www.kantvai.com/live/livestream");
    }

    public boolean getEnableWisePlay() {
        String key = mAppContext.getString(R.string.pref_key_enable_wiseplay);
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean isCloseSplashPage() {
        String key = "close_splash_page";
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getUsingNetworkSubtitle() {
        String key = "network_subtitle";
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getAutoLoadNetworkSubtitle() {
        String key = "auto_load_network_subtitle";
        return mSharedPreferences.getBoolean(key, false);
    }

    public boolean getAutoLoadLocalSubtitle() {
        String key = "auto_load_local_subtitle";
        return mSharedPreferences.getBoolean(key, true);
    }

    public int getLLMbackend() {
        String key = mAppContext.getString(R.string.pref_key_backend);
        String value = mSharedPreferences.getString(key, "1"); //actual backend is 1 + 3 = 4
        try {
            return Integer.valueOf(value).intValue() + 3; //skip QNN-CPU,QNN-GPU,QNN-NPU
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 4;
        }
    }

    public int getLLMThreadCounts() {
        String key = mAppContext.getString(R.string.pref_key_llmthreadcounts);
        String value = mSharedPreferences.getString(key, "3"); // actual thread counts is 3 + 1 = 4
        try {
            return Integer.valueOf(value).intValue() + 1;
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 4;
        }
    }

    public int getLLMModel() {
        String key = mAppContext.getString(R.string.pref_key_llmmodel);
        String value = mSharedPreferences.getString(key, "4"); //Gemma3-4B
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            KANTVLog.j(TAG, "exception occurred");
            return 4;
        }
    }

    public boolean getHexagonEnabled() {
        return true;
    }

}
