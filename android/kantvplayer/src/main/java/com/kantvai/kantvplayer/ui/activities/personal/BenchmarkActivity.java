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

package com.kantvai.kantvplayer.ui.activities.personal;
import static kantvai.media.player.KANTVBenchmarkType.AV_PIX_FMT_YUV420P;
import static kantvai.media.player.KANTVBenchmarkType.BENCHMARK_AOM_AV1;
import static kantvai.media.player.KANTVBenchmarkType.BENCHMARK_H264;
import static kantvai.media.player.KANTVBenchmarkType.BENCHMARK_H265;
import static kantvai.media.player.KANTVBenchmarkType.BENCHMARK_H266;
import static kantvai.media.player.KANTVBenchmarkType.BENCHMARK_SVT_AV1;
import static kantvai.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_INFO;
import static kantvai.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_START;
import static kantvai.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_STOP;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Build;
import android.preference.PreferenceManager;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import com.kantvai.kantvplayer.base.BaseMvcActivity;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.Settings;


import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicBoolean;

import kantvai.media.player.KANTVEvent;
import kantvai.media.player.KANTVEventListener;
import kantvai.media.player.KANTVEventType;
import kantvai.media.player.KANTVException;
import kantvai.media.player.KANTVMgr;
import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


public class BenchmarkActivity extends BaseMvcActivity {
    private static final String TAG = BenchmarkActivity.class.getName();

    private KANTVMgr mKANTVMgr = null;
    private BenchmarkActivity.MyEventListener mEventListener = new BenchmarkActivity.MyEventListener();

    private TextView _tvPageInfo;

    private TextView _tvEncodeBenchmarkInfo;
    private TextView _tvSynthesisBenchmarkInfo;

    private Button _btnGraphicBenchmark;
    private Button _btnEncodeBenchmark;
    private Button _btnSynthesisBenchmark;

    private CheckBox _checkEnablefilter;


    private Context mContext;
    private Activity mActivity;
    private Context mAppContext;
    private Settings mSettings;
    private SharedPreferences mSharedPreferences;

    private long beginTime = 0;
    private long endTime = 0;
    private long duration = 0;
    private int benchmarkIndex = 0;
    private int totalDuration = 0;
    private String strBenchmarkInfo;
    private String strBenchmarkReport;
    private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
    private ProgressDialog mProgressDialog;

    private final int[] encodeformatToChoose = {
            0/*h264*/,
            1/*h265*/,
            2/*h266*/,
            3/*intel av1*/,
            4/*google av1*/

    };
    private final int[] encodeResolutionToChoose = {
            0/*352  x 288*/,
            1/*640  x 480*/,
            2/*720  x 576*/,
            3/*1280 x 720 */,
            4/*1920 x 1080 */,
            5/*2560 x 1440 */,
            6/*3840 x 2160 */,
            7/*7680 x 4320 */
    };
    private final int[] encodeFilterToChoose = {
            0, /* save to file */
            1  /* don't save to file */
    };
    private int mEncodeID = 0; //h264;
    private int mResolutionID = 0; //352 x 288
    private int mEncodePattern = 0;
    private int mFilterID = 0; //don't save to file

    private FrameLayout flEncodeBenchmark;


    private Vector<RelativeLayout.LayoutParams> vectorLPStatus = new Vector<RelativeLayout.LayoutParams>();
    private Vector<RelativeLayout.LayoutParams> vectorLPHint = new Vector<RelativeLayout.LayoutParams>();
    private Vector<RelativeLayout.LayoutParams> vectorLPSurface = new Vector<RelativeLayout.LayoutParams>();
    private Map<Integer, SurfaceView> mapSV = new HashMap<Integer, SurfaceView>();
    private Map<Integer, Surface> mapSurfaceMap = new HashMap<Integer, Surface>();

    private SurfaceView objSurfaceView;
    private SurfaceHolder surfaceHolder;

    private int SURFACE_WIDTH = 640;
    private int SURFACE_HEIGHT = 480;

