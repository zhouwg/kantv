 /*
  * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
  *
  * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
  *
  * @author: zhou.weiguo
  *
  * @desc: implementation of PoC stage-2 for https://github.com/cdeos/kantv/issues/64
  *
  * @date: 03-08-2024(2024-03-08)
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
 package com.cdeos.kantv.ui.fragment;

 import static cdeos.media.player.KANTVEvent.KANTV_INFO_ASR_FINALIZE;
 import static cdeos.media.player.KANTVEvent.KANTV_INFO_ASR_STOP;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.app.AlertDialog;
 import android.app.ProgressDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.res.Resources;
 import android.media.MediaPlayer;
 import android.os.Build;
 import android.view.View;
 import android.view.WindowManager;
 import android.widget.AdapterView;
 import android.widget.ArrayAdapter;
 import android.widget.Button;
 import android.widget.LinearLayout;
 import android.widget.Spinner;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;

 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.mvp.impl.ASRPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.ASRPresenter;
 import com.cdeos.kantv.mvp.view.ASRView;
 import com.cdeos.kantv.utils.Settings;

 import org.ggml.whispercpp.whispercpp;

 import java.io.File;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.io.RandomAccessFile;
 import java.nio.ByteBuffer;
 import java.nio.ByteOrder;
 import java.util.concurrent.atomic.AtomicBoolean;

 import butterknife.BindView;
 import cdeos.media.player.CDEAssetLoader;
 import cdeos.media.player.CDELibraryLoader;
 import cdeos.media.player.CDELog;
 import cdeos.media.player.CDEUtils;
 import cdeos.media.player.KANTVEvent;
 import cdeos.media.player.KANTVEventListener;
 import cdeos.media.player.KANTVEventType;
 import cdeos.media.player.KANTVException;
 import cdeos.media.player.KANTVMgr;


 public class ASRFragment extends BaseMvpFragment<ASRPresenter> implements ASRView {
     @BindView(R.id.ggmlLayout)
     LinearLayout layout;

     private static final String TAG = ASRFragment.class.getName();
     TextView _txtASRInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;

     Button _btnBenchmark;

     private static final int BECHMARK_MEMCPY       = 0;
     private static final int BECHMARK_MULMAT       = 1;
     private static final int BECHMARK_ASR          = 2;
     private static final int BECHMARK_FULL         = 3; //very slow on Android phone

     private int nThreadCounts = 1;
     private int benchmarkIndex = 0;
     private String strModeName = "tiny";

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     private String ggmlModelFileName = "ggml-tiny.bin";
     private String ggmlSampleFileName = "jfk.wav";

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private KANTVMgr mKANTVMgr = null;
     private ASRFragment.MyEventListener mEventListener = new ASRFragment.MyEventListener();


     public static ASRFragment newInstance() {
         return new ASRFragment();
     }

     @NonNull
     @Override
     protected ASRPresenter initPresenter() {
         return new ASRPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_asr;
     }


     @SuppressLint("CheckResult")
     @Override
     public void initView() {
         long beginTime = 0;
         long endTime = 0;
         beginTime = System.currentTimeMillis();

         mActivity = getActivity();
         mContext = mActivity.getBaseContext();
         mSettings = new Settings(mContext);
         mSettings.updateUILang((AppCompatActivity) getActivity());
         Resources res = mActivity.getResources();

         _txtASRInfo = (TextView) mActivity.findViewById(R.id.asrInfo);
         _txtGGMLInfo = (TextView) mActivity.findViewById(R.id.ggmlInfo);
         _txtGGMLStatus = (TextView) mActivity.findViewById(R.id.ggmlStatus);
         _btnBenchmark = (Button) mActivity.findViewById(R.id.btnBenchmark);


         //copy asset files to /sdcard
         //or just upload dependent files to /sdcard accordingly so the APK size would be smaller significantly
         CDEAssetLoader.copyAssetFile(mContext, ggmlModelFileName, CDEUtils.getDataPath() + ggmlModelFileName);
         CDEAssetLoader.copyAssetFile(mContext, ggmlSampleFileName, CDEUtils.getDataPath() + ggmlSampleFileName);

         _txtASRInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
         displayFileStatus(CDEUtils.getDataPath() + ggmlSampleFileName, CDEUtils.getDataPath() + ggmlModelFileName);

         CDELibraryLoader.load("whispercpp");

         try {
             initKANTVMgr();
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         CDELog.j(TAG, "load ggml's whisper model");
         String systemInfo = whispercpp.get_systeminfo();
         String phoneInfo = "Device info:" + "\n"
                 + "Brand:" + Build.BRAND + "\n"
                 + "Hardware:" + Build.HARDWARE + "\n"
                 /*+ "Fingerprint:" + Build.FINGERPRINT + "\n"*/ /* pls don't uncomment this line in public project */
                 + "OS:" + "Android " + android.os.Build.VERSION.RELEASE + "\n"
                 + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")";
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(phoneInfo + "\n");
         _txtGGMLInfo.append("Powered by whisper.cpp(https://github.com/ggerganov/whisper.cpp)\n");


         Spinner spinnerBenchType = mActivity.findViewById(R.id.spinnerBenchType);
         String[] arrayBenchType = getResources().getStringArray(R.array.benchType);
         ArrayAdapter<String> adapterBenchType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBenchType);
         spinnerBenchType.setAdapter(adapterBenchType);
         spinnerBenchType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "bench type:" + arrayBenchType[position]);
                 benchmarkIndex = Integer.valueOf(position);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });

         Spinner spinnerThreadsCounts = mActivity.findViewById(R.id.spinnerThreadCounts);
         String[] arrayThreadCounts = getResources().getStringArray(R.array.threadCounts);
         ArrayAdapter<String> adapter = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayThreadCounts);
         spinnerThreadsCounts.setAdapter(adapter);
         spinnerThreadsCounts.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "thread counts:" + arrayThreadCounts[position]);
                 nThreadCounts = Integer.valueOf(arrayThreadCounts[position]);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });

         Spinner spinnerModelName = mActivity.findViewById(R.id.spinnerModelName);
         String[] arrayModelName = getResources().getStringArray(R.array.modelName);
         ArrayAdapter<String> adapterModel = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayModelName);
         spinnerModelName.setAdapter(adapterModel);
         spinnerModelName.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "model:" + arrayModelName[position]);
                 strModeName = arrayModelName[position];
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });


         _btnBenchmark.setOnClickListener(v -> {
             CDELog.j(TAG, "exec ggml benchmark: type: " + getBenchmarkDesc(benchmarkIndex) + ", threads:" + nThreadCounts + ", model:" + strModeName);

             String selectModeFileName = "ggml-" + strModeName + ".bin";
             String selectModelFilePath = CDEUtils.getDataPath() + selectModeFileName;
             File selectModeFile = new File(selectModelFilePath);
             displayFileStatus(CDEUtils.getDataPath() + ggmlSampleFileName, selectModelFilePath);
             if (!selectModeFile.exists()) {
                 CDELog.j(TAG, "model file not exist:" + selectModeFile.getAbsolutePath());
             }
             File sampleFile = new File(CDEUtils.getDataPath() + ggmlSampleFileName);

             if (!selectModeFile.exists() || (!sampleFile.exists())) {
                 showMsgBox(mActivity, "pls check whether model file or sample file exist in /sdcard/kantv/");
                 return;
             }

             if (benchmarkIndex == BECHMARK_ASR) {
                 //playAudioFile();
             }

             isBenchmarking.set(true);

             startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating) + "(" + getBenchmarkDesc(benchmarkIndex) + ")");

             Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();

             //update UI status
             _txtASRInfo.setText("");
             _btnBenchmark.setEnabled(false);

             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

             launchGGMLBenchmarkThread();

         });


         endTime = System.currentTimeMillis();
         CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }


     private final void launchGGMLBenchmarkThread() {
         Thread workThread = new Thread(new Runnable() {
             @RequiresApi(api = Build.VERSION_CODES.O)
             @Override
             public void run() {
                 strBenchmarkInfo = "";

                 initKANTVMgr();
                 whispercpp.set_benchmark_status(0);

                 if (mKANTVMgr != null) {
                     mKANTVMgr.initASR();
                     mKANTVMgr.startASR();
                 }

                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();
                     strBenchmarkInfo = whispercpp.bench(
                             CDEUtils.getDataPath() + ggmlModelFileName,
                             CDEUtils.getDataPath() + ggmlSampleFileName,
                             benchmarkIndex,
                             nThreadCounts);
                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);

                     isBenchmarking.set(false);
                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String benchmarkTip = getBenchmarkDesc(benchmarkIndex) + "(model: " + strModeName
                                     + " ,threads: " + nThreadCounts
                                     + " ) cost " + duration + " milliseconds";
                             benchmarkTip += "\n";

                             //becareful here
                             if (!strBenchmarkInfo.startsWith("unknown")) {
                                 benchmarkTip += strBenchmarkInfo;
                             }


                             if (strBenchmarkInfo.startsWith("asr_result")) { //when got asr result, playback the audio file
                                 playAudioFile();
                             }

                             CDELog.j(TAG, benchmarkTip);
                             _txtGGMLStatus.setText("");
                             _txtGGMLStatus.setText(benchmarkTip);

                             //update UI status
                             _btnBenchmark.setEnabled(true);
                         }
                     });
                 }

                 stopUIBuffering();
                 release();

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
                                 CDELog.j(TAG, "stop GGML benchmark");
                                 whispercpp.set_benchmark_status(1);
                                 isBenchmarking.set(false);
                                 mProgressDialog.dismiss();
                                 mProgressDialog = null;

                                 //background computing task(it's a blocked task) in native layer might be not finished,
                                 //so don't update UI status here

                                 //TODO:
                                 //for keep (FSM) status sync accurately between UI and native source code, there are might be much efforts to do it
                                 //just like ggml_abort_callback in ggml.c
                                 //this is the gap between open source project(PoC/demo) and "real project"(commercial project)
                                 //we don't care this during PoC stage
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
                     Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_stop), Toast.LENGTH_SHORT).show();
                 }
                 String benchmarkTip = "GGML benchmark finished ";
                 CDELog.j(TAG, benchmarkTip);
                 release();
             }
         });
     }


     @Override
     public void initListener() {

     }

     @Override
     public void onDestroy() {
         super.onDestroy();
     }

     @Override
     public void onResume() {
         super.onResume();
     }

     @Override
     public void onStop() {
         super.onStop();
     }


     public void playAudioFile() {
         try {
             MediaPlayer mediaPlayer = new MediaPlayer();
             CDELog.j(TAG, "audio file:" + CDEUtils.getDataPath() + ggmlSampleFileName);
             mediaPlayer.setDataSource(CDEUtils.getDataPath() + ggmlSampleFileName);
             mediaPlayer.prepare();
             mediaPlayer.start();
         } catch (IOException ex) {
             CDELog.j(TAG, "failed to play audio file:" + ex.toString());
         } catch (Exception ex) {
             CDELog.j(TAG, "failed to play audio file:" + ex.toString());
         }
     }

     protected class MyEventListener implements KANTVEventListener {

         MyEventListener() {
         }


         @Override
         public void onEvent(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
             String eventString = "got event from native layer: " + eventType.toString() + " (" + what + ":" + arg1 + " ) :" + (String) obj;
             String content = (String) obj;

             if (eventType.getValue() == KANTVEvent.KANTV_ERROR) {
                 CDELog.j(TAG, "ERROR:" + eventString);
                 _txtASRInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP)
                         || (arg1 == KANTV_INFO_ASR_FINALIZE)
                 ) {
                     return;
                 }


                 //CDELog.j(TAG, "content:" + content);
                 if (content.startsWith("unknown")) {

                 } else {
                     _txtASRInfo.setText(content);
                 }
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO_ASR_RESULT) {
                 playAudioFile();
             }

         }
     }


     private void initKANTVMgr() {
         if (mKANTVMgr != null) {
             return;
         }

         try {
             mKANTVMgr = new KANTVMgr(mEventListener);
             CDELog.j(TAG, "KANTVMgr version:" + mKANTVMgr.getMgrVersion());
         } catch (KANTVException ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             CDELog.j(TAG, "error occurred: " + errorMsg);
             showMsgBox(mActivity, errorMsg);
             ex.printStackTrace();
         }
     }


     public void release() {
         if (mKANTVMgr == null) {
             return;
         }

         try {
             CDELog.j(TAG, "release");
             {
                 mKANTVMgr.finalizeASR();
                 mKANTVMgr.stopASR();
                 mKANTVMgr.release();
                 mKANTVMgr = null;
             }
         } catch (Exception ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             CDELog.j(TAG, "error occurred: " + errorMsg);
             ex.printStackTrace();
         }
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


     private String getBenchmarkDesc(int benchmarkIndex) {
         switch (benchmarkIndex) {
             case BECHMARK_FULL:
                 return "GGML whisper_encoder";

             case BECHMARK_MEMCPY:
                 return "GGML memcopy";

             case BECHMARK_MULMAT:
                 return "GGML matrix multipy";

             case BECHMARK_ASR:
                 return "GGML ASR inference";
         }

         return "unknown";
     }

     private void displayFileStatus(String sampleFilePath, String modelFilePath) {
         _txtGGMLStatus.setText("");

         File sampleFile = new File(sampleFilePath);
         if (sampleFile.exists()) {
             _txtGGMLStatus.append("sample file exist:" + sampleFile.getAbsolutePath());
         } else {
             CDELog.j(TAG, "sample file not exist:" + sampleFile.getAbsolutePath());
             _txtGGMLStatus.append("\nsample file not exist: " + sampleFile.getAbsolutePath());
         }

         _txtGGMLStatus.append("\n");

         File modelFile = new File(modelFilePath);
         if (modelFile.exists()) {
             _txtGGMLStatus.append("model   file exist:" + modelFile.getAbsolutePath());
         } else {
             CDELog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
             _txtGGMLStatus.append("model   file not exist: " + modelFile.getAbsolutePath());
         }
     }

 }
