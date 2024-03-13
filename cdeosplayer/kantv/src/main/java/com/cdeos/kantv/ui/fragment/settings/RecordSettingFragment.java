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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.CheckBoxPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;

import com.cdeos.kantv.ui.activities.ShellActivity;
import com.cdeos.kantv.utils.Settings;
import com.cdeos.kantv.R;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import cdeos.media.player.KANTVDRM;
import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;


public class RecordSettingFragment extends BaseSettingsFragment {
    private static final String TAG = RecordSettingFragment.class.getName();
    private static ShellActivity mActivity;
    private Context mContext;
    private Context mAppContext;
    private SharedPreferences mSharedPreferences;
    private Settings mSettings;
    private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
    private ProgressDialog mProgressDialog;
    private CheckBoxPreference mWidgetVideoRecord;
    private CheckBoxPreference mWidgetAudioRecord;
    private EditTextPreference mWidgetRecordSize;
    private EditTextPreference mWidgetRecordDuration;
    private IjkListPreference mWidgetRecordCodec;
    private Preference mWidgetBenchmark;
    private Preference mWidgetBenchmarkStatus;
    private long beginTime = 0;
    private long endTime = 0;
    private long duration = 0;
    private int benchmarkIndex = 0;
    private int totalDuration = 0;
    private String strBenchmarkInfo;
    private String strBenchmarkReport;


    @Override
    public String getTitle() {
        return mActivity.getBaseContext().getString(R.string.record_settings);
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

        addPreferencesFromResource(R.xml.settings_recording);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);
        mWidgetVideoRecord = (CheckBoxPreference) findPreference("pref.record_videoes");
        mWidgetAudioRecord = (CheckBoxPreference) findPreference("pref.record_audioes");
        mWidgetRecordCodec = (IjkListPreference) findPreference("pref.recordcodec");

        CDELog.j(TAG, "dev mode:" + mSettings.getDevMode());

        if (!CDEUtils.getExpertMode()) {
            CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
            CDELog.j(TAG, "record format " + mSettings.getRecordFormat());
            CDELog.j(TAG, "record codec " + mSettings.getRecordCodec());
            int recordMode = mSettings.getRecordMode();  //default is both video & audio
            int recordFormat = mSettings.getRecordFormat();//default is mp4
            int recordCodec = mSettings.getRecordFormat(); //default is h264
            if ((recordCodec != 0) && (recordCodec != 1)) {
                SharedPreferences.Editor editor = mSharedPreferences.edit();
                String key = mContext.getString(R.string.pref_key_recordcodec);
                editor.putString(key, "0"); //H264
                editor.commit();
                mWidgetRecordCodec.setValue("0");
                CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
            }
        }

