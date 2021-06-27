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

package tv.danmaku.ijk.kantv.fragments;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceFragmentCompat;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceActivity;
import android.preference.RingtonePreference;
import android.util.DisplayMetrics;

import java.util.Locale;

import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.kantv.activities.SettingsActivity;
import tv.danmaku.ijk.kantv.application.AppActivity;
import tv.danmaku.ijk.kantv.application.Settings;
import tv.danmaku.ijk.media.player.IjkLog;

public class SettingsFragment extends PreferenceFragmentCompat {
    private static final String TAG = SettingsFragment.class.getName();
    private static SettingsActivity mActivity;
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;
    private Settings mSettings;

    public static SettingsFragment newInstance(SettingsActivity activity) {
        SettingsFragment f = new SettingsFragment();
        mActivity = activity;
        return f;
    }

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        mAppContext = mActivity.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mSettings.updateUILang(mActivity);
        addPreferencesFromResource(R.xml.settings);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);
        mSharedPreferences.registerOnSharedPreferenceChangeListener(
                new SharedPreferences.OnSharedPreferenceChangeListener() {
                    @Override
                    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
                        if (key.contains("pref.lang")) {
                            mSettings.updateUILang(mActivity);
                            //TODO:the following two line not works as expected, so UI language will be updated after restart apk manually
                            //android.os.Process.killProcess(android.os.Process.myPid());
                            //System.exit(0);
                        }

                    }
                });
    }
}
