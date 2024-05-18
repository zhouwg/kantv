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
 import android.widget.EditText;
 import android.widget.LinearLayout;
 import android.widget.Spinner;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;

 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.mvp.impl.AIResearchPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.AIResearchPresenter;
 import com.cdeos.kantv.mvp.view.AIResearchView;
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


 public class AIResearchFragment extends BaseMvpFragment<AIResearchPresenter> implements AIResearchView {
     @BindView(R.id.airesearchLayout)
     LinearLayout layout;

     private static final String TAG = AIResearchFragment.class.getName();
     TextView _txtASRInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;
     EditText _txtUserInput;

     Button _btnBenchmark;

     private int nThreadCounts = 1;
     private int benchmarkIndex = 0; //0: asr 12: qnn-ggml-op, make "PoC-S49: implementation of other GGML OP(non-mulmat) using QNN API" happy
     private int previousBenchmakrIndex = 0;
     private String strModeName = "tiny.en-q8_0";
     private String strBackend = "cpu";
     private int backendIndex = 0; //CPU
     private String strOPType = "add";
     private int optypeIndex = 0; //matrix addition operation
     private String selectModeFileName = "";

     Spinner spinnerOPType = null;
     String[] arrayOPType = null;
     String[] arrayGraphType = null;
     ArrayAdapter<String> adapterOPType = null;
     ArrayAdapter<String> adapterGraphType = null;


     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private long nLogCounts = 0;
     private boolean isLLMModel = false;
     private boolean isQNNModel = false;
     private boolean isSDModel = false;
     private boolean isMNISTModel = false;
     private boolean isTTSModel = false;
     private boolean isASRModel = false;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     //private String ggmlModelFileName = "ggml-tiny-q5_1.bin"; //31M
     private String ggmlModelFileName = "ggml-tiny.en-q8_0.bin";//42M, ggml-tiny.en-q8_0.bin is preferred
     private String ggmlSampleFileName = "jfk.wav";

     // https://huggingface.co/TheBloke/Llama-2-13B-chat-GGUF/tree/main
     // https://huggingface.co/TheBloke/Llama-2-7B-GGUF
     // https://huggingface.co/TheBloke/Llama-2-13B-GGUF
     // https://huggingface.co/TheBloke/Llama-2-70B-GGUF
     // https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF
     // https://huggingface.co/TheBloke/Llama-2-13B-chat-GGUF
     // https://huggingface.co/TheBloke/Llama-2-70B-Chat-GGUF

     // https://huggingface.co/Xorbits/Qwen-7B-Chat-GGUF/blob/main/Qwen-7B-Chat.Q4_K_M.gguf  //4.9 GB

     // https://huggingface.co/XeIaso/yi-chat-6B-GGUF/tree/main
     // https://huggingface.co/XeIaso/yi-chat-6B-GGUF/blob/main/yi-chat-6b.Q2_K.gguf //2.62 GB
     // https://huggingface.co/XeIaso/yi-chat-6B-GGUF/blob/main/yi-chat-6b.Q4_0.gguf //3.48 GB


     // https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_0.gguf   //1.1 GB


     // https://huggingface.co/TheBloke/blossom-v3-baichuan2-7B-GGUF
     // https://huggingface.co/shaowenchen/baichuan2-7b-chat-gguf
     // https://huggingface.co/TheBloke/blossom-v3-baichuan2-7B-GGUF/blob/main/blossom-v3-baichuan2-7b.Q4_K_M.gguf // 4.61 GB


     // https://huggingface.co/mlabonne/gemma-2b-GGUF/tree/main
     // https://huggingface.co/mlabonne/gemma-2b-GGUF/resolve/main/gemma-2b.Q4_K_M.gguf  // 1.5 GB
     // https://huggingface.co/mlabonne/gemma-2b-GGUF/resolve/main/gemma-2b.Q8_0.gguf    // 2.67 GB

     // https://huggingface.co/TheBloke/Yi-34B-Chat-GGUF/tree/main
     // https://huggingface.co/second-state/Yi-34B-Chat-GGUF/blob/a93fd377944179153cc8477eef2e69645c1a8ff9/Yi-34B-Chat-Q2_K.gguf // 12.8 GB


     //private String ggmlModelFileName = "llama-2-7b.Q4_K_M.gguf";    //4.08 GB
     //private String ggmlModelFileName = "llama-2-7b-chat.Q4_K_M.gguf"; //4.08 GB
     //private String ggmlModelFileName = "qwen1_5-1_8b-chat-q4_0.gguf"; // 1.1 GB
     //private String ggmlModelFileName = "baichuan2-7b.Q4_K_M.gguf"; // 4.61 GB
     //private String ggmlModelFileName = "gemma-2b.Q4_K_M.gguf";  // 1.5 GB

     //https://huggingface.co/ggerganov/gemma-2b-Q8_0-GGUF/resolve/main/gemma-2b.Q8_0.gguf // 2.67 GB
     //private String ggmlModelFileName = "gemma-2b.Q8_0.gguf";    // 2.67 GB
     private String strUserInput = "introduce the movie Once Upon a Time in America briefly.\n";

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private KANTVMgr mKANTVMgr = null;
     private AIResearchFragment.MyEventListener mEventListener = new AIResearchFragment.MyEventListener();


     public static AIResearchFragment newInstance() {
         return new AIResearchFragment();
     }

     @NonNull
     @Override
     protected AIResearchPresenter initPresenter() {
         return new AIResearchPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_airesearch;
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
         //TODO: change to voice input, and then use whisper.cpp to convert it into text
         _txtUserInput = (EditText) mActivity.findViewById(R.id.txtPrompt);

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
         String phoneInfo = "Device info:" + " "
                 + Build.BRAND + " "
                 + Build.HARDWARE + " "
                 + "Android " + android.os.Build.VERSION.RELEASE + " "
                 + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")";
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(phoneInfo + "\n");
         _txtGGMLInfo.append("Powered by GGML(https://github.com/ggerganov/ggml)\n");

         Spinner spinnerBenchType = mActivity.findViewById(R.id.spinnerBenchType);
         String[] arrayBenchType = getResources().getStringArray(R.array.benchType);
         ArrayAdapter<String> adapterBenchType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBenchType);
         spinnerBenchType.setAdapter(adapterBenchType);
         spinnerBenchType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "bench type:" + arrayBenchType[position]);
                 benchmarkIndex = Integer.valueOf(position);
                 CDELog.j(TAG, "benchmark index:" + benchmarkIndex);

                 //if ((previousBenchmakrIndex != CDEUtils.BENCHMARK_QNN_COMPLEX) && (benchmarkIndex != CDEUtils.BENCHMARK_QNN_COMPLEX)) {
                 //    previousBenchmakrIndex = benchmarkIndex;
                 //    return;
                 //}

                 spinnerOPType.setAdapter(adapterOPType);

                 adapterOPType.notifyDataSetChanged();

                 previousBenchmakrIndex = benchmarkIndex;
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerBenchType.setSelection(CDEUtils.BENCHMARK_ASR);

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
         spinnerThreadsCounts.setSelection(4);

         Spinner spinnerModelName = mActivity.findViewById(R.id.spinnerModelName);
         String[] arrayModelName = getResources().getStringArray(R.array.modelName);
         ArrayAdapter<String> adapterModel = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayModelName);
         spinnerModelName.setAdapter(adapterModel);
         spinnerModelName.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "position: " + position + ", model:" + arrayModelName[position]);
                 strModeName = arrayModelName[position];

                 CDELog.j(TAG, "strModeName:" + strModeName);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerModelName.setSelection(3);


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
         spinnerBackend.setSelection(3);

         spinnerOPType = mActivity.findViewById(R.id.spinnerOPType);
         arrayGraphType = getResources().getStringArray(R.array.graphtype);

         {
             int max_idx = ggmljava.ggml_op.valueOf("GGML_OP_MUL_MAT").ordinal();
             arrayOPType = new String[max_idx + 1];
             ggmljava.ggml_op[] ggmlops = ggmljava.ggml_op.values();
             int idx = 0;
             for (ggmljava.ggml_op op : ggmlops) {
                 CDELog.j(TAG, "ggml op index:" + op.ordinal() + ", name:" + op.name());
                 //if (op.name().contains("GGML_OP_NONE"))
                 //    continue;

                 arrayOPType[idx] = op.name();
                 idx++;

                 if (op.name().contains("GGML_OP_MUL_MAT")) {
                     break;
                 }
             }
         }
         spinnerOPType.setSelection(0);

         adapterOPType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayOPType);
         adapterGraphType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayGraphType);

         spinnerOPType.setAdapter(adapterOPType);

         spinnerOPType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 CDELog.j(TAG, "GGML op type:" + arrayOPType[position]);
                 strOPType = arrayOPType[position];
                 optypeIndex = Integer.valueOf(position);
                 CDELog.j(TAG, "strOPType:" + strOPType);
                 CDELog.j(TAG, "optypeIndex:" + optypeIndex);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });

         _btnBenchmark.setOnClickListener(v -> {
             CDELog.j(TAG, "strModeName:" + strModeName);
             CDELog.j(TAG, "exec ggml benchmark: type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex)
                     + ", threads:" + nThreadCounts + ", model:" + strModeName + ", backend:" + strBackend);
             //String selectModeFileName = "";
             String selectModelFilePath = "";
             File selectModeFile = null;

             isLLMModel = false;
             isQNNModel = false;
             isSDModel = false;
             isMNISTModel = false;
             isTTSModel = false;
             isASRModel = false;

             //TODO: better method
             //sanity check begin
             if (strModeName.contains("llama")) {
                 isLLMModel = true;
                 selectModeFileName = "llama-2-7b-chat.Q4_K_M.gguf";
             } else if (strModeName.contains("qwen")) {
                 isLLMModel = true;
                 selectModeFileName = "qwen1_5-1_8b-chat-q4_0.gguf";
             } else if (strModeName.contains("baichuan")) {
                 isLLMModel = true;
                 selectModeFileName = "baichuan2-7b.Q4_K_M.gguf";
             } else if (strModeName.contains("gemma")) {
                 isLLMModel = true;
                 selectModeFileName = "gemma-2b.Q4_K_M.gguf";
                 selectModeFileName = "gemma-2b.Q8_0.gguf";
             } else if (strModeName.contains("yi-chat")) {
                 isLLMModel = true;
                 selectModeFileName = "yi-chat-6b.Q2_K.gguf";
                 selectModeFileName = "yi-chat-6b.Q4_0.gguf";
             } else if (strModeName.startsWith("qnn")) {
                 //not used since v1.3.8, but keep it for future usage because Qualcomm provide some prebuilt dedicated QNN models
                 isQNNModel = true;
             } else if (strModeName.startsWith("mnist")) {
                 isMNISTModel = true;
                 selectModeFileName = "mnist-ggml-model-f32.gguf";
             } else if (strModeName.startsWith("sdmodel")) {
                 isSDModel = true;
                 //https://github.com/leejet/stable-diffusion.cpp
                 //curl -L -O https://huggingface.co/stabilityai/stable-diffusion-2-1/resolve/main/v2-1_768-nonema-pruned.safetensors
                 //sd -M convert -m v2-1_768-nonema-pruned.safetensors -o  v2-1_768-nonema-pruned.q8_0.gguf -v --type q8_0
                 selectModeFileName = "v2-1_768-nonema-pruned.q8_0.gguf";
             } else if (strModeName.contains("bark")) {
                 isTTSModel = true;
                 selectModeFileName = "ggml-bark-small.bin";
             } else {
                 isASRModel = true;
                 selectModeFileName = "ggml-" + strModeName + ".bin";
             }
             CDELog.j(TAG, "selectModeFileName:" + selectModeFileName);

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

             if (isTTSModel && (benchmarkIndex != CDEUtils.BENCHMARK_TTS)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }
             if (!isTTSModel && (benchmarkIndex == CDEUtils.BENCHMARK_TTS)) {
                 CDEUtils.showMsgBox(mActivity, "mismatch between model file:" + selectModeFileName + " and bench type: " + CDEUtils.getBenchmarkDesc(benchmarkIndex));
                 return;
             }

             if (!isQNNModel)
                 selectModelFilePath = CDEUtils.getDataPath() + selectModeFileName;
             else {
                 //not used since v1.3.8, but keep it for future usage because Qualcomm provide some prebuilt dedicated QNN models
                 selectModelFilePath = CDEUtils.getDataPath(mContext) + selectModeFileName;
             }

             CDELog.j(TAG, "selectModelFilePath:" + selectModelFilePath);
             selectModeFile = new File(selectModelFilePath);
             displayFileStatus(CDEUtils.getDataPath() + ggmlSampleFileName, selectModelFilePath);

             if (!selectModeFile.exists()) {
                 CDELog.j(TAG, "model file not exist:" + selectModeFile.getAbsolutePath());
             }
             File sampleFile = new File(CDEUtils.getDataPath() + ggmlSampleFileName);
             if (!selectModeFile.exists() || (!sampleFile.exists())) {
                 CDEUtils.showMsgBox(mActivity, "pls check whether model file:" + selectModeFileName + " exist in /sdcard/kantv/");
                 return;
             }

             String strPrompt = _txtUserInput.getText().toString();
             if (strPrompt.isEmpty()) {
                 //CDEUtils.showMsgBox(mActivity, "pls check your input");
                 //return;
                 strPrompt = strUserInput;
             }
             strPrompt = strPrompt.trim();
             strUserInput = strPrompt;
             CDELog.j(TAG, "User input: \n " + strUserInput);

             //sanity check end

             //reset default ggml model file name after sanity check
             ggmlModelFileName = selectModeFileName;
             CDELog.j(TAG, "model file:" + CDEUtils.getDataPath() + selectModeFileName);
             if (isASRModel) { //avoid crash
                 ggmljava.asr_reset(CDEUtils.getDataPath() + selectModeFileName, ggmljava.get_cpu_core_counts() / 2, CDEUtils.ASR_MODE_BECHMARK, backendIndex);
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
                         if (isLLMModel) {
                             strBenchmarkInfo = ggmljava.llm_inference(
                                     CDEUtils.getDataPath() + ggmlModelFileName,
                                     strUserInput,
                                     benchmarkIndex,
                                     nThreadCounts, backendIndex);
                         } else {
                             strBenchmarkInfo = ggmljava.ggml_bench(
                                     CDEUtils.getDataPath() + ggmlModelFileName,
                                     CDEUtils.getDataPath() + ggmlSampleFileName,
                                     benchmarkIndex,
                                     nThreadCounts, backendIndex, optypeIndex);
                         }
                     } else {
                         // avoid following issue
                         // dlopen failed: library "/sdcard/kantv/libInception_v3.so" needed or dlopened by
                         // "/data/app/~~70peMvcNIhRmzhm-PhmfRg==/com.cdeos.kantv-bUwy7gbMeCP0JFLe1J058g==/base.apk!/lib/arm64-v8a/libggml-jni.so"
                         // is not accessible for the namespace "clns-4"
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 CDEUtils.getDataPath(mContext) + ggmlModelFileName,
                                 CDEUtils.getDataPath() + ggmlSampleFileName,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, optypeIndex);
                     }
                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);

                     isBenchmarking.set(false);
                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String benchmarkTip = "Bench:" + CDEUtils.getBenchmarkDesc(benchmarkIndex) + " (model: " + selectModeFileName
                                     + " ,threads: " + nThreadCounts
                                     + " ,backend: " + CDEUtils.getBackendDesc(backendIndex)
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
                                 if (!strBenchmarkInfo.startsWith("asr_result")) {
                                     benchmarkTip += strBenchmarkInfo;
                                 }
                             }


                             if (strBenchmarkInfo.startsWith("asr_result")) { //when got asr result, playback the audio file
                                 playAudioFile();
                             }

                             CDELog.j(TAG, benchmarkTip);
                             _txtGGMLStatus.setText("");
                             _txtGGMLStatus.setText(benchmarkTip);

                             //update UI status
                             _btnBenchmark.setEnabled(true);

                             _txtASRInfo.scrollTo(0, 0);
                         }
                     });
                 }

                 stopUIBuffering();
                 //release();

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
                 //release();
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
                     /*
                     if (content.startsWith("llama-timings")) {
                         _txtGGMLStatus.setText("");
                         _txtGGMLStatus.append(content);
                     }
                     else */
                     {
                         nLogCounts++;
                         if (nLogCounts > 100) {
                             //_txtASRInfo.setText(""); //make QNN SDK happy on Xiaomi14
                             nLogCounts = 0;
                         }
                         _txtASRInfo.append(content);
                         int offset = _txtASRInfo.getLineCount() * _txtASRInfo.getLineHeight();
                         int screenHeight = CDEUtils.getScreenHeight();
                         int maxHeight = 500;//TODO: works fine on Xiaomi 14, adapt to other Android phone
                         //CDELog.j(TAG, "offset:" + offset);
                         //CDELog.j(TAG, "screenHeight:" + screenHeight);
                         if (offset > maxHeight)
                             _txtASRInfo.scrollTo(0, offset - maxHeight);
                     }
                 }
             }
         }
     }


     private void initKANTVMgr() {
         if (mKANTVMgr != null) {
             //04-13-2024, workaround for PoC:Add Qualcomm mobile SoC native backend for GGML,https://github.com/zhouwg/kantv/issues/121
             release();
             mKANTVMgr = null;
             //return;
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
