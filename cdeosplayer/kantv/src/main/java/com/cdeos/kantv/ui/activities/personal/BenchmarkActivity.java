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

package com.cdeos.kantv.ui.activities.personal;
import static cdeos.media.player.KANTVBenchmarkType.AV_PIX_FMT_YUV420P;
import static cdeos.media.player.KANTVBenchmarkType.BENCHMARK_AOM_AV1;
import static cdeos.media.player.KANTVBenchmarkType.BENCHMARK_H264;
import static cdeos.media.player.KANTVBenchmarkType.BENCHMARK_H265;
import static cdeos.media.player.KANTVBenchmarkType.BENCHMARK_H266;
import static cdeos.media.player.KANTVBenchmarkType.BENCHMARK_SVT_AV1;
import static cdeos.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_INFO;
import static cdeos.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_START;
import static cdeos.media.player.KANTVEvent.KANTV_INFO_ENCODE_BENCHMARK_STOP;

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
import com.cdeos.kantv.base.BaseMvcActivity;
import com.cdeos.kantv.R;
import com.cdeos.kantv.utils.Settings;


import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;
import java.util.concurrent.atomic.AtomicBoolean;

import cdeos.media.player.KANTVEvent;
import cdeos.media.player.KANTVEventListener;
import cdeos.media.player.KANTVEventType;
import cdeos.media.player.KANTVException;
import cdeos.media.player.KANTVMgr;
import cdeos.media.player.KANTVDRM;
import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;


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
        setTitle("性能评测");
        long beginTime = 0;
        long endTime = 0;
        beginTime = System.currentTimeMillis();

        mActivity = this;
        mContext = mActivity.getBaseContext();
        mSettings = new Settings(mContext);

        initView();

        endTime = System.currentTimeMillis();
        CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
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

        //if (mSettings.getUILang() == Settings.KANTV_UILANG_CN)
        {
            String info = "性能评测分为三类:图形处理,CPU算力,视频编码\n";
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
            info += "设备信息:" + "<br>"
                    + "品牌:" + Build.BRAND + "<br>"
                    + "架构:" + Build.CPU_ABI + "<br>"
                    + "硬件:" + Build.HARDWARE + "<br>"
                    /*+ "指纹:" + Build.FINGERPRINT + "<br>" */
                    + "操作系统:" + "Android " + android.os.Build.VERSION.RELEASE;
            //_deviceInfo.setText("手机设备信息:" + deviceInfo);
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
                CDELog.j(TAG, "position=" + position);
                if (position >= 0) {
                    mEncodeID = encodeformatToChoose[position];
                    CDELog.j(TAG, "encode id:" + mEncodeID);
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
                CDELog.j(TAG, "position=" + position);
                if (position >= 0) {
                    mResolutionID = encodeResolutionToChoose[position];
                    CDELog.j(TAG, "resolution id:" + mResolutionID);

                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

/*
        Spinner spinnerPattern = mActivity.findViewById(R.id.spinnerPattern);
        String[] sentencesPattern = getResources().getStringArray(R.array.encodePattern);
        ArrayAdapter<String> adapterPattern = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, sentencesPattern);
        spinnerPattern.setAdapter(adapterPattern);
        spinnerPattern.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                CDELog.j(TAG, "position=" + position);
                if (position >= 0) {
                    mEncodePattern = position;
                    CDELog.j(TAG, "pattern id:" + mEncodePattern);

                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

 */


        _checkEnablefilter = mActivity.findViewById(R.id.chkEnableFilter);
        _checkEnablefilter.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                CDELog.j(TAG, "isChecked:" + isChecked);
                _bEnableFilter = isChecked;
                CDELog.j(TAG, "_bEnableFilter:" + _bEnableFilter);
            }
        });

        flEncodeBenchmark = mActivity.findViewById(R.id.flEncodeBenchmark);


        //layoutUI(mActivity);


        _tvPageInfo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String strTipInfo = "" +
                        "图形测试大约需要1-3分钟，不能中途退出图形测试(强制退出可能导致app异常)\n\n" +
                        "综合测试很耗时，大约需要5-10分钟，可以随时退出综合测试\n\n" +
                        "编码测试可以随时退出，如果勾选编码视频帧保存为文件，为节约手机存储资源，只保存250帧.保存的文件在\"本地媒体\"页面中可查看\n\n" +
                        "如在进行性能评测中有bug或技术问题需要反馈，可在各类测试完毕后截图，然后将截图发给开发人员(admin@cde-os.com)，便于改进性能评测";
                showMsgBox(mActivity, strTipInfo);
            }
        });


        _btnGraphicBenchmark.setOnClickListener(v -> {
            CDELog.j(TAG, "start graphic benchmark");

            launchActivity(KanTVBenchmarkActivity.class);
        });


        _btnSynthesisBenchmark.setOnClickListener(v -> {
            CDELog.j(TAG, "start simple benchmark");
            doSimpleBenchmark();
        });


        _btnEncodeBenchmark.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                CDELog.j(TAG, "encode id:" + mEncodeID);
                CDELog.j(TAG, "resolution id:" + mResolutionID);
                CDELog.j(TAG, "pattern id:" + mEncodePattern);
                CDELog.j(TAG, "enableFilter:" + _bEnableFilter);
                int encodeID = BENCHMARK_H264;
                int width = 352;
                int height = 288;
                int fps = 25;
                switch (mEncodeID) {
                    case 0:
                        encodeID = BENCHMARK_H264;
                        CDELog.j(TAG, "H264");
                        break;
                    case 1:
                        encodeID = BENCHMARK_H265;
                        CDELog.j(TAG, "H265");
                        break;
                    case 2:
                        encodeID = BENCHMARK_H266;
                        CDELog.j(TAG, "H266");
                        break;
                    case 3:
                        encodeID = BENCHMARK_AOM_AV1;
                        CDELog.j(TAG, "AOM AV1");
                        break;
                    case 4:
                        encodeID = BENCHMARK_SVT_AV1;
                        CDELog.j(TAG, "SVT AV1");
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
                        _btnEncodeBenchmark.setText("停止测试");
                        _tvEncodeBenchmarkInfo.setText(" ");


                        mKANTVMgr.open();
                        CDELog.j(TAG, "encode id:" + encodeID + ",width:" + width + ",height:" + height + ",fps:" + fps);
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
                    _btnEncodeBenchmark.setText("编码测试");
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
                        //通知系统刷新目录
                        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                        //intent.setData(Uri.fromFile(Environment.getExternalStorageDirectory()));
                        Uri uri = Uri.fromFile(new File(CDEUtils.getDataPath()));
                        intent.setData(uri);
                        if (mActivity.getBaseContext() != null)
                            mActivity.getBaseContext().sendBroadcast(intent);
                    }
                }
            }
        });

        CDELog.j(TAG, "layout ui, create new surface view");
        objSurfaceView = mActivity.findViewById(R.id.svEncodeBenchmark);
        surfaceHolder = objSurfaceView.getHolder();
        surfaceHolder.setFixedSize(SURFACE_WIDTH, SURFACE_HEIGHT);
        surfaceHolder.addCallback(shCallback);


        endTime = System.currentTimeMillis();
        CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
    }



    @Override
    public void onDestroy() {
        CDELog.j(TAG, "onDestroy");
        super.onDestroy();
        //release();

        if (surfaceHolder != null)
            surfaceHolder.removeCallback(shCallback);


    }

    @Override
    public void onResume() {
        CDELog.j(TAG, "onResume");
        super.onResume();
        //layoutUI(mActivity);
    }


    @Override
    public void onStop() {
        CDELog.j(TAG, "onStop");
        super.onStop();
        //release();
    }

    private SurfaceHolder.Callback shCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(@NonNull SurfaceHolder holder) {
            CDELog.j(TAG, "surface Created");
        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
            CDELog.j(TAG, "surface Changed:width" + width + ":height:" + height);
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            CDELog.j(TAG, "surface Destroyed");
            release();
        }
    };


    private static void addBuildField(String name, String value) {
        CDELog.j(TAG, "  " + name + ": " + value + "\n");
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
        _btnSynthesisBenchmark.setText("测试中...");
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
                    CDELog.j(TAG, "ANDROID_JNI_Benchmark cost: " + duration + " milliseconds");

                    mActivity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            //String benchmarkTip = strBenchmarkInfo + "\n" + "第" + benchmarkIndex + "项测试耗时:" + duration + "毫秒";
                            String benchmarkTip = "综合测试第" + benchmarkIndex + "项耗时:" + duration + "毫秒";
                            String benchmarkTip1 = strBenchmarkInfo + ",第" + benchmarkIndex + "项测试耗时:" + duration + "毫秒" + ",总耗时:" + totalDuration + "毫秒";
                            CDELog.j(TAG, benchmarkTip1);
                            //Toast.makeText(mActivity, "ANDROID_JNI_Benchmark cost: " + duration + " milliseconds", Toast.LENGTH_LONG).show();
                            if ((_tvSynthesisBenchmarkInfo != null) && (benchmarkIndex <= 10) && (mProgressDialog != null)) {
                                strBenchmarkReport += benchmarkTip + "\n";
                                _tvSynthesisBenchmarkInfo.setText(benchmarkTip);
                                //_tvSynthesisBenchmarkInfo.setVisibility(View.INVISIBLE);
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
                CDELog.j(TAG, "benchmarkIndex: " + benchmarkIndex);
                CDELog.j(TAG, "benchmark report:" + strBenchmarkReport);
                String benchmarkTip1 = "完成" + benchmarkIndex + "项测试,总耗时:" + totalDuration + "毫秒";
                CDELog.j(TAG, benchmarkTip1);
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
            benchmarkTip = "您尚未完成手机性能综合测试";
        } else {
            benchmarkTip = "您完成了" + benchmarkIndex + "项手机性能综合测试" + "\n" + "总耗时" + totalDuration + "毫秒(1秒=1000毫秒)";
            durations = totalDuration / 1000;

            if (benchmarkIndex > 0)
                durationPerItem = durations / benchmarkIndex;

            benchmarkTip += "\n" + "单项测试平均耗时" + durationPerItem + "秒";
            SharedPreferences.Editor editor = mSharedPreferences.edit();
            editor.putString("pref.recordBenchmark", String.valueOf(durationPerItem));
            editor.commit();
            CDELog.j(TAG, "pref.recordBenchmark=" + mSettings.getRecordBenchmark());

            if (durationPerItem <= 1) {
                benchmarkTip += "\n" + "恭贺您，您的手机性能非常不错";
            } else if ((durationPerItem >= 2) && (durationPerItem < 4)) {
                benchmarkTip += "\n" + "您的手机性能很不错";
            } else if ((durationPerItem >= 4) && (durationPerItem <= 6)) {
                benchmarkTip += "\n" + "您的手机性能不错";
            } else if ((durationPerItem > 6) && (durationPerItem <= 9)) {
                benchmarkTip += "\n" + "您的手机速度可能比较慢";
            } else if ((durationPerItem > 9) && (durationPerItem < 12)) {
                benchmarkTip += "\n" + "您的手机可能很慢";
            } else if (durationPerItem >= 12) {
                benchmarkTip += "\n" + "您的手机可能非常慢";
            }
        }
        CDELog.j(TAG, benchmarkTip);
        CDELog.j(TAG, "单项测试耗时:" + durationPerItem + "秒");
        CDEUtils.umRecordBenchmark(benchmarkIndex, totalDuration, durationPerItem);
        _tvSynthesisBenchmarkInfo.setVisibility(View.VISIBLE);
        _tvSynthesisBenchmarkInfo.setText(benchmarkTip);
        //benchmarkTip += "\n\n" + "KanTV视频录制提示:AV1编码非常耗时，H265编码也很耗时";
        showBenchmarkResult(mActivity, benchmarkTip);
    }

    private void showBenchmarkResult(Context context, String benchmarkInfo) {
        new AlertDialog.Builder(context)
                .setTitle("您的手机性能综合测试结果")
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
                CDELog.j(TAG, "ERROR:" + eventString);
                _tvEncodeBenchmarkInfo.setText("ERROR:" + eventString);
            }

            if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_INFO) {
                    _tvEncodeBenchmarkInfo.setText(content);
                    return;
                }

                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_START) {
                    CDELog.j(TAG, "encode benchmark start");
                    return;
                }

                if (arg1 == KANTV_INFO_ENCODE_BENCHMARK_STOP) {
                    CDELog.j(TAG, "encode benchmark stop");
                    //if (_bEnableFilter)
                    {
                        release();
                    }
                    return;
                }

                CDELog.j(TAG, "eventString:" + eventString);
            }

        }
    }


    private void initKANTVMgr() {
        if (mKANTVMgr != null) {
            return;
        }

        try {
            mKANTVMgr = new KANTVMgr(mEventListener);
            CDELog.j(TAG, "Mgr version:" + mKANTVMgr.getMgrVersion());
        } catch (KANTVException ex) {
            String errorMsg = "An Exception was thrown because:\n" + " " + ex.getMessage();
            CDELog.j(TAG, "error occurred: " + errorMsg);
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
            CDELog.j(TAG, "benchmark already stopped");
            return;
        }

        try {
            CDELog.j(TAG, "release");
            if (_bPreviewing) {
                mKANTVMgr.disablePreview(BENCHMARK_H264);
                mKANTVMgr.stopPreview();
                mKANTVMgr.close();
                mKANTVMgr.release();
                mKANTVMgr = null;
                _bPreviewing = false;
                _btnEncodeBenchmark.setText("编码测试");
            }
        } catch (Exception ex) {
            String errorMsg = "An Exception was thrown because:\n" + " " + ex.getMessage();
            CDELog.j(TAG, "error occured: " + errorMsg);
            ex.printStackTrace();
        }
    }

    public static native int kantv_anti_tamper();

}
