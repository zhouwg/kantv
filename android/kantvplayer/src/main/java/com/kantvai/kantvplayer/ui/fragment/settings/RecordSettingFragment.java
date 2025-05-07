 /*
  * Copyright (c) Project KanTV. 2021-2023
  * Copyright (c) 2024- KanTV Authors
  */
package com.kantvai.kantvplayer.ui.fragment.settings;

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

import com.kantvai.kantvplayer.ui.activities.ShellActivity;
import com.kantvai.kantvplayer.utils.Settings;
import com.kantvai.kantvplayer.R;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


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

        KANTVLog.j(TAG, "dev mode:" + mSettings.getDevMode());

        if (!KANTVUtils.getExpertMode()) {
            KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
            KANTVLog.j(TAG, "record format " + mSettings.getRecordFormat());
            KANTVLog.j(TAG, "record codec " + mSettings.getRecordCodec());
            int recordMode = mSettings.getRecordMode();  //default is both video & audio
            int recordFormat = mSettings.getRecordFormat();//default is mp4
            int recordCodec = mSettings.getRecordFormat(); //default is h264
            if ((recordCodec != 0) && (recordCodec != 1)) {
                SharedPreferences.Editor editor = mSharedPreferences.edit();
                String key = mContext.getString(R.string.pref_key_recordcodec);
                editor.putString(key, "0"); //H264
                editor.commit();
                mWidgetRecordCodec.setValue("0");
                KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
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
            KANTVLog.j(TAG, "key : " + key);
            KANTVLog.j(TAG, "key: " + key);

            KANTVLog.j(TAG, "record format " + mSettings.getRecordFormat());
            KANTVLog.j(TAG, "dev  mode " + mSettings.getDevMode());
            KANTVLog.j(TAG, "record duration " + KANTVUtils.getRecordDuration());
            KANTVLog.j(TAG, "record size " + KANTVUtils.getRecordSize());

            if (key.contains("pref.record_videoes")) {
                KANTVLog.d(TAG, "record video es " + mSettings.getEnableRecordVideoES());
                KANTVUtils.setEnableRecordVideoES(mSettings.getEnableRecordVideoES());
                if (mSettings.getEnableRecordVideoES()) {
                    mWidgetAudioRecord.setEnabled(false);
                } else {
                    mWidgetAudioRecord.setEnabled(true);
                }
            }

            if (key.contains("pref.record_audioes")) {
                KANTVLog.d(TAG, "record audio es " + mSettings.getEnableRecordAudioES());
                KANTVUtils.setEnableRecordAudioES(mSettings.getEnableRecordAudioES());
                if (mSettings.getEnableRecordAudioES()) {
                    mWidgetVideoRecord.setEnabled(false);
                } else {
                    mWidgetVideoRecord.setEnabled(true);
                }
            }

            if (key.contains("pref.record_thresholddisksize")) {
                String recordSizeString = mSettings.getThresholddisksizeString();
                KANTVLog.d(TAG, "threshold size: " + recordSizeString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(recordSizeString);
                if (!isNum.matches()) {
                    KANTVLog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid value " + recordSizeString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mContext.getString(R.string.pref_key_record_thresholddisksize);
                    editor.putString(key, "500");
                    editor.commit();
                    recordSizeString = mSettings.getThresholddisksizeString();
                    KANTVLog.j(TAG, "thresold size: " + recordSizeString);
                } else {
                    KANTVDRM instance = KANTVDRM.getInstance();
                    File sdcardFile = Environment.getExternalStorageDirectory();
                    String sdcardPath = sdcardFile.getAbsolutePath();
                    KANTVLog.j(TAG, "sdcard path:" + sdcardPath);
                    int diskFreeSize = instance.ANDROID_JNI_GetDiskFreeSize(sdcardPath);
                    int diskTotalSize = instance.ANDROID_JNI_GetDiskSize(sdcardPath);
                    KANTVLog.j(TAG, "disk free:" + diskFreeSize + "MB");
                    KANTVLog.j(TAG, "disk total:" + diskTotalSize + "MB");

                    int thresoldsize = Integer.valueOf(mSettings.getThresholddisksize());
                    KANTVUtils.setDiskThresholdFreeSize(thresoldsize);
                    KANTVLog.j(TAG, "thresold disk size:" + thresoldsize + "MBytes");
                }
            }

            if (key.contains("pref.recordformat")) {
                KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
                KANTVLog.j(TAG, "record format " + mSettings.getRecordFormat());
                KANTVLog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264

                KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
            }

            if (key.contains("pref.recordcodec")) {
                KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
                KANTVLog.j(TAG, "record format " + mSettings.getRecordFormat());
                KANTVLog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264
                boolean couldUsingH265Codec = true;
                boolean couldUsingH266Codec = false;
                boolean couldUsingAV1Codec = true;

                KANTVLog.j(TAG, "mSetting.getRecordBenchmark()=" + mSettings.getRecordBenchmark());
                if (mSettings.getRecordBenchmark() >= 12) {
                    couldUsingH265Codec = false;
                    couldUsingH266Codec = false;
                    couldUsingAV1Codec = false;
                }

                KANTVLog.j(TAG, "could using H265:" + couldUsingH265Codec);
                KANTVLog.j(TAG, "could using AV1:" + couldUsingAV1Codec);
                if (!KANTVUtils.getExpertMode()) {
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
                KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
            }

            if (key.contains("pref.recordmode")) {
                KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
                KANTVLog.j(TAG, "record format " + mSettings.getRecordFormat());
                KANTVLog.j(TAG, "record codec " + mSettings.getRecordCodec());
                int recordMode = mSettings.getRecordMode();  //default is both video & audio
                int recordFormat = mSettings.getRecordFormat();//default is mp4
                int recordCodec = mSettings.getRecordCodec(); //default is h264

                KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
            }

            if (key.contains("pref.record_duration")) {
                String durationRecordESString = mSettings.getRecordDurationString();
                KANTVLog.d(TAG, "duration of record es: " + durationRecordESString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(durationRecordESString);
                if (!isNum.matches()) {
                    KANTVLog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid time " + durationRecordESString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mAppContext.getString(R.string.pref_key_record_duration);
                    editor.putString(key, "10"); // 10 minutes
                    editor.commit();
                    durationRecordESString = mSettings.getRecordDurationString();
                    KANTVLog.d(TAG, "duration of record es: " + durationRecordESString);
                } else {
                    int recordDuration = Integer.valueOf(mSettings.getRecordDuration());
                    KANTVLog.d(TAG, "record duration: " + recordDuration);
                    KANTVUtils.setRecordDuration(recordDuration);
                    KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
                    KANTVLog.d(TAG, "record format " + mSettings.getRecordFormat());
                    KANTVLog.d(TAG, "record codec " + mSettings.getRecordCodec());
                    KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                    KANTVLog.d(TAG, "record duration " + KANTVUtils.getRecordDuration());
                    KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                    int recordMode = mSettings.getRecordMode();  //default is both video & audio
                    int recordFormat = mSettings.getRecordFormat();//default is mp4
                    int recordCodec = mSettings.getRecordCodec(); //default is h264

                    KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
                }
            }

            if (key.contains("pref.record_size")) {
                String recordSizeString = mSettings.getRecordSizeString();
                KANTVLog.d(TAG, "record size: " + recordSizeString);
                Pattern pattern = Pattern.compile("[0-9]*");
                Matcher isNum = pattern.matcher(recordSizeString);
                if (!isNum.matches()) {
                    KANTVLog.d(TAG, "user's input is invalid");
                    Toast.makeText(mContext, "invalid value " + recordSizeString, Toast.LENGTH_LONG).show();
                    SharedPreferences.Editor editor = mSharedPreferences.edit();
                    key = mAppContext.getString(R.string.pref_key_record_size);
                    editor.putString(key, "200"); //200 MiB
                    editor.commit();
                    recordSizeString = mSettings.getRecordSizeString();
                    KANTVLog.d(TAG, "record size: " + recordSizeString);
                } else {
                    int recordSize = Integer.valueOf(mSettings.getRecordSize());
                    KANTVLog.d(TAG, "record duration: " + recordSize);
                    KANTVUtils.setRecordSize(recordSize);
                    KANTVLog.j(TAG, "record mode " + mSettings.getRecordMode());
                    KANTVLog.d(TAG, "record format " + mSettings.getRecordFormat());
                    KANTVLog.d(TAG, "record codec " + mSettings.getRecordCodec());
                    KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
                    KANTVLog.d(TAG, "record size " + KANTVUtils.getRecordSize());
                    KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
                    int recordMode = mSettings.getRecordMode();  //default is both video & audio
                    int recordFormat = mSettings.getRecordFormat();//default is mp4
                    int recordCodec = mSettings.getRecordCodec(); //default is h264

                    KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, KANTVUtils.getRecordDuration(), KANTVUtils.getRecordSize());
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
                    KANTVLog.j(TAG, "ANDROID_JNI_Benchmark cost: " + duration + " milliseconds");

                    mActivity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            String benchmarkTip = strBenchmarkInfo + "\n" + "Item " + benchmarkIndex + " cost :" + duration + "milliseconds";
                            String benchmarkTip1 = strBenchmarkInfo + ",Item " + benchmarkIndex + " cost :" + duration + "milliseconds" + ", total time:" + totalDuration + "milliseconds";
                            KANTVLog.j(TAG, benchmarkTip1);
                            if ((mWidgetBenchmarkStatus != null) && (benchmarkIndex <= 10) && (mProgressDialog != null)) {
                                strBenchmarkReport += benchmarkTip + "\n";
                                mWidgetBenchmarkStatus.setTitle(benchmarkTip);
                                mWidgetBenchmarkStatus.setVisible(true);
                                KANTVLog.j(TAG, "benchmark report:" + strBenchmarkReport);
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
                                KANTVLog.j(TAG, "stop benchmark");
                                isBenchmarking.set(false);
                                mProgressDialog.dismiss();
                                mProgressDialog = null;
                                KANTVLog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                                KANTVLog.j(TAG, "benchmark report:" + strBenchmarkReport);
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
                KANTVLog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                KANTVLog.j(TAG, "benchmark report:" + strBenchmarkReport);
                String benchmarkTip1 = "finish " + benchmarkIndex + " items, total cost:" + totalDuration + " milliseconds";
                KANTVLog.j(TAG, benchmarkTip1);
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
            KANTVLog.j(TAG, "pref.recordBenchmark=" + mSettings.getRecordBenchmark());

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
        KANTVLog.j(TAG, benchmarkTip);
        KANTVLog.j(TAG, "single item cost:" + durationPerItem + " seconds");
        KANTVUtils.umRecordBenchmark(benchmarkIndex, totalDuration, durationPerItem);
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

    public static native int kantv_anti_remove_rename_this_file();
}
