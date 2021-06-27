/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package tv.danmaku.ijk.kantv.application;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.util.DisplayMetrics;

import java.util.Locale;

import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.media.player.IjkLog;

public class Settings {
    private static final String TAG = Settings.class.getName();
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;

    public static final int PV_PLAYER__Auto               = 0;
    public static final int PV_PLAYER__AndroidMediaPlayer = 1;
    public static final int PV_PLAYER__IjkMediaPlayer     = 2;
    public static final int PV_PLAYER__IjkExoMediaPlayer  = 3;

    public static final int KANTV_UILANG_AUTO = 0;
    public static final int KANTV_UILANG_CN   = 1;
    public static final int KANTV_UILANG_EN   = 2;

    public Settings(Context context) {
        mAppContext = context.getApplicationContext();
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);
    }

    public boolean getEnableBackgroundPlay() {
        String key = mAppContext.getString(R.string.pref_key_enable_background_play);
        return mSharedPreferences.getBoolean(key, true);
    }

    public int getPlayer() {
        String key = mAppContext.getString(R.string.pref_key_player);
        String value = mSharedPreferences.getString(key, "");
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public int getUILang() {
        String key   = "pref.lang";
        String value = mSharedPreferences.getString(key, "");
        try {
            return Integer.valueOf(value).intValue();
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    public void updateUILang(AppCompatActivity activity) {
        int current_lang = getUILang();
        IjkLog.d(TAG, "user's seletion lang: " + current_lang);

        String lang = "en";
        if (KANTV_UILANG_CN == current_lang)
            lang = "zh";
        else if (KANTV_UILANG_EN == current_lang)
            lang = "en";

        Locale myLocale;
        Resources res = activity.getResources();
        String country = res.getConfiguration().locale.getCountry();
        IjkLog.d(TAG, "current country: " + country);
        DisplayMetrics dm = res.getDisplayMetrics();
        Configuration conf = res.getConfiguration();
        if (KANTV_UILANG_AUTO == current_lang) {
            if ((country == null) || country.isEmpty()) {
                IjkLog.d(TAG, "can't get current country, set lang to chinese");
                myLocale = new Locale("zh");
            } else if (country.contains("CN")) {
                IjkLog.d(TAG, "set lang to chinese");
                myLocale = new Locale("zh");
            } else {
                IjkLog.d(TAG, "set lang to english");
                myLocale = new Locale("en"); //UI language only support Chinese and English at the moment
            }
        } else {
            IjkLog.d(TAG, "lang: " + lang);
            myLocale = new Locale(lang);
        }
        conf.locale = myLocale;
        res.updateConfiguration(conf, dm);
    }

    public boolean getUsingMediaCodec() {
        String key = mAppContext.getString(R.string.pref_key_using_media_codec);
        return mSharedPreferences.getBoolean(key, false);
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

    public boolean getDevMode() {
        String key = mAppContext.getString(R.string.pref_key_dev_mode);
        return mSharedPreferences.getBoolean(key, false);
    }

    public String getLastDirectory() {
        String key = mAppContext.getString(R.string.pref_key_last_directory);
        return mSharedPreferences.getString(key, "/");
    }

    public void setLastDirectory(String path) {
        String key = mAppContext.getString(R.string.pref_key_last_directory);
        mSharedPreferences.edit().putString(key, path).apply();
    }
}
