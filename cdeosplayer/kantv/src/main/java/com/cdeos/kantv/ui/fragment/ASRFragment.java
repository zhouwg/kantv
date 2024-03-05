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

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.app.ProgressDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.res.Resources;
 import android.media.MediaPlayer;
 import android.os.Build;
 import android.text.Html;
 import android.text.method.LinkMovementMethod;
 import android.view.View;
 import android.view.WindowManager;
 import android.widget.Button;
 import android.widget.EditText;
 import android.widget.LinearLayout;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;

 import com.cdeos.kantv.mvp.impl.ASRPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.ASRPresenter;
 import com.cdeos.kantv.mvp.view.ASRView;
 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.utils.Settings;

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

 import org.ggml.whispercpp.WhisperLib;


 public class ASRFragment extends BaseMvpFragment<ASRPresenter> implements ASRView {
     @BindView(R.id.ggmlLayout)
     LinearLayout layout;

     private static final String TAG = ASRFragment.class.getName();
     TextView _txtASRInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;

     Button _ggmlLoadModel;
     Button _ggmlBenchmark;
     Button _ggmlInference;

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     private String ggmlModelFileName   = "ggml-tiny.bin";
     private String ggmlSampleFileName  = "jfk.wav";

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;


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
         _ggmlLoadModel = (Button) mActivity.findViewById(R.id.btnGGMLLoadModel);
         _ggmlBenchmark = (Button) mActivity.findViewById(R.id.btnGGMLBenchmark);
         _ggmlInference = (Button) mActivity.findViewById(R.id.btnGGMLInference);

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

         CDELibraryLoader.load("whisper");

         //_ggmlLoadModel.setOnClickListener(v -> {
             CDELog.j(TAG, "load ggml's whisper model");
             String systemInfo = WhisperLib.getSystemInfo();
             String phoneInfo = "Device info:" + "\n"
                     + "Brand:" + Build.BRAND + "\n"
                     + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")" + "\n"
                     + "Hardware:" + Build.HARDWARE + "\n"
                     /*+ "Fingerprint:" + Build.FINGERPRINT + "\n"*/
                     + "OS:" + "Android " + android.os.Build.VERSION.RELEASE;
             _txtGGMLInfo.setText("");
             _txtGGMLInfo.append(phoneInfo + "\n\n");
             _txtGGMLInfo.append("Powered by whisper.cpp(https://github.com/ggerganov/whisper.cpp)\n\n");
         //});

         _ggmlBenchmark.setOnClickListener(v -> {
             CDELog.j(TAG, "exec ggml benchmark");
             isBenchmarking.set(true);
             startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating));
             Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();

             WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
             attributes.screenBrightness = 1.0f;
             mActivity.getWindow().setAttributes(attributes);
             mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
             launchGGMLBenchmarkThread();

         });

         _ggmlInference.setOnClickListener(v -> {
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
                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();

                     strBenchmarkInfo = WhisperLib.benchMemcpy(1);

                     endTime = System.currentTimeMillis();

                     duration = (endTime - beginTime);

                     CDELog.j(TAG, "benchMemcpy cost: " + duration + " milliseconds");

                     isBenchmarking.set(false);
                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String benchmarkTip = "benchmark cost: " + duration + " milliseconds";
                             benchmarkTip += "\n";
                             benchmarkTip += strBenchmarkInfo;
                             CDELog.j(TAG, benchmarkTip);
                             _txtASRInfo.setText("");
                             _txtASRInfo.setText(benchmarkTip);
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
                 String benchmarkTip = "benchmark finished ";
                 CDELog.j(TAG, benchmarkTip);
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

     private char readLEChar(RandomAccessFile f) throws IOException {
         byte b1 = f.readByte();
         byte b2 = f.readByte();
         return (char) ((b2 << 8) | b1);
     }

     private int readLEInt(RandomAccessFile f) throws IOException {
         byte b1 = f.readByte();
         byte b2 = f.readByte();
         byte b3 = f.readByte();
         byte b4 = f.readByte();
         return (int) ((b1 & 0xFF) | (b2 & 0xFF) << 8 | (b3 & 0xFF) << 16 | (b4 & 0xFF) << 24);
     }

     private void doInference(String audioFile) {
         long inferenceExecTime = 0;
         try {
             RandomAccessFile wave = new RandomAccessFile(audioFile, "r");

             wave.seek(20);
             char audioFormat = this.readLEChar(wave);
             assert (audioFormat == 1); // 1 is PCM

             wave.seek(22);
             char numChannels = this.readLEChar(wave);
             assert (numChannels == 1); // MONO

             wave.seek(24);
             int sampleRate = this.readLEInt(wave);
             //assert (sampleRate == this._m.sampleRate()); // desired sample rate

             wave.seek(34);
             char bitsPerSample = this.readLEChar(wave);
             assert (bitsPerSample == 16); // 16 bits per sample

             wave.seek(40);
             int bufferSize = this.readLEInt(wave);
             assert (bufferSize > 0);

             wave.seek(44);
             byte[] bytes = new byte[bufferSize];
             wave.readFully(bytes);

             short[] shorts = new short[bytes.length / 2];
             // to turn bytes to shorts as either big endian or little endian.
             ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);

             //this._tfliteStatus.setText("Running inference ...");

             long inferenceStartTime = System.currentTimeMillis();

             //String decoded = this._m.stt(shorts, shorts.length);

             inferenceExecTime = System.currentTimeMillis() - inferenceStartTime;

             //this._decodedString.setText("ASR result:" + decoded);

         } catch (FileNotFoundException ex) {

         } catch (IOException ex) {

         } finally {

         }
         //this._tfliteStatus.setText("ASR cost: " + inferenceExecTime + " milliseconds");

         //this._startInference.setEnabled(true);
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


     public static native int kantv_anti_tamper();

 }
