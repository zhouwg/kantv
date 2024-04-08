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


 import static org.ggml.ggmljava.GGML_JNI_OP_ADD;
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
 import com.cdeos.kantv.mvp.impl.ASRResearchPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.ASRResearchPresenter;
 import com.cdeos.kantv.mvp.view.ASRResearchView;
 import com.cdeos.kantv.utils.Settings;


 import org.ggml.ggmljava;

 import java.io.File;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.io.RandomAccessFile;
 import java.nio.ByteBuffer;
 import java.nio.ByteOrder;
 import java.text.SimpleDateFormat;
 import java.util.Date;
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


 public class ASRResearchFragment extends BaseMvpFragment<ASRResearchPresenter> implements ASRResearchView {
     @BindView(R.id.ggmlLayout)
     LinearLayout layout;

     private static final String TAG = ASRResearchFragment.class.getName();
     TextView _txtASRInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;

     Button _btnBenchmark;

     private int nThreadCounts = 1;
     private int benchmarkIndex = 0;
     private String strModeName = "tiny.en-q8_0";
     private String strBackend = "cpu";
     private int backendIndex = 0; //CPU

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private long nLogCounts = 0;
     private boolean isLLMModel = false;
     private boolean isQNNModel = false;
     private boolean isSDModel=false;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     //private String ggmlModelFileName = "ggml-tiny-q5_1.bin"; //31M
     private String ggmlModelFileName  = "ggml-tiny.en-q8_0.bin";//42M, ggml-tiny.en-q8_0.bin is preferred
     private String ggmlSampleFileName = "jfk.wav";

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private KANTVMgr mKANTVMgr = null;
     private ASRResearchFragment.MyEventListener mEventListener = new ASRResearchFragment.MyEventListener();


     public static ASRResearchFragment newInstance() {
         return new ASRResearchFragment();
     }

     @NonNull
     @Override
     protected ASRResearchPresenter initPresenter() {
         return new ASRResearchPresenterImpl(this, this);
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


         //copy asset files to /sdcard/kantv/
         //or just upload dependent files to /sdcard/kantv/ accordingly so the APK size would be smaller significantly
         CDEAssetLoader.copyAssetFile(mContext, ggmlModelFileName, CDEUtils.getDataPath() + ggmlModelFileName);
         CDEAssetLoader.copyAssetFile(mContext, ggmlSampleFileName, CDEUtils.getDataPath() + ggmlSampleFileName);


         //add the qnn binary files to facilitate other developers to reproduce the qnn sample on Qualcomm SoC based android phone(Xiaomi 14 is preferred)
         //qualcomm's dedicated model file
         CDEAssetLoader.copyAssetFile(mContext, "libInception_v3.so", CDEUtils.getDataPath(mContext) + "libInception_v3.so");
         //qualcomm's prebuilt QNN userspace library
         CDEAssetLoader.copyAssetFile(mContext, "libQnnCpu.so", CDEUtils.getDataPath(mContext) + "libQnnCpu.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnGpu.so", CDEUtils.getDataPath(mContext) + "libQnnGpu.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnDsp.so", CDEUtils.getDataPath(mContext) + "libQnnDsp.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnHtp.so", CDEUtils.getDataPath(mContext) + "libQnnHtp.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnHtpV75Stub.so", CDEUtils.getDataPath(mContext) + "libQnnHtpV75Stub.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnHtpV75CalculatorStub.so", CDEUtils.getDataPath(mContext) + "libQnnHtpV75CalculatorStub.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnSystem.so", CDEUtils.getDataPath(mContext) + "libQnnSystem.so");
         CDEAssetLoader.copyAssetFile(mContext, "libQnnSaver.so", CDEUtils.getDataPath(mContext) + "libQnnSaver.so");
         CDEAssetLoader.copyAssetFile(mContext, "params.bin", CDEUtils.getDataPath() + "params.bin");
         //qualcomm's prebuilt binary file
         CDEAssetLoader.copyAssetFile(mContext, "raw_list.txt", CDEUtils.getDataPath() + "raw_list.txt");
         CDEAssetLoader.copyAssetDir(mContext, "data", CDEUtils.getDataPath() + "data");


         _txtASRInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
         displayFileStatus(CDEUtils.getDataPath() + ggmlSampleFileName, CDEUtils.getDataPath() + ggmlModelFileName);

         try {
             CDELibraryLoader.load("ggml-jni");
             CDELog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         try {
             initKANTVMgr();
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         CDELog.j(TAG, "load ggml's whispercpp info");
         String systemInfo = ggmljava.asr_get_systeminfo();
         String phoneInfo = "Device info:" + "\n"
                 + "Brand:" + Build.BRAND + "\n"
                 + "Hardware:" + Build.HARDWARE + "\n"
                 /*+ "Fingerprint:" + Build.FINGERPRINT + "\n"*/ /* pls don't uncomment this line in public project */
                 + "OS:" + "Android " + android.os.Build.VERSION.RELEASE + "\n"
                 + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")";
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(phoneInfo + "\n");
         _txtGGMLInfo.append("Powered by GGML(Georgi Gerganov Machine Learning)(https://github.com/ggerganov/ggml)\n");


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

                 CDELog.j(TAG, "strModeName:" + strModeName);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });


         Spinner spinnerBackend = mActivity.findViewById(R.id.spinnerBackend);
         String[] arrayBackend = getResources().getStringArray(R.array.backend);
         ArrayAdapter<String> adapterBackend = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBackend);
         spinnerBackend.setAdapter(adapterBackend);
         spinnerBackend.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "backend:" + arrayBackend[position]);
                 strBackend = arrayBackend[position];
                 backendIndex = Integer.valueOf(position);
                 CDELog.j(TAG, "strBackend:" + strBackend);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });

         _btnBenchmark.setOnClickListener(v -> {
             CDELog.j(TAG, "strModeName:" + strModeName);
             CDELog.j(TAG, "exec ggml benchmark: type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex)
                     + ", threads:" + nThreadCounts + ", model:" + strModeName + ", backend:" + strBackend);
             String selectModeFileName = "";
             String selectModelFilePath = "";
             File selectModeFile = null;

             isLLMModel = false;
             isQNNModel = false;
             isSDModel  = false;

             //TODO: better method
             //sanity check begin
             if (strModeName.contains("llama")) {
                 isLLMModel = true;
             } else if (strModeName.contains("qwen")) {
                 isLLMModel = true;
             } else if (strModeName.contains("baichuan")) {
                 isLLMModel = true;
             } else if (strModeName.contains("gemma")) {
                 isLLMModel = true;
             } else if (strModeName.startsWith("qnn")) {
                 isQNNModel = true;
            } else if (strModeName.startsWith("sdmodel")) //TODO:hardcode
                isSDModel = true;

             CDELog.j(TAG, "isSDModel:" + isSDModel);
             if (isLLMModel)
                 selectModeFileName = strModeName + ".gguf";
             else if (isQNNModel)
                 //qualcomm's dedicated model:a model is a dynamic so and it's generated by Qualcomm special tools
                 selectModeFileName =  "libInception_v3.so"; //TODO:hardcode in PoC stage
             else if (isSDModel)
                 //https://github.com/leejet/stable-diffusion.cpp
                 //curl -L -O https://huggingface.co/stabilityai/stable-diffusion-2-1/resolve/main/v2-1_768-nonema-pruned.safetensors
                 //sd -M convert -m v2-1_768-nonema-pruned.safetensors -o  v2-1_768-nonema-pruned.q8_0.gguf -v --type q8_0
                 selectModeFileName = "v2-1_768-nonema-pruned.q8_0.gguf"; //TODO: hardcode SD model name
             else
                 selectModeFileName = "ggml-" + strModeName + ".bin";


             if (isLLMModel && (benchmarkIndex != CDEUtils.BENCHMARK_LLM)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }
             if ((!isLLMModel) && (benchmarkIndex == CDEUtils.BENCHMARK_LLM)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }

             if (isSDModel && (benchmarkIndex != CDEUtils.BENCHMARK_STABLEDIFFUSION)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }
             if ((!isSDModel) && (benchmarkIndex == CDEUtils.BENCHMARK_STABLEDIFFUSION)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }

             if (isQNNModel && (benchmarkIndex < CDEUtils.BENCHMARK_QNN_SAMPLE)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }
             if (!isQNNModel && ((benchmarkIndex >= CDEUtils.BENCHMARK_QNN_SAMPLE) && (benchmarkIndex < CDEUtils.BENCHMARK_STABLEDIFFUSION))) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }

             if (!isQNNModel)
                 selectModelFilePath = CDEUtils.getDataPath() + selectModeFileName;
             else
                 selectModelFilePath = CDEUtils.getDataPath(mContext) + selectModeFileName;

             CDELog.j(TAG, "selectModelFilePath:" + selectModelFilePath);
             selectModeFile = new File(selectModelFilePath);
             displayFileStatus(CDEUtils.getDataPath() + ggmlSampleFileName, selectModelFilePath);

             if (!selectModeFile.exists()) {
                 CDELog.j(TAG, "model file not exist:" + selectModeFile.getAbsolutePath());
             }
             File sampleFile = new File(CDEUtils.getDataPath() + ggmlSampleFileName);

             if (!selectModeFile.exists() || (!sampleFile.exists())) {
                 CDEUtils.showMsgBox(mActivity, "pls check whether GGML's model file:" + selectModeFileName + " and sample file(jfk.wav) exist in /sdcard/kantv/");
                 return;
             }
             //sanity check end

             //reset default ggml model file name after sanity check
             ggmlModelFileName = selectModeFileName;
             CDELog.j(TAG, "model file:" + CDEUtils.getDataPath() + selectModeFileName);
             if (!isQNNModel) {
                 ggmljava.asr_reset(CDEUtils.getDataPath() + selectModeFileName, ggmljava.get_cpu_core_counts() / 2, CDEUtils.ASR_MODE_BECHMARK);
             }
             if (benchmarkIndex == CDEUtils.BECHMARK_ASR) {
                 //playAudioFile();
             }
             nLogCounts = 0;
             isBenchmarking.set(true);

             startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating) + "(" + CDEUtils.getBenchmarkDesc(benchmarkIndex) + ")");

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
                 if (!isQNNModel) {
                     ggmljava.asr_set_benchmark_status(0);
                 }


                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();
                     if (!isQNNModel) {
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 CDEUtils.getDataPath() + ggmlModelFileName,
                                 CDEUtils.getDataPath() + ggmlSampleFileName,
                                 benchmarkIndex,
                                 nThreadCounts, 0, 0);
                     } else {
                         // avoid following issue
                         // dlopen failed: library "/sdcard/kantv/libInception_v3.so" needed or dlopened by
                         // "/data/app/~~70peMvcNIhRmzhm-PhmfRg==/com.cdeos.kantv-bUwy7gbMeCP0JFLe1J058g==/base.apk!/lib/arm64-v8a/libggml-jni.so"
                         // is not accessible for the namespace "clns-4"
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 CDEUtils.getDataPath(mContext) + ggmlModelFileName,
                                 CDEUtils.getDataPath() + ggmlSampleFileName,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, GGML_JNI_OP_ADD);
                     }
                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);

                     isBenchmarking.set(false);
                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String benchmarkTip = "Bench:" + CDEUtils.getBenchmarkDesc(benchmarkIndex) + " (model: " + strModeName
                                     + " ,threads: " + nThreadCounts
                                     + " ) cost " + duration + " milliseconds";
                             //04-07-2024(April,7,2024), add timestamp
                             String timestamp = "";
                             SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
                             Date date = new Date(System.currentTimeMillis());
                             timestamp = fullDateFormat.format(date);
                             benchmarkTip += ", on " + timestamp;

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
                                 ggmljava.asr_set_benchmark_status(1);
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

                 if (content.startsWith("reset")) {
                     _txtASRInfo.setText("");
                     return;
                 }

                 if (content.startsWith("unknown")) {

                 } else {
                     nLogCounts++;
                     if (nLogCounts > 100) {
                         _txtASRInfo.setText(""); //make QNN SDK happy on Xiaomi14
                         nLogCounts = 0;
                     }
                     _txtASRInfo.append(content);
                 }
             }
         }
     }


     private void initKANTVMgr() {
         if (mKANTVMgr != null) {
             return;
         }

         try {
             mKANTVMgr = new KANTVMgr(mEventListener);
             if (mKANTVMgr != null) {
                 mKANTVMgr.initASR();
                 mKANTVMgr.startASR();
             }
             CDELog.j(TAG, "KANTVMgr version:" + mKANTVMgr.getMgrVersion());
         } catch (KANTVException ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             CDELog.j(TAG, "error occurred: " + errorMsg);
             CDEUtils.showMsgBox(mActivity, errorMsg);
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

     public static native int kantv_anti_remove_rename_this_file();
 }
