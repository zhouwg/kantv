 /*
  * Copyright (c) 2024- KanTV Author
  *
  * 03-26-2024, zhouwg, this is a skeleton/initial UI for study llama.cpp on Android phone
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
 package com.kantvai.kantvplayer.ui.fragment;

 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_FINALIZE;
 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_STOP;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.app.AlertDialog;
 import android.app.ProgressDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.res.Resources;
 import android.os.Build;
 import android.text.method.ScrollingMovementMethod;
 import android.view.View;
 import android.view.WindowManager;
 import android.widget.AdapterView;
 import android.widget.ArrayAdapter;
 import android.widget.Button;
 import android.widget.EditText;
 import android.widget.LinearLayout;
 import android.widget.Spinner;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;

 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.base.BaseMvpFragment;
 import com.kantvai.kantvplayer.mvp.impl.LLMResearchPresenterImpl;
 import com.kantvai.kantvplayer.mvp.presenter.LLMResearchPresenter;
 import com.kantvai.kantvplayer.mvp.view.LLMResearchView;
 import com.kantvai.kantvplayer.utils.Settings;

 import java.io.File;
 import java.text.SimpleDateFormat;
 import java.util.Date;
 import java.util.concurrent.atomic.AtomicBoolean;

 import butterknife.BindView;
 import kantvai.ai.ggmljava;

 import kantvai.media.player.KANTVLibraryLoader;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;
 import kantvai.media.player.KANTVEvent;
 import kantvai.media.player.KANTVEventListener;
 import kantvai.media.player.KANTVEventType;
 import kantvai.media.player.KANTVException;
 import kantvai.media.player.KANTVMgr;


 public class LLMResearchFragment extends BaseMvpFragment<LLMResearchPresenter> implements LLMResearchView {
     @BindView(R.id.ggmlLayoutLLM)
     LinearLayout layout;

     private static final String TAG = LLMResearchFragment.class.getName();
     TextView _txtLLMInfo;
     TextView _txtGGMLInfo;
     EditText _txtUserInput;
     Button _btnInference;

     private int nThreadCounts = 8;
     private int benchmarkIndex = 0;

     private String strBackend = "ggml";

     private int offset = 3;
     //TODO: the existing codes can't cover following special case:
     //      toggle backend and forth between QNN-NPU and cDSP and ggml in a standard Android APP or in
     //      a same running process, so here backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset
     //      supportive of such special case is easy but it will significantly increase the size of APK
     private int backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset;

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private String strLLMInferenceInfo;

     private String strUserInput = "introduce the movie Once Upon a Time in America briefly, less then 100 words.";

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);

     // https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_0.gguf   //1.1 GB
     private String ggmlModelFileName = "qwen1_5-1_8b-chat-q4_0.gguf";
     private String selectModelFileName;

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private KANTVMgr mKANTVMgr = null;
     private LLMResearchFragment.MyEventListener mEventListener = new LLMResearchFragment.MyEventListener();

     public static LLMResearchFragment newInstance() {
         return new LLMResearchFragment();
     }

     @NonNull
     @Override
     protected LLMResearchPresenter initPresenter() {
         return new LLMResearchPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_llm;
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

         _txtLLMInfo = (TextView) mActivity.findViewById(R.id.llmInfo);
         _txtGGMLInfo = (TextView) mActivity.findViewById(R.id.ggmlInfoLLM);

         //TODO: change to voice input, and then use whisper.cpp to convert it into text
         _txtUserInput = (EditText) mActivity.findViewById(R.id.txtUserInput);

         _btnInference = (Button) mActivity.findViewById(R.id.btnInference);

         _txtLLMInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
         _txtLLMInfo.setMovementMethod(ScrollingMovementMethod.getInstance());
         selectModelFileName = KANTVUtils.getSDCardDataPath() + ggmlModelFileName;
         displayFileStatus(selectModelFileName);

         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         try {
             initKANTVMgr();
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         KANTVLog.j(TAG, "load ggml's llama.cpp info");
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_LLM));

         Spinner spinnerThreadsCounts = mActivity.findViewById(R.id.spinnerLLMThreadCounts);
         String[] arrayThreadCounts = getResources().getStringArray(R.array.threadCounts);
         ArrayAdapter<String> adapter = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayThreadCounts);
         spinnerThreadsCounts.setAdapter(adapter);
         spinnerThreadsCounts.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "thread counts:" + arrayThreadCounts[position]);
                 nThreadCounts = Integer.valueOf(arrayThreadCounts[position]);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerThreadsCounts.setSelection(4);

         Spinner spinnerBackend = mActivity.findViewById(R.id.spinnerLLMBackend);
         String[] arrayBackend = getResources().getStringArray(R.array.backendtype);
         ArrayAdapter<String> adapterBackend = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBackend);
         spinnerBackend.setAdapter(adapterBackend);
         spinnerBackend.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "backend:" + arrayBackend[position]);
                 strBackend = arrayBackend[position];
                 backendIndex = Integer.valueOf(position) + offset;
                 KANTVLog.j(TAG, "strBackend:" + strBackend);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });

         spinnerBackend.setSelection(ggmljava.HEXAGON_BACKEND_GGML - offset);

         _btnInference.setOnClickListener(v -> {
             String strPrompt = _txtUserInput.getText().toString();

             //sanity check begin
             if (strPrompt.isEmpty()) {
                 //KANTVUtils.showMsgBox(mActivity, "pls check your input");
                 //return;
                 //just for test
                 strPrompt = strUserInput;
             }
             strPrompt = strPrompt.trim();
             strUserInput = strPrompt;
             KANTVLog.j(TAG, "User input: \n " + strUserInput);

             KANTVLog.j(TAG, "strModeName:" + ggmlModelFileName);

             File selectModeFile = new File(selectModelFileName);
             if (!selectModeFile.exists()) {
                 KANTVLog.j(TAG, "model file not exist:" + selectModeFile.getAbsolutePath());
             }
             displayFileStatus(selectModelFileName);

             if (!selectModeFile.exists()) {
                 KANTVUtils.showMsgBox(mActivity, "pls check whether GGML's model file exist in /sdcard/");
                 return;
             }
             //sanity check end

             KANTVLog.j(TAG, "model file:" + selectModelFileName);

             initUIAndStatus();

             launchGGMLBenchmarkThread();
         });

         endTime = System.currentTimeMillis();
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     private final void launchGGMLBenchmarkThread() {
         Thread workThread = new Thread(new Runnable() {
             @RequiresApi(api = Build.VERSION_CODES.O)
             @Override
             public void run() {
                 strBenchmarkInfo = "";

                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();

                     strBenchmarkInfo = ggmljava.llm_inference(
                             selectModelFileName,
                             strUserInput,
                             benchmarkIndex,
                             nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);
                     isBenchmarking.set(false);

                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             restoreUIAndStatus();
                             displayInferenceResult(strLLMInferenceInfo);
                         }
                     });
                 }
             }
         });
         workThread.start();
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


     private void displayFileStatus(String modelFilePath) {
         File modelFile = new File(modelFilePath);
         if (modelFile.exists()) {
             KANTVLog.j(TAG, "model   file exist:" + modelFile.getAbsolutePath());
         } else {
             KANTVLog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
             KANTVUtils.showMsgBox(mActivity, "model   file not exist: " + modelFile.getAbsolutePath());
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
                 KANTVLog.j(TAG, "ERROR:" + eventString);
                 _txtLLMInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP) || (arg1 == KANTV_INFO_ASR_FINALIZE)
                 ) {
                     return;
                 }

                 if (content.startsWith("unknown")) {
                     restoreUIAndStatus();
                 } else {
                     if (content.startsWith("llama-timings")) {
                         strLLMInferenceInfo = content;
                     } else {
                         _txtLLMInfo.append(content);
                         int offset = _txtLLMInfo.getLineCount() * _txtLLMInfo.getLineHeight();
                         if (offset > _txtLLMInfo.getHeight())
                             _txtLLMInfo.scrollTo(0, offset - _txtLLMInfo.getHeight());
                     }
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
             KANTVLog.j(TAG, "KANTVMgr version:" + mKANTVMgr.getMgrVersion());
         } catch (KANTVException ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             KANTVLog.j(TAG, "error occurred: " + errorMsg);
             KANTVUtils.showMsgBox(mActivity, errorMsg);
             ex.printStackTrace();
         }
     }

     public void release() {
         if (mKANTVMgr == null) {
             return;
         }

         try {
             KANTVLog.j(TAG, "release");
             {
                 mKANTVMgr.finalizeASR();
                 mKANTVMgr.stopASR();
                 mKANTVMgr.release();
                 mKANTVMgr = null;
             }
         } catch (Exception ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             KANTVLog.j(TAG, "error occurred: " + errorMsg);
             ex.printStackTrace();
         }
     }

     private void initUIAndStatus() {
         //Toast.makeText(mContext, "LLM inference is launched", Toast.LENGTH_LONG).show();
         _txtLLMInfo.setText("");
         isBenchmarking.set(true);
         _btnInference.setEnabled(false);
         _btnInference.setBackgroundColor(0xffa9a9a9);

         WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
         attributes.screenBrightness = 1.0f;
         mActivity.getWindow().setAttributes(attributes);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
     }

     private void restoreUIAndStatus() {
         isBenchmarking.set(false);
         _btnInference.setEnabled(true);
         _btnInference.setBackgroundColor(0xC3009688);
     }

     private void displayInferenceResult(String content) {
         String dispInfo = KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_LLM);
         dispInfo += "\n\n";

         String benchmarkTip = "LLM inference " + "(model: " + ggmlModelFileName
                 + " ,threads: " + nThreadCounts + " , backend: " + KANTVUtils.getGGMLBackendDesc(backendIndex)
                 + " ) cost " + duration + " milliseconds";
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         benchmarkTip += ", on " + timestamp;

         if (!strBenchmarkInfo.startsWith("unknown")) {
             benchmarkTip += "\n";
             benchmarkTip += strBenchmarkInfo;
         }
         benchmarkTip += "\n";
         dispInfo += benchmarkTip;
         dispInfo += "\n";

         dispInfo += content;
         KANTVUtils.showMsgBox(mActivity, dispInfo);
     }
 }