    private boolean _bPreviewing = false;
    private boolean _bEnableFilter = false;
    LinearLayout layout;


    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_custom_benchmark;
    }

    @Override
    public void initPageViewListener() {

    }

    @Override
    public void initPageView() {
        setTitle("Performance benchmark");
        long beginTime = 0;
        long endTime = 0;
        beginTime = System.currentTimeMillis();

        mActivity = this;
        mContext = mActivity.getBaseContext();
        mSettings = new Settings(mContext);

        initView();

        endTime = System.currentTimeMillis();
        KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
    }


    public void initView() {
        long beginTime = 0;
        long endTime = 0;
        beginTime = System.currentTimeMillis();

        mContext = mActivity.getBaseContext();
        mAppContext = mActivity.getApplicationContext();
        mSettings = new Settings(mContext);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);
        Resources res = mActivity.getResources();

        DisplayMetrics dm = new DisplayMetrics();
        mActivity.getWindowManager().getDefaultDisplay().getMetrics(dm);
        int screenWidth = dm.widthPixels;
        int screenHeight = dm.heightPixels;

        _tvPageInfo = mActivity.findViewById(R.id.pageinfo);

        _btnGraphicBenchmark = mActivity.findViewById(R.id.btnGraphicBenchmark);
        _btnEncodeBenchmark = mActivity.findViewById(R.id.btnEncodeBenchmark);
        _btnSynthesisBenchmark = mActivity.findViewById(R.id.btnSynthesisBenchmark);

        _tvSynthesisBenchmarkInfo = (TextView) mActivity.findViewById(R.id.tvEncodeBenchmarkInfo);
        _tvEncodeBenchmarkInfo = (TextView) mActivity.findViewById(R.id.tvEncodeBenchmarkInfo);

        _tvEncodeBenchmarkInfo.setSingleLine(false);
        _tvEncodeBenchmarkInfo.setMaxLines(5);
        _tvEncodeBenchmarkInfo.setGravity(Gravity.LEFT);

        _tvSynthesisBenchmarkInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);

        //if (mSettings.getUILang() == Settings.KANTV_UILANG_EN)
        {
            String info = "performance benchmark consist of: graphic benchmark, CPU, video encode <br>";
            addBuildField("BOARD", Build.BOARD);
            addBuildField("BOOTLOADER", Build.BOOTLOADER);
            addBuildField("BRAND", Build.BRAND);
            addBuildField("CPU_ABI", Build.CPU_ABI);
            addBuildField("DEVICE", Build.DEVICE);
            addBuildField("DISPLAY", Build.DISPLAY);
            addBuildField("FINGERPRINT", Build.FINGERPRINT);
            addBuildField("HARDWARE", Build.HARDWARE);
            addBuildField("HOST", Build.HOST);
            addBuildField("ID", Build.ID);
            addBuildField("MANUFACTURER", Build.MANUFACTURER);
            addBuildField("MODEL", Build.MODEL);
            addBuildField("PRODUCT", Build.PRODUCT);
            addBuildField("SERIAL", Build.SERIAL);
            addBuildField("TAGS", Build.TAGS);
            addBuildField("TYPE", Build.TYPE);
            addBuildField("USER", Build.USER);
            addBuildField("ANDROID SDK", String.valueOf(android.os.Build.VERSION.SDK_INT));
            addBuildField("OS Version", android.os.Build.VERSION.RELEASE);
            info += "<br>"
                    + "Brand:" + Build.BRAND + "<br>"
                    + "CPU:" + Build.CPU_ABI + "<br>"
                    + "Hardware:" + Build.HARDWARE + "<br>"
                    /*+ "Fingerprint:" + Build.FINGERPRINT + "<br>" */
                    + "OS:" + "Android " + android.os.Build.VERSION.RELEASE;
            _tvPageInfo.setText(Html.fromHtml(info));
            //_tvPageInfo.setMovementMethod(LinkMovementMethod.getInstance());

            _tvSynthesisBenchmarkInfo.setText("");
            _tvSynthesisBenchmarkInfo.setMovementMethod(LinkMovementMethod.getInstance());
        }


        Spinner spinner = mActivity.findViewById(R.id.spinnerEncode);
        String[] sentences = getResources().getStringArray(R.array.encodeFormat);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, sentences);
        spinner.setAdapter(adapter);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                KANTVLog.j(TAG, "position=" + position);
                if (position >= 0) {
                    mEncodeID = encodeformatToChoose[position];
                    KANTVLog.j(TAG, "encode id:" + mEncodeID);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });


        Spinner spinnerResolution = mActivity.findViewById(R.id.spinnerResolution);
        String[] sentencesResolution = getResources().getStringArray(R.array.encodeResolution);
        ArrayAdapter<String> adapterResolution = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, sentencesResolution);
        spinnerResolution.setAdapter(adapterResolution);
        spinnerResolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                KANTVLog.j(TAG, "position=" + position);
                if (position >= 0) {
                    mResolutionID = encodeResolutionToChoose[position];
                    KANTVLog.j(TAG, "resolution id:" + mResolutionID);

                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });


        _checkEnablefilter = mActivity.findViewById(R.id.chkEnableFilter);
        _checkEnablefilter.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                KANTVLog.j(TAG, "isChecked:" + isChecked);
                _bEnableFilter = isChecked;
                KANTVLog.j(TAG, "_bEnableFilter:" + _bEnableFilter);
            }
        });

        flEncodeBenchmark = mActivity.findViewById(R.id.flEncodeBenchmark);


        _tvPageInfo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            }
        });


        _btnGraphicBenchmark.setOnClickListener(v -> {
            KANTVLog.j(TAG, "start graphic benchmark");

            launchActivity(KanTVBenchmarkActivity.class);
        });


        _btnSynthesisBenchmark.setOnClickListener(v -> {
            KANTVLog.j(TAG, "start simple benchmark");
            doSimpleBenchmark();
        });


        _btnEncodeBenchmark.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                KANTVLog.j(TAG, "encode id:" + mEncodeID);
                KANTVLog.j(TAG, "resolution id:" + mResolutionID);
                KANTVLog.j(TAG, "pattern id:" + mEncodePattern);
                KANTVLog.j(TAG, "enableFilter:" + _bEnableFilter);
                int encodeID = BENCHMARK_H264;
                int width = 352;
                int height = 288;
                int fps = 25;
                switch (mEncodeID) {
                    case 0:
                        encodeID = BENCHMARK_H264;
                        KANTVLog.j(TAG, "H264");
                        break;
                    case 1:
                        encodeID = BENCHMARK_H265;
                        KANTVLog.j(TAG, "H265");
                        break;
                    case 2:
                        encodeID = BENCHMARK_H266;
                        KANTVLog.j(TAG, "H266");
                        break;
                    case 3:
                        encodeID = BENCHMARK_AOM_AV1;
                        KANTVLog.j(TAG, "AOM AV1");
                        break;
                    case 4:
                        encodeID = BENCHMARK_SVT_AV1;
                        KANTVLog.j(TAG, "SVT AV1");
                        break;
                }

                switch (mResolutionID) {
                    case 0:
                        width = 352;
                        height = 288;
                        break;
                    case 1:
                        width = 640;
                        height = 480;
                        break;
                    case 2:
                        width = 720;
                        height = 576;
                        break;
                    case 3:
                        width = 1280;
                        height = 720;
                        break;
                    case 4:
                        width = 1920;
                        height = 1080;
                        break;
                    case 5:
                        width = 2560;
                        height = 1440;
                        break;
                    case 6:
                        width = 3840;
                        height = 2160;
                        break;
                    case 7:
                        width = 7680;
                        height = 4320;
                        break;

                }
                if (!_bPreviewing) {
                    initKANTVMgr();
                    if (mKANTVMgr != null) {
                        _bPreviewing = true;
                        _btnEncodeBenchmark.setText("stop benchmark");
                        _tvEncodeBenchmarkInfo.setText(" ");


                        mKANTVMgr.open();
                        KANTVLog.j(TAG, "encode id:" + encodeID + ",width:" + width + ",height:" + height + ",fps:" + fps);
                        if (objSurfaceView == null) {
                            mKANTVMgr.setStreamFormat(0, encodeID, width, height, fps, AV_PIX_FMT_YUV420P, mEncodePattern, (_bEnableFilter ? 1 : 0), 1);
                            Surface surface = null;
                            mKANTVMgr.enablePreview(0, surface);
                        } else {
                            mKANTVMgr.setStreamFormat(0, encodeID, width, height, fps, AV_PIX_FMT_YUV420P, mEncodePattern, (_bEnableFilter ? 1 : 0), 0);
                            mKANTVMgr.enablePreview(0, objSurfaceView.getHolder().getSurface());
                        }

                        mKANTVMgr.startPreview();


                        _btnGraphicBenchmark.setEnabled(false);
                        _btnSynthesisBenchmark.setEnabled(false);
                    }

                } else {
                    _bPreviewing = false;
                    _btnEncodeBenchmark.setText("video encode benchmark");
                    if (mKANTVMgr != null) {
                        mKANTVMgr.disablePreview(0);
                        mKANTVMgr.stopPreview();
                        mKANTVMgr.close();
                        mKANTVMgr.release();
                        mKANTVMgr = null;
                    }
                    //_btnGraphicBenchmark.setEnabled(true);
                    //_btnSynthesisBenchmark.setEnabled(true);

                    if (_bEnableFilter) {
                        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                        //intent.setData(Uri.fromFile(Environment.getExternalStorageDirectory()));
                        Uri uri = Uri.fromFile(new File(KANTVUtils.getDataPath()));
                        intent.setData(uri);
                        if (mActivity.getBaseContext() != null)
                            mActivity.getBaseContext().sendBroadcast(intent);
                    }
                }
            }
        });

        KANTVLog.j(TAG, "layout ui, create new surface view");
        objSurfaceView = mActivity.findViewById(R.id.svEncodeBenchmark);
        surfaceHolder = objSurfaceView.getHolder();
        surfaceHolder.setFixedSize(SURFACE_WIDTH, SURFACE_HEIGHT);
        surfaceHolder.addCallback(shCallback);


        endTime = System.currentTimeMillis();
        KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
    }



    @Override
    public void onDestroy() {
        KANTVLog.j(TAG, "onDestroy");
        super.onDestroy();
        //release();

        if (surfaceHolder != null)
            surfaceHolder.removeCallback(shCallback);


    }

    @Override
    public void onResume() {
        KANTVLog.j(TAG, "onResume");
        super.onResume();
        //layoutUI(mActivity);
    }


    @Override
    public void onStop() {
        KANTVLog.j(TAG, "onStop");
        super.onStop();
        //release();
    }

    private SurfaceHolder.Callback shCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(@NonNull SurfaceHolder holder) {
            KANTVLog.j(TAG, "surface Created");
        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
            KANTVLog.j(TAG, "surface Changed:width" + width + ":height:" + height);
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            KANTVLog.j(TAG, "surface Destroyed");
            release();
        }
    };


    private static void addBuildField(String name, String value) {
        KANTVLog.j(TAG, "  " + name + ": " + value + "\n");
    }

    private int align(int value, int align) {
        return (((value) + (align) - 1) / (align) * (align));
    }

    private void doSimpleBenchmark() {
        benchmarkIndex = 0;
        totalDuration = 0;
        beginTime = 0;
        endTime = 0;

        isBenchmarking.set(true);
        startUIBuffering(mContext.getString(R.string.benchmark_updating));
        Toast.makeText(mContext, mContext.getString(R.string.benchmark_start), Toast.LENGTH_LONG).show();

        WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
        attributes.screenBrightness = 1.0f;
        mActivity.getWindow().setAttributes(attributes);
        mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        launchBenchmarkThread();
        _btnSynthesisBenchmark.setEnabled(false);
        _btnGraphicBenchmark.setEnabled(false);
        _btnEncodeBenchmark.setEnabled(false);
        _tvSynthesisBenchmarkInfo.setVisibility(View.VISIBLE);
        _btnSynthesisBenchmark.setText("progressing..");
        _tvEncodeBenchmarkInfo.setText(" ");

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
                            String benchmarkTip = "item " + benchmarkIndex + " cost: " + duration + " milliseconds";
                            String benchmarkTip1 = strBenchmarkInfo + " ,item " + benchmarkIndex + " cost: "
                                    + duration + " milliseconds" + " ,total cost: " + totalDuration + " millisecond";
                            KANTVLog.j(TAG, benchmarkTip1);
                            if ((_tvSynthesisBenchmarkInfo != null) && (benchmarkIndex <= 10) && (mProgressDialog != null)) {
                                strBenchmarkReport += benchmarkTip + "\n";
                                _tvSynthesisBenchmarkInfo.setText(benchmarkTip);
                                //_tvSynthesisBenchmarkInfo.setVisibility(View.INVISIBLE);
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
                                //_tvSynthesisBenchmarkInfo.setVisibility(View.INVISIBLE);
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

                    _tvSynthesisBenchmarkInfo.setVisibility(View.INVISIBLE);
                    Toast.makeText(mContext, mContext.getString(R.string.benchmark_stop), Toast.LENGTH_SHORT).show();

                    displayBenchmarkResult();
                }
                KANTVLog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                KANTVLog.j(TAG, "benchmark report:" + strBenchmarkReport);
                String benchmarkTip1 = "benchmark finished " + benchmarkIndex + ", cost:" + totalDuration + " milliseconds";
                KANTVLog.j(TAG, benchmarkTip1);
                _btnSynthesisBenchmark.setEnabled(true);
                _btnGraphicBenchmark.setEnabled(true);
                _btnEncodeBenchmark.setEnabled(true);
                _btnSynthesisBenchmark.setText(mContext.getString(R.string.StartSynthesisBenchmark));
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
            benchmarkTip = "you finished " + benchmarkIndex + " items" + "\n" + " total cost " + totalDuration + " milliseconds";
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

    private void showMsgBox(Context context, String message) {
        AlertDialog dialog = new AlertDialog.Builder(context).create();
        dialog.setMessage(message);
        dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

            }
        });
        dialog.show();
    }


    protected class MyEventListener implements KANTVEventListener {

        MyEventListener() {
        }


        @Override
        public void onEvent(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
            //String eventString = "got event from native layer: " + " eventType: " + eventType.getValue() + " eventName:" + eventType.toString() + " (" + what + ":" + arg1  + " ) :" + (String) obj;
            String eventString = "got event from native layer: " + eventType.toString() + " (" + what + ":" + arg1 + " ) :" + (String) obj;
            String content = (String) obj;

            if (eventType.getValue() == KANTVEvent.KANTV_ERROR) {
                KANTVLog.j(TAG, "ERROR:" + eventString);
                _tvEncodeBenchmarkInfo.setText("ERROR:" + eventString);
            }

            if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_INFO) {
                    _tvEncodeBenchmarkInfo.setText(content);
                    return;
                }

                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_START) {
                    KANTVLog.j(TAG, "encode benchmark start");
                    return;
                }

                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_STOP) {
                    KANTVLog.j(TAG, "encode benchmark stop");
                    //if (_bEnableFilter)
                    {
                        release();
                    }
                    return;
                }

                KANTVLog.j(TAG, "eventString:" + eventString);
            }

        }
    }


    private void initKANTVMgr() {
        if (mKANTVMgr != null) {
            return;
        }

        try {
            mKANTVMgr = new KANTVMgr(mEventListener);
            KANTVLog.j(TAG, "Mgr version:" + mKANTVMgr.getMgrVersion());
        } catch (KANTVException ex) {
            String errorMsg = "An Exception was thrown because:\n" + " " + ex.getMessage();
            KANTVLog.j(TAG, "error occurred: " + errorMsg);
            showMsgBox(mActivity, errorMsg);
            ex.printStackTrace();
        }
    }


    public void release() {
        _btnGraphicBenchmark.setEnabled(true);
        _btnSynthesisBenchmark.setEnabled(true);
        _bEnableFilter = false;
        _checkEnablefilter.setChecked(false);
        if (mKANTVMgr == null) {
            KANTVLog.j(TAG, "benchmark already stopped");
            return;
        }

        try {
            KANTVLog.j(TAG, "release");
            if (_bPreviewing) {
                mKANTVMgr.disablePreview(BENCHMARK_H264);
                mKANTVMgr.stopPreview();
                mKANTVMgr.close();
                mKANTVMgr.release();
                mKANTVMgr = null;
                _bPreviewing = false;
                _btnEncodeBenchmark.setText("video encode benchmark");
            }
        } catch (Exception ex) {
            String errorMsg = "An Exception was thrown because:\n" + " " + ex.getMessage();
            KANTVLog.j(TAG, "error occurred: " + errorMsg);
            ex.printStackTrace();
        }
    }

    public static native int kantv_anti_remove_rename_this_file();

}
