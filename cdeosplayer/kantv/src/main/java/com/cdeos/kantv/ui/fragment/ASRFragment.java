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
 package com.cdeos.kantv.ui.fragment;

 import static cdeos.media.player.KANTVEvent.KANTV_INFO_ASR_FINALiZE;
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
 import android.view.WindowManager;
 import android.widget.Button;
 import android.widget.LinearLayout;
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

 import org.ggml.whispercpp.WhisperLib;

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

     Button _btnBenchmarkMemcpy;
     Button _btnBenchmarkMalmat;
     Button _btnInference;

     private int benchmarkIndex = 0;
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

     @RequiresApi(api = Build.VERSION_CODES.O)
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
         _btnBenchmarkMemcpy = (Button) mActivity.findViewById(R.id.btnBenchmarkMemcpy);
         _btnBenchmarkMalmat = (Button) mActivity.findViewById(R.id.btnBenchmarkMulmat);
         _btnInference = (Button) mActivity.findViewById(R.id.btnInference);

         CDEAssetLoader.copyAssetFile(mContext, ggmlModelFileName, CDEUtils.getDataPath(mContext) + ggmlModelFileName);
         CDEAssetLoader.copyAssetFile(mContext, ggmlSampleFileName, CDEUtils.getDataPath(mContext) + ggmlSampleFileName);

         _txtASRInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);

         File modelFile = new File(CDEUtils.getDataPath(mContext) + ggmlModelFileName);
         if (modelFile.exists()) {
             _txtGGMLStatus.setText("model file exist:" + ggmlModelFileName);
         } else {
             CDELog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
             _txtGGMLStatus.setText("model file not exist: " + ggmlModelFileName);
         }

         File sampleFile = new File(CDEUtils.getDataPath(mContext) + ggmlSampleFileName);
         if (sampleFile.exists()) {
             _txtGGMLStatus.append("\nsample file exist:" + ggmlSampleFileName);
         } else {
             CDELog.j(TAG, "model file not exist:" + sampleFile.getAbsolutePath());
             _txtGGMLStatus.append("\nmodel file not exist: " + ggmlSampleFileName);
         }

         CDELibraryLoader.load("whispercpp");

         initKANTVMgr();


         CDELog.j(TAG, "load ggml's whisper model");
         String systemInfo = WhisperLib.getSystemInfo();
         String phoneInfo = "Device info:" + "\n"
                 + "Brand:" + Build.BRAND + "\n"
                 + "Hardware:" + Build.HARDWARE + "\n"
                 /*+ "Fingerprint:" + Build.FINGERPRINT + "\n"*/
                 + "OS:" + "Android " + android.os.Build.VERSION.RELEASE + "\n"
                 + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")";
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(phoneInfo + "\n\n");
         _txtGGMLInfo.append("Powered by whisper.cpp(https://github.com/ggerganov/whisper.cpp)\n\n");


         _btnBenchmarkMemcpy.setOnClickListener(v -> {
             CDELog.j(TAG, "exec ggml benchmark of memcopy");
             benchmarkIndex = 0;
             isBenchmarking.set(true);
             startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating));
             Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();
             _txtASRInfo.setText("");

             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

             launchGGMLBenchmarkThread();

         });

         _btnBenchmarkMalmat.setOnClickListener(v -> {
             CDELog.j(TAG, "exec ggml benchmakr of matrix multiply");
             isBenchmarking.set(true);
             benchmarkIndex = 1;
             startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating));
             Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();
             _txtASRInfo.setText("");

             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

             launchGGMLBenchmarkThread();

         });


         _btnInference.setOnClickListener(v -> {
             CDELog.j(TAG, "exec ggml inference");
             this.playAudioFile();
             _txtASRInfo.setText("");
             //TODO:exec inference by ggml's whisper.cpp
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
                 WhisperLib.set_mulmat_benchmark_status(0);

                 if (mKANTVMgr != null) {
                     mKANTVMgr.initASR();
                     mKANTVMgr.startASR();
                 }
                 while (isBenchmarking.get()) {
                     switch (benchmarkIndex) {
                         case 0:
                             beginTime = System.currentTimeMillis();
                             strBenchmarkInfo = WhisperLib.benchMemcpy(1);
                             endTime = System.currentTimeMillis();
                             duration = (endTime - beginTime);
                             CDELog.j(TAG, "GGML memcpy benchmark cost: " + duration + " milliseconds");
                             break;
                         case 1:
                             beginTime = System.currentTimeMillis();
                             strBenchmarkInfo += "\n";
                             strBenchmarkInfo += WhisperLib.benchGgmlMulMat(1);
                             endTime = System.currentTimeMillis();
                             duration = (endTime - beginTime);
                             CDELog.j(TAG, "GGML mulmat benchmark cost: " + duration + " milliseconds");
                         default:
                             break;
                     }
                     isBenchmarking.set(false);


                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String benchmarkTip = "GGML benchmark cost: " + duration + " milliseconds";
                             benchmarkTip += "\n";
                             benchmarkTip += strBenchmarkInfo;
                             CDELog.j(TAG, benchmarkTip);
                             _txtASRInfo.setText("");
                             _txtASRInfo.setText(benchmarkTip);
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
                         @RequiresApi(api = Build.VERSION_CODES.O)
                         @Override
                         public void onCancel(DialogInterface dialogInterface) {
                             if (mProgressDialog != null) {
                                 CDELog.j(TAG, "stop GGML benchmark");
                                 WhisperLib.set_mulmat_benchmark_status(1);
                                 isBenchmarking.set(false);
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


     private void stopUIBuffering() {
         mActivity.runOnUiThread(new Runnable() {
             @RequiresApi(api = Build.VERSION_CODES.O)
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
             CDELog.j(TAG, "audio file:" + CDEUtils.getDataPath(mContext) + ggmlSampleFileName);
             mediaPlayer.setDataSource(CDEUtils.getDataPath(mContext) + ggmlSampleFileName);
             mediaPlayer.prepare();
             mediaPlayer.start();
         } catch (IOException ex) {
             CDELog.j(TAG, "failed:" + ex.toString());
         } catch (Exception ex) {
             CDELog.j(TAG, "failed:" + ex.toString());
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
                         || (arg1 == KANTV_INFO_ASR_FINALiZE)
                 ) {
                     return;
                 }
                 CDELog.j(TAG, "eventString:" + eventString);
                 _txtASRInfo.setText(content);
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
 }
