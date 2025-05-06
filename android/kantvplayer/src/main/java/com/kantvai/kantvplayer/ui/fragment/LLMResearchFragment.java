 /*
  * Copyright (c) 2024- KanTV Authors
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  */
 package com.kantvai.kantvplayer.ui.fragment;

 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_FINALIZE;
 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_STOP;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.content.res.Resources;
 import android.os.Build;
 import android.view.View;
 import android.view.WindowManager;
 import android.widget.AdapterView;
 import android.widget.ArrayAdapter;
 import android.widget.Button;
 import android.widget.EditText;
 import android.widget.LinearLayout;
 import android.widget.Spinner;
 import android.widget.TextView;
 import android.text.method.ScrollingMovementMethod;

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
 import java.util.HashMap;
 import java.util.Map;
 import java.util.concurrent.atomic.AtomicBoolean;

 import butterknife.BindView;
 import kantvai.ai.KANTVAIModelMgr;
 import kantvai.ai.ggmljava;
 import kantvai.media.player.KANTVEvent;
 import kantvai.media.player.KANTVEventListener;
 import kantvai.media.player.KANTVEventType;
 import kantvai.media.player.KANTVException;
 import kantvai.media.player.KANTVLibraryLoader;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVMgr;
 import kantvai.media.player.KANTVUtils;


 //the feature in this UI page is exactly similar to AIResearchFragment.java
 //keep it for further usage
 public class LLMResearchFragment extends BaseMvpFragment<LLMResearchPresenter> implements LLMResearchView {
     @BindView(R.id.llmresearchLayout)
     LinearLayout layout;

     private static final String TAG = LLMResearchFragment.class.getName();
     TextView txtLLMInfo;
     TextView txtGGMLInfo;

     EditText txtUserInput;
     LinearLayout llInfoLayout;

     Button btnInference;
     Button btnStopInference;

     private int nThreadCounts = 4;
     private int nLLMType = 1;

     private String strLLMInferenceInfo;
     private String strBackend = "ggml";
     
     private int offset = 3;
     //TODO: the existing codes can't cover following special case:
     //      toggle backend and forth between QNN-NPU and cDSP and ggml in a standard Android APP or in
     //      a same running process, so here backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset
     //      supportive of such special case is easy but it will significantly increase the size of APK
     private int backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset;

     ArrayAdapter<String> adapterGGMLBackendType = null;

     Spinner spinnerBackendType = null;

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private long nLogCounts = 0;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);

     private String LLMModelFullName;
     private String LLMModelURL;
     String selectModelFilePath = "";
     private String strUserInput = "introduce the movie Once Upon a Time in America briefly, less then 100 words\n";
     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;
     private KANTVMgr mKANTVMgr = null;
     private LLMResearchFragment.MyEventListener mEventListener = new LLMResearchFragment.MyEventListener();
     private KANTVAIModelMgr LLMModelMgr = KANTVAIModelMgr.getInstance();

     private void initLLMModels() {
         LLMModelFullName = LLMModelMgr.getKANTVAIModelFromName("Gemma3-4B").getName();
         LLMModelURL = LLMModelMgr.getKANTVAIModelFromName("Gemma3-4B").getUrl();
     }

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

         txtLLMInfo = mActivity.findViewById(R.id.llmInfo);
         txtGGMLInfo = mActivity.findViewById(R.id.ggmlInfoLLM);
         btnInference = mActivity.findViewById(R.id.btnInference);
         btnStopInference      = mActivity.findViewById(R.id.btnStop);
         txtUserInput = mActivity.findViewById(R.id.txtUserInput);
         llInfoLayout = mActivity.findViewById(R.id.llLLMInfoLayout);
         txtLLMInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);

         initLLMModels();

         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         KANTVLog.j(TAG, "set ggml's llama.cpp info");
         setTextGGMLInfo(LLMModelFullName);

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
         spinnerThreadsCounts.setSelection(4); // 4 threads

         spinnerBackendType = mActivity.findViewById(R.id.spinnerLLMBackend);
         String[] arrayBackend = getResources().getStringArray(R.array.backendtype);
         adapterGGMLBackendType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBackend);
         spinnerBackendType.setAdapter(adapterGGMLBackendType);
         spinnerBackendType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
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
         spinnerBackendType.setSelection(ggmljava.HEXAGON_BACKEND_GGML - offset);

         btnStopInference.setOnClickListener(v -> {
             KANTVLog.g(TAG, "here");
             if (ggmljava.llm_is_running()) {
                 KANTVLog.g(TAG, "here");
                 ggmljava.llm_stop_inference();
             }
             restoreUIAndStatus();
         });

         btnInference.setOnClickListener(v -> {
             KANTVLog.j(TAG, "strModeName:" + LLMModelFullName);
             KANTVLog.j(TAG, "threads:" + nThreadCounts + ", model:" + LLMModelFullName + ", backend:" + strBackend);

             //sanity check begin
             {
                 if (!checkLLMModelExist())
                     return;

                 String strPrompt = txtUserInput.getText().toString();
                 if (strPrompt.isEmpty()) {
                     //KANTVUtils.showMsgBox(mActivity, "pls check your input");
                     //return;
                     strPrompt = strUserInput;
                 }
                 strPrompt = strPrompt.trim();
                 strUserInput = strPrompt;
                 KANTVLog.g(TAG, "User input: \n " + strUserInput);
             }
             //sanity check end

             KANTVLog.g(TAG, "model file:" + selectModelFilePath);

             nLogCounts = 0;

             initUIAndStatus();

             launchGGMLBenchmarkThread();
         });

         checkLLMModelExist();
         endTime = System.currentTimeMillis();
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     private final void launchGGMLBenchmarkThread() {
         Thread workThread = new Thread(new Runnable() {
             @RequiresApi(api = Build.VERSION_CODES.O)
             @Override
             public void run() {
                 strBenchmarkInfo = "";

                 initKANTVMgr();

                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();
                     ggmljava.llm_inference(
                             selectModelFilePath,
                             strUserInput,
                             nLLMType,
                             nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);

                     isBenchmarking.set(false);
                     mActivity.runOnUiThread(() -> {
                         restoreUIAndStatus();
                         txtLLMInfo.scrollTo(0, 0);
                         displayInferenceResult(strLLMInferenceInfo);
                         strLLMInferenceInfo = null;
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


     protected class MyEventListener implements KANTVEventListener {
         MyEventListener() {
         }

         @Override
         public void onEvent(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
             String eventString = "got event from native layer: " + eventType.toString() + " (" + what + ":" + arg1 + " ) :" + (String) obj;
             String content = (String) obj;

             if (eventType.getValue() == KANTVEvent.KANTV_ERROR) {
                 KANTVLog.j(TAG, "ERROR:" + eventString);
                 txtLLMInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP) || (arg1 == KANTV_INFO_ASR_FINALIZE)) {
                     KANTVLog.g(TAG, "return");
                     return;
                 }

                 if (content.startsWith("reset")) {
                     KANTVLog.g(TAG, "reset");
                     txtLLMInfo.setText("");
                     return;
                 }

                 //make UI happy when disable GGML_USE_HEXAGON manually
                 if (content.startsWith("ggml-hexagon")) {
                     txtLLMInfo.setText(content);
                     return;
                 }

                 if (content.startsWith("unknown")) {
                     restoreUIAndStatus();
                 } else {
                     if (content.startsWith("llama-timings")) {
                         KANTVLog.g(TAG, "LLM timings");
                         strLLMInferenceInfo = content;
                     } else {
                         nLogCounts++;
                         if (nLogCounts > 100) {
                             //_txtASRInfo.setText("");
                             nLogCounts = 0;
                         }

                         if (!ggmljava.llm_is_running()) {
                             return;
                         }

                         txtLLMInfo.append(content);
                         int offset = txtLLMInfo.getLineCount() * txtLLMInfo.getLineHeight();
                         int screenHeight = KANTVUtils.getScreenHeight();
                         int maxHeight = 500;
                         KANTVLog.j(TAG, "offset:" + offset);
                         KANTVLog.j(TAG, "screenHeight:" + screenHeight);
                         if (offset > maxHeight)
                             txtLLMInfo.scrollTo(0, offset - maxHeight);
                     }
                 }
             }
         }
     }

     private void initKANTVMgr() {
         if (mKANTVMgr != null) {
             release();
             mKANTVMgr = null;
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
         if (ggmljava.llm_is_running()) {
             ggmljava.llm_stop_inference();
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

     public void stopLLMInference() {
         if (ggmljava.llm_is_running()) {
             ggmljava.llm_stop_inference();
         }

         restoreUIAndStatus();
         txtLLMInfo.setText("");
         txtUserInput.setText("introduce the movie Once Upon a Time in America briefly, less then 100 words\n");
     }

     private void initUIAndStatus() {
         isBenchmarking.set(true);
         //Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();

         txtLLMInfo.setText("");
         btnInference.setEnabled(false);
         btnInference.setBackgroundColor(0xffa9a9a9);

         WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
         attributes.screenBrightness = 1.0f;
         mActivity.getWindow().setAttributes(attributes);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
     }

     private void restoreUIAndStatus() {
         isBenchmarking.set(false);
         btnInference.setEnabled(true);
         btnInference.setBackgroundColor(0xC3009688);
     }

     private void displayInferenceResult(String content) {
         if (null == content)
             return;

         if (strBenchmarkInfo.startsWith("unknown")) {
             KANTVLog.g(TAG, "unknown");
             restoreUIAndStatus();
             return;
         }
         String backendDesc = KANTVUtils.getGGMLBackendDesc(backendIndex);

         String benchmarkTip =  " (model: " + LLMModelFullName
                 + " ,threads: " + nThreadCounts
                 + " ,backend: " + backendDesc
                 + " ) cost " + duration + " milliseconds";
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         benchmarkTip += ", on " + timestamp;

         benchmarkTip += "\n";
         if (!strBenchmarkInfo.startsWith("unknown")) {
             benchmarkTip += strBenchmarkInfo;
         }

         KANTVLog.j(TAG, benchmarkTip);

         String dispInfo;

         dispInfo = KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_LLM);
         dispInfo += "\n\n";

         benchmarkTip += "\n";
         dispInfo += benchmarkTip;
         dispInfo += "\n";

         if (content != null) {
             dispInfo += content;
         }
         KANTVUtils.showMsgBox(mActivity, dispInfo);
     }

     private boolean checkLLMModelExist() {
         File selectModeFile = null;
         KANTVLog.g(TAG, "selectModeFileName:" + LLMModelFullName);
         selectModelFilePath = KANTVUtils.getSDCardDataPath() + LLMModelFullName;
         KANTVLog.g(TAG, "selectModelFilePath:" + selectModelFilePath);
         selectModeFile = new File(selectModelFilePath);
         if (!selectModeFile.exists()) {
             KANTVUtils.showMsgBox(mActivity, "pls check whether model file:" +
                     selectModeFile.getAbsolutePath() + " exist and down from " + LLMModelURL);
             return false;
         }
         return true;
     }
     private void setTextGGMLInfo(String LLMModelFullName) {
         txtGGMLInfo.setText("");
         txtGGMLInfo.append(KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_LLM));
         txtGGMLInfo.append("\n" + "LLM model:" + LLMModelFullName);
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         txtGGMLInfo.append("\n");
         txtGGMLInfo.append("running timestamp:" + timestamp);
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