        mWidgetBenchmark = (Preference) findPreference("pref.benchmark");
        mWidgetBenchmarkStatus = (Preference) findPreference("pref.benchmarkinfo");
        if (mWidgetBenchmark != null) {
            mWidgetBenchmarkStatus.setVisible(false);
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
            CDELog.j(TAG, "key : " + key);
            CDELog.j(TAG, "key: " + key);

            CDELog.j(TAG, "record format " + mSettings.getRecordFormat());
            CDELog.j(TAG, "dev  mode " + mSettings.getDevMode());
            CDELog.j(TAG, "record duration " + CDEUtils.getRecordDuration());
            CDELog.j(TAG, "record size " + CDEUtils.getRecordSize());

            if (key.contains("pref.record_videoes")) {
                CDELog.d(TAG, "record video es " + mSettings.getEnableRecordVideoES());
                CDEUtils.setEnableRecordVideoES(mSettings.getEnableRecordVideoES());
                if (mSettings.getEnableRecordVideoES()) {
                    mWidgetAudioRecord.setEnabled(false);
                } else {
                    mWidgetAudioRecord.setEnabled(true);
                }
            }

            if (key.contains("pref.record_audioes")) {
                CDELog.d(TAG, "record audio es " + mSettings.getEnableRecordAudioES());
                CDEUtils.setEnableRecordAudioES(mSettings.getEnableRecordAudioES());
                if (mSettings.getEnableRecordAudioES()) {
                    mWidgetVideoRecord.setEnabled(false);
                } else {
                    mWidgetVideoRecord.setEnabled(true);
                }
            }

            if (key.contains("pref.record_thresholddisksize")) {
                String recordSizeString = mSettings.getThresholddisksizeString();
                CDELog.d(TAG, "threshold size: " + recordSizeString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(recordSizeString);
                if (!isNum.matches()) {
                    CDELog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid value " + recordSizeString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_record_thresholddisksize);
                    editor.putString(key, "500");
                    editor.commit();
                    recordSizeString = mSettings.getThresholddisksizeString();
                    CDELog.j(TAG, "thresold size: " + recordSizeString);
                } else {
                    KANTVDRM instance = KANTVDRM.getInstance();
                    File sdcardFile = Environment.getExternalStorageDirectory();
                    String sdcardPath = sdcardFile.getAbsolutePath();
                    CDELog.j(TAG, "sdcard path:" + sdcardPath);
                    int diskFreeSize = instance.ANDROID_JNI_GetDiskFreeSize(sdcardPath);
                    int diskTotalSize = instance.ANDROID_JNI_GetDiskSize(sdcardPath);
                    CDELog.j(TAG, "disk free:" + diskFreeSize + "MB");
                    CDELog.j(TAG, "disk total:" + diskTotalSize + "MB");

                    int thresoldsize = Integer.valueOf(mSettings.getThresholddisksize());
                    CDEUtils.setDiskThresholdFreeSize(thresoldsize);
                    CDELog.j(TAG, "thresold disk size:" + thresoldsize + "MBytes");
                }
            }

            if (key.contains("pref.recordformat")) {
                CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
                CDELog.j(TAG, "record format " + mSettings.getRecordFormat());
                CDELog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264

                CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
            }

            if (key.contains("pref.recordcodec")) {
                CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
                CDELog.j(TAG, "record format " + mSettings.getRecordFormat());
                CDELog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264
                boolean couldUsingH265Codec = true;
                boolean couldUsingH266Codec = false;
                boolean couldUsingAV1Codec = true;

                CDELog.j(TAG, "mSetting.getRecordBenchmark()=" + mSettings.getRecordBenchmark());
                if (mSettings.getRecordBenchmark() >= 12) {
                    couldUsingH265Codec = false;
                    couldUsingH266Codec = false;
                    couldUsingAV1Codec = false;
                }

                CDELog.j(TAG, "could using H265:" + couldUsingH265Codec);
                CDELog.j(TAG, "could using AV1:" + couldUsingAV1Codec);
                if (!CDEUtils.getExpertMode()) {
                    if ((recordCodec != 0) && (recordCodec != 1)) {
                        SharedPreferences.Editor editor = mSharedPreferences.edit();
                        key = mContext.getString(R.string.pref_key_recordcodec);
                        editor.putString(key, "0"); //H264
                        editor.commit();
                        mWidgetRecordCodec.setValue("0");
                        Toast.makeText(mContext, "only H264/H265 is available", Toast.LENGTH_LONG).show();
                        return;
                    }
                }
                CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
            }

            if (key.contains("pref.recordmode")) {
                CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
                CDELog.j(TAG, "record format " + mSettings.getRecordFormat());
                CDELog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264

                CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
            }

            if (key.contains("pref.record_duration")) {
                String durationRecordESString = mSettings.getRecordDurationString();
                CDELog.d(TAG, "duration of record es: " + durationRecordESString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(durationRecordESString);
                if (!isNum.matches()) {
                    CDELog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid time " + durationRecordESString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mAppContext.getString(R.string.pref_key_record_duration);
                    editor.putString(key, "3");
                    editor.commit();
                    durationRecordESString = mSettings.getRecordDurationString();
                    CDELog.d(TAG, "duration of record es: " + durationRecordESString);
                } else {
                    int recordDuration = Integer.valueOf(mSettings.getRecordDuration());
                    CDELog.d(TAG, "record duration: " + recordDuration);
                    CDEUtils.setRecordDuration(recordDuration);
                    CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
                    CDELog.d(TAG, "record format " + mSettings.getRecordFormat());
                    CDELog.d(TAG, "record codec " + mSettings.getRecordCodec());
                    CDELog.d(TAG, "dev  mode " + mSettings.getDevMode());
                    CDELog.d(TAG, "record duration " + CDEUtils.getRecordDuration());
                    CDELog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                    int recordMode = mSettings.getRecordMode();  //default is both video & audio
                    int recordFormat = mSettings.getRecordFormat();//default is mp4
                    int recordCodec = mSettings.getRecordCodec(); //default is h264

                    CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
                }
            }

            if (key.contains("pref.record_size")) {
                String recordSizeString = mSettings.getRecordSizeString();
                CDELog.d(TAG, "record size: " + recordSizeString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(recordSizeString);
                if (!isNum.matches()) {
                    CDELog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid value " + recordSizeString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mAppContext.getString(R.string.pref_key_record_size);
                    editor.putString(key, "20");
                    editor.commit();
                    recordSizeString = mSettings.getRecordSizeString();
                    CDELog.d(TAG, "record size: " + recordSizeString);
                } else {
                    int recordSize = Integer.valueOf(mSettings.getRecordSize());
                    CDELog.d(TAG, "record duration: " + recordSize);
                    CDEUtils.setRecordSize(recordSize);
                    CDELog.j(TAG, "record mode " + mSettings.getRecordMode());
                    CDELog.d(TAG, "record format " + mSettings.getRecordFormat());
                    CDELog.d(TAG, "record codec " + mSettings.getRecordCodec());
                    CDELog.d(TAG, "dev  mode " + mSettings.getDevMode());
                    CDELog.d(TAG, "record size " + CDEUtils.getRecordSize());
                    CDELog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                    int recordMode = mSettings.getRecordMode();  //default is both video & audio
                    int recordFormat = mSettings.getRecordFormat();//default is mp4
                    int recordCodec = mSettings.getRecordCodec(); //default is h264

                    CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, CDEUtils.getRecordDuration(), CDEUtils.getRecordSize());
                }
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

        return true;
    }

    private final void launchBenchmarkThread() {

        Thread workThread = new Thread(new Runnable() {
            @Override
            public void run() {
                KANTVDRM instance = KANTVDRM.getInstance();

                while (isBenchmarking.get()) {
                    if (benchmarkIndex > 10) {
                        benchmarkIndex = 10;
                        break;
                    }

                    beginTime = System.currentTimeMillis();
                    switch (benchmarkIndex) {
                        case 0:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(0, 1920, 1080, 1920, 1080);
                            break;
                        case 1:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(0, 1280, 720, 1920, 1080);
                            break;
                        case 2:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(0, 1920, 1080, 1280, 720);
                            break;
                        case 3:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(0, 1920, 1080, 960, 540);
                            break;
                        case 4:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(0, 1920, 1080, 640, 360);
                            break;
                        default:
                            strBenchmarkInfo = instance.ANDROID_JNI_Benchmark(benchmarkIndex, 1920, 1080, 1920, 1080);
                    }
                    benchmarkIndex++;
                    endTime = System.currentTimeMillis();
                    duration = (endTime - beginTime);
                    totalDuration += duration;
                    CDELog.j(TAG, "ANDROID_JNI_Benchmark cost: " + duration + " milliseconds");

                    mActivity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            String benchmarkTip = strBenchmarkInfo + "\n" + "Item " + benchmarkIndex + " cost :" + duration + "milliseconds";
                            String benchmarkTip1 = strBenchmarkInfo + ",Item " + benchmarkIndex + " cost :" + duration + "milliseconds" + ", total time:" + totalDuration + "milliseconds";
                            CDELog.j(TAG, benchmarkTip1);
                            if ((mWidgetBenchmarkStatus != null) && (benchmarkIndex <= 10) && (mProgressDialog != null)) {
                                strBenchmarkReport += benchmarkTip + "\n";
                                mWidgetBenchmarkStatus.setTitle(benchmarkTip);
                                mWidgetBenchmarkStatus.setVisible(true);
                                CDELog.j(TAG, "benchmark report:" + strBenchmarkReport);
                            }
                        }
                    });


                }
                stopUIBuffering();
            }
        });
        workThread.start();

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
                    mProgressDialog.setCanceledOnTouchOutside(true);

                    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialogInterface) {
                            if (mProgressDialog != null) {
                                CDELog.j(TAG, "stop benchmark");
                                isBenchmarking.set(false);
                                mProgressDialog.dismiss();
                                mProgressDialog = null;
                                CDELog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                                CDELog.j(TAG, "benchmark report:" + strBenchmarkReport);
                                mWidgetBenchmarkStatus.setVisible(false);
                                displayBenchmarkResult();
                            }
                        }
                    });
                    mProgressDialog.show();
                }
            }
        });
    }


    private void stopUIBuffering() {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                    mProgressDialog = null;

                    mWidgetBenchmarkStatus.setVisible(false);
                    Toast.makeText(mContext, mContext.getString(R.string.benchmark_stop), Toast.LENGTH_SHORT).show();

                    displayBenchmarkResult();
                }
                CDELog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                CDELog.j(TAG, "benchmark report:" + strBenchmarkReport);
                String benchmarkTip1 = "finish " + benchmarkIndex + " items, total cost:" + totalDuration + " milliseconds";
                CDELog.j(TAG, benchmarkTip1);
                mWidgetBenchmark.setEnabled(true);
            }
        });
    }

    private void displayBenchmarkResult() {
        String benchmarkTip;
        int durationPerItem = 0;
        int durations = 0;
        if (totalDuration == 0) {
            benchmarkTip = "you canceled the benchmark";
        } else {
            benchmarkTip = "you finished " + benchmarkIndex + " items" + "\n" + "total cost " + totalDuration + "milliseconds";
            durations = totalDuration / 1000;

            if (benchmarkIndex > 0)
                durationPerItem = durations / benchmarkIndex;

            benchmarkTip += "\n" + "single item cost " + durationPerItem + " seconds";
            SharedPreferences.Editor editor = mSharedPreferences.edit();
            editor.putString("pref.recordBenchmark", String.valueOf(durationPerItem));
            editor.commit();
            CDELog.j(TAG, "pref.recordBenchmark=" + mSettings.getRecordBenchmark());

            if (durationPerItem <= 1) {
                benchmarkTip += "\n" + "performance of your phone seems good";
            } else if ((durationPerItem >= 2) && (durationPerItem < 4)) {
                benchmarkTip += "\n" + "performance of your phone seems well";
            } else if ((durationPerItem >= 4) && (durationPerItem < 6)) {
                benchmarkTip += "\n" + "performance of your phone seems very good";
            } else if ((durationPerItem >= 6) && (durationPerItem < 9)) {
                benchmarkTip += "\n" + "performance of your phone seems not good";
            } else if ((durationPerItem >= 9) && (durationPerItem < 12)) {
                benchmarkTip += "\n" + "performance of your phone seems a little slow";
            } else if (durationPerItem >= 12) {
                benchmarkTip += "\n" + "performance of your phone seems very slow";
            }

            benchmarkTip += "\n\n";

        }
        CDELog.j(TAG, benchmarkTip);
        CDELog.j(TAG, "single item cost:" + durationPerItem + " seconds");
        CDEUtils.umRecordBenchmark(benchmarkIndex, totalDuration, durationPerItem);
        showBenchmarkResult(mActivity, benchmarkTip);
    }

    private void showBenchmarkResult(Context context, String benchmarkInfo) {
        new AlertDialog.Builder(context)
                .setTitle("result of performance benchmark")
                .setMessage(benchmarkInfo)
                .setCancelable(true)
                .setNegativeButton(context.getString(R.string.OK),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        })
                .show();
    }

    public static native int kantv_anti_tamper();
}
