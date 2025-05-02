 /*
  * Copyright (c) Project KanTV. 2021-2023
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

 import static android.app.Activity.RESULT_OK;
 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_FINALIZE;
 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_STOP;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.app.ActivityManager;
 import android.app.AlertDialog;
 import android.app.ProgressDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.Intent;
 import android.content.res.Resources;
 import android.database.Cursor;
 import android.graphics.Bitmap;
 import android.graphics.BitmapFactory;
 import android.media.MediaPlayer;
 import android.net.Uri;
 import android.os.Build;
 import android.os.Debug;
 import android.provider.MediaStore;
 import android.view.Gravity;
 import android.view.View;
 import android.view.ViewGroup;
 import android.view.WindowManager;
 import android.widget.AdapterView;
 import android.widget.ArrayAdapter;
 import android.widget.Button;
 import android.widget.EditText;
 import android.widget.ImageView;
 import android.widget.LinearLayout;
 import android.widget.Spinner;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;
 import androidx.loader.content.CursorLoader;

 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.base.BaseMvpFragment;
 import com.kantvai.kantvplayer.mvp.impl.AIResearchPresenterImpl;
 import com.kantvai.kantvplayer.mvp.presenter.AIResearchPresenter;
 import com.kantvai.kantvplayer.mvp.view.AIResearchView;
 import com.kantvai.kantvplayer.utils.Settings;


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
 import kantvai.ai.ggmljava;
 import kantvai.media.player.KANTVAssetLoader;
 import kantvai.media.player.KANTVLLMModel;
 import kantvai.media.player.KANTVLLMModelMgr;
 import kantvai.media.player.KANTVLibraryLoader;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;
 import kantvai.media.player.KANTVEvent;
 import kantvai.media.player.KANTVEventListener;
 import kantvai.media.player.KANTVEventType;
 import kantvai.media.player.KANTVException;
 import kantvai.media.player.KANTVMgr;
 import kantvai.media.player.RecentMediaStorage;

 public class AIResearchFragment extends BaseMvpFragment<AIResearchPresenter> implements AIResearchView {
     @BindView(R.id.airesearchLayout)
     LinearLayout layout;

     private static final String TAG = AIResearchFragment.class.getName();
     TextView _txtASRInfo;
     TextView _txtGGMLInfo;
     EditText _txtUserInput;
     ImageView _ivInfo;
     LinearLayout _llInfoLayout;
     Button _btnBenchmark;
     Button _btnStop;
     Button _btnSelectImage;

     Spinner spinnerBackendType = null;
     Spinner spinnerAccelType = null;
     Spinner spinnerModelName = null;

     ArrayAdapter<String> adapterGGMLBackendType = null;
     ArrayAdapter<String> adapterGGMLAccelType = null;

     private static final int SELECT_IMAGE = 1;

     private int nThreadCounts = 1;
     private int benchmarkIndex = KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal();
     private int previousBenchmakrIndex = 0;
     private String strModeName = "tiny.en-q8_0";
     private String strBackend = "ggml";
     private String strAccel = "cdsp";

     private boolean bASROK = true;

     private int offset = 3;
     //TODO: the existing codes can't cover following special case:
     //      toggle backend and forth between QNN-NPU and cDSP and ggml in a standard Android APP or in
     //      a same running process, so here backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset
     //      supportive of such special case is easy but it will significantly increase the size of APK
     private int backendIndex = ggmljava.HEXAGON_BACKEND_GGML - offset;
     private int accelIndex = ggmljava.HEXAGON_BACKEND_CDSP;

     private String selectModeFileName = "";
     private Bitmap bitmapSelectedImage = null;
     private String pathSelectedImage = "";

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private long nLogCounts = 0;
     private boolean isLLMModel = false;
     private boolean isSDModel = false;
     private boolean isMNISTModel = false;
     private boolean isTTSModel = false;
     private boolean isASRModel = false;

     //05-25-2024, add for MiniCPM-V(A GPT-4V Level Multimodal LLM, https://github.com/OpenBMB/MiniCPM-V) or other GPT-4o style Multimodal LLM)
     private boolean isLLMVModel = false; //A GPT-4V style multimodal LLM
     private boolean isLLMOModel = false; //A GPT-4o style multimodal LLM

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     private String ggmlModelFileName = "ggml-tiny.en-q8_0.bin";
     private String ggmlSampleFileName = "jfk.wav";
     private String ggmlMNISTImageFile = "mnist-5.png";
     private String ggmlMNISTModelFile = "mnist-ggml-model-f32.gguf";

     private String strUserInput = "introduce the movie Once Upon a Time in America briefly, less then 100 words\n";
     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;
     private KANTVMgr mKANTVMgr = null;
     private AIResearchFragment.MyEventListener mEventListener = new AIResearchFragment.MyEventListener();
     private int selectModelIndex = 4; //index of selected LLM model, default index is 4 (gemma-3-4b)
     private int selectedUIIndex  = 0; //index of user's selected model in all models(ASR model  and LLM model)

     private KANTVLLMModelMgr LLMModelMgr = KANTVLLMModelMgr.getInstance();

     //=============================================================================================
     private String[]        arrayModelName;
     private void initLLMModels() {
         arrayModelName = LLMModelMgr.getArrayModelName();
     }
     //=============================================================================================


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

         _txtASRInfo = mActivity.findViewById(R.id.asrInfo);
         _txtGGMLInfo = mActivity.findViewById(R.id.ggmlInfo);
         //_txtGGMLStatus = mActivity.findViewById(R.id.ggmlStatus);
         _btnBenchmark = mActivity.findViewById(R.id.btnBenchmark);
         _btnStop = mActivity.findViewById(R.id.btnStop);
         _btnSelectImage = mActivity.findViewById(R.id.btnSelectImage);
         _txtUserInput = mActivity.findViewById(R.id.txtPrompt);
         _llInfoLayout = mActivity.findViewById(R.id.llInfoLayout);
         _txtASRInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
         //will be removed in the future
         //displayFileStatus(KANTVUtils.getDataPath() + ggmlSampleFileName, KANTVUtils.getDataPath() + ggmlModelFileName);

         initLLMModels();

         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         KANTVLog.j(TAG, "set ggml's whisper.cpp info");
         setTextGGMLInfo(LLMModelMgr.getName(selectModelIndex));

         Spinner spinnerBenchType = mActivity.findViewById(R.id.spinnerBenchType);
         String[] arrayBenchType = getResources().getStringArray(R.array.benchType);
         ArrayAdapter<String> adapterBenchType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBenchType);
         spinnerBenchType.setAdapter(adapterBenchType);
         spinnerBenchType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "bench type:" + arrayBenchType[position]);
                 benchmarkIndex = Integer.valueOf(position);
                 KANTVLog.j(TAG, "benchmark index:" + benchmarkIndex);

                 if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                     spinnerModelName.setSelection(0); //hardcode to ggml-tiny.en-q8_0.bin for purpose of validate various models more easily on Android phone
                 }
                 if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                     spinnerModelName.setSelection(5); //hardcode to gemma-3-4b-it-Q8_0.gguf for purpose of validate LLM multimodal more easily on Android phone
                 }

                 if ((previousBenchmakrIndex < KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal()) && (benchmarkIndex < KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())) {
                     previousBenchmakrIndex = benchmarkIndex;
                     return;
                 }

                 if ((previousBenchmakrIndex >= KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal()) && (benchmarkIndex >= KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())) {
                     previousBenchmakrIndex = benchmarkIndex;
                     return;
                 }

                 spinnerBackendType.setAdapter(adapterGGMLBackendType);
                 adapterGGMLBackendType.notifyDataSetChanged();

                 previousBenchmakrIndex = benchmarkIndex;
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerBenchType.setSelection(KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal());

         Spinner spinnerThreadsCounts = mActivity.findViewById(R.id.spinnerThreadCounts);
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

         spinnerBackendType = mActivity.findViewById(R.id.spinnerBackend);
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

         /* not used currently, keep them for further usage
         spinnerAccelType = mActivity.findViewById(R.id.spinnerAccel);
         String[] arrayAccel = getResources().getStringArray(R.array.hwacceltype);
         adapterGGMLAccelType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayAccel);
         spinnerAccelType.setAdapter(adapterGGMLAccelType);
         spinnerAccelType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "hwaccel type:" + arrayAccel[position]);
                 strAccel = arrayAccel[position];
                 accelIndex = Integer.valueOf(position);
                 KANTVLog.j(TAG, "strAccel:" + strAccel);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerAccelType.setSelection(ggmljava.HWACCEL_CDSP);
         */

         spinnerModelName = mActivity.findViewById(R.id.spinnerModelName);
         //replace with code-generated array
         //String[] arrayModelName = getResources().getStringArray(R.array.newModelName);
         ArrayAdapter<String> adapterModel = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayModelName);
         spinnerModelName.setAdapter(adapterModel);
         spinnerModelName.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "position: " + position + ", model:" + arrayModelName[position]);
                 strModeName = arrayModelName[position];
                 selectedUIIndex = position;
                 //attention here
                 selectModelIndex = selectedUIIndex - 1;
                 KANTVLog.j(TAG, "strModeName:" + strModeName);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerModelName.setSelection(0); // ggml-tiny.en-q8_0.bin, 42M

         _btnSelectImage.setOnClickListener(v -> {
             Intent intent = new Intent(Intent.ACTION_PICK);
             intent.setType("image/*");
             startActivityForResult(intent, SELECT_IMAGE);
         });

         _btnStop.setOnClickListener(v -> {
             KANTVLog.g(TAG, "here");
             if (ggmljava.llm_is_running()) {
                 KANTVLog.g(TAG, "here");
                 ggmljava.llm_stop_inference();
             }
             resetUIAndStatus(true, false);
         });

         _btnBenchmark.setOnClickListener(v -> {
             KANTVLog.g(TAG, "selectModeIndex:" + selectModelIndex);
             KANTVLog.j(TAG, "strModeName:" + arrayModelName[selectedUIIndex]);
             KANTVLog.j(TAG, "exec ggml benchmark: type: " + KANTVUtils.getBenchmarkDesc(benchmarkIndex)
                     + ", threads:" + nThreadCounts + ", model:" + strModeName + ", backend:" + strBackend);
             String selectModelFilePath = "";

             resetUIAndStatus(true, true);

             //sanity check begin
             {
                 isASRModel = KANTVUtils.isASRModel(strModeName);
                 if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                     //attention here
                     if (selectModelIndex < 0) {
                         selectModelIndex = 0;
                         //selectModeFileName = "ggml-" + strModeName + ".bin";
                         selectModeFileName = strModeName;
                     } else {
                         selectModeFileName = LLMModelMgr.getName(selectModelIndex);
                     }

                     isLLMModel = true;
                     setTextGGMLInfo(selectModeFileName);
                 } else {
                     //https://huggingface.co/ggerganov/whisper.cpp
                     //selectModeFileName = "ggml-" + strModeName + ".bin";
                     selectModeFileName = strModeName;
                     setTextGGMLInfo(selectModeFileName);
                 }
                 KANTVLog.g(TAG, "selectModeFileName:" + selectModeFileName);

                 if (isASRModel && (benchmarkIndex != KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())) {
                     KANTVLog.j(TAG, "1-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVUtils.getBenchmarkDesc(benchmarkIndex));
                     KANTVUtils.showMsgBox(mActivity, "1-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVUtils.getBenchmarkDesc(benchmarkIndex));
                     return;
                 }
                 if ((!isASRModel) && (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())) {
                     KANTVLog.j(TAG, "2-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVUtils.getBenchmarkDesc(benchmarkIndex));
                     KANTVUtils.showMsgBox(mActivity, "2-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVUtils.getBenchmarkDesc(benchmarkIndex));
                     return;
                 }

                 //FIXME: refine logic here
                 if ((pathSelectedImage != null) && (!pathSelectedImage.isEmpty())) {
                     if (selectModeFileName.contains("gemma-3")) {
                         isLLMVModel = true;
                         File mmprModelFile = new File(KANTVUtils.getSDCardDataPath() + LLMModelMgr.getMMProjName(selectModelIndex));
                         if (!mmprModelFile.exists()) {
                             KANTVUtils.showMsgBox(mActivity, "LLM mmproj model file:" +
                                     LLMModelMgr.getMMProjName(selectModelIndex) +
                                     " not exist, pls download from: "
                                     + LLMModelMgr.getMMProjUrl(selectModelIndex) + " in LLM Setting");
                             return;
                         }
                     }
                 }

                 if ((!isMNISTModel) && (!isLLMVModel) && (!isLLMOModel)) {
                     if (_ivInfo != null) {
                         _ivInfo.setVisibility(View.INVISIBLE);
                         _llInfoLayout.removeView(_ivInfo);
                         _ivInfo = null;
                     }
                 }

                 if (isLLMVModel || isLLMOModel) {
                     if ((bitmapSelectedImage == null) || (pathSelectedImage.isEmpty())) {
                         KANTVLog.j(TAG, "image is empty");
                         KANTVUtils.showMsgBox(mActivity, "please select a image for LLM multimodal inference");
                         return;
                     }
                 }

                 if (isASRModel) //ASR inference
                     //ASR inference: built-in ASR model which located in path /sdcard/kantv/
                     selectModelFilePath = KANTVUtils.getDataPath() + selectModeFileName;
                 else {
                     //LLM inference: LLM model is located on /sdcard/ and manually uploaded by user currently
                     selectModelFilePath = KANTVUtils.getSDCardDataPath() + selectModeFileName;
                 }

                 KANTVLog.g(TAG, "selectModelFilePath:" + selectModelFilePath);

                 File selectModeFile = new File(selectModelFilePath);
                 if (!selectModeFile.exists()) {
                     KANTVLog.j(TAG, "model file not exist:" + selectModeFile.getAbsolutePath());
                 }
                 File sampleFile = new File(KANTVUtils.getDataPath() + ggmlSampleFileName);
                 if (!sampleFile.exists()) {
                     KANTVLog.j(TAG, "sample file not exist:" + sampleFile.getAbsolutePath());
                 }

                 if (isASRModel) {
                     if (!selectModeFile.exists() || (!sampleFile.exists())) {
                         KANTVUtils.showMsgBox(mActivity, "pls check whether model file:" +
                                 selectModeFile.getAbsolutePath() + " and sample file:" + sampleFile.getAbsolutePath() + " exist");
                         return;
                     }
                 } else {
                     if (!selectModeFile.exists()) {
                         KANTVUtils.showMsgBox(mActivity, "LLM model file:" +
                                 selectModeFile.getAbsolutePath() + " not exist, pls download from: "
                                 + LLMModelMgr.getModelUrl(selectModelIndex) + " in LLM Setting");
                         return;
                     }
                 }

                 String strPrompt = _txtUserInput.getText().toString();
                 if (strPrompt.isEmpty()) {
                     //KANTVUtils.showMsgBox(mActivity, "pls check your input");
                     //return;
                     strPrompt = strUserInput;
                 }
                 strPrompt = strPrompt.trim();
                 strUserInput = strPrompt;
                 KANTVLog.j(TAG, "user input: \n " + strUserInput);
             }
             //sanity check end

             //reset default ggml model file name after sanity check
             ggmlModelFileName = selectModelFilePath;
             KANTVLog.j(TAG, "model file:" + ggmlModelFileName);
             if (isASRModel) { //avoid crash
                 int result = ggmljava.asr_reset(selectModelFilePath, ggmljava.get_cpu_core_counts() / 2, KANTVUtils.ASR_MODE_BECHMARK, backendIndex);
                 if (0 != result) {
                     KANTVLog.j(TAG, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                     KANTVUtils.showMsgBox(mActivity, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                     return;
                 }
             }

             nLogCounts = 0;

             //will be removed in the future
             //startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating) + "(" + KANTVUtils.getBenchmarkDesc(benchmarkIndex) + ")");

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

                 initKANTVMgr();

                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();
                     ggmljava.ggml_set_benchmark_status(0);

                     if (isLLMModel) {
                         if (isLLMVModel) {
                             //LLM multimodal inference
                             KANTVLog.g(TAG, "LLMV model, image path:" + pathSelectedImage);
                             strBenchmarkInfo = ggmljava.llava_inference(
                                     KANTVUtils.getSDCardDataPath() + LLMModelMgr.getName(selectModelIndex),
                                     KANTVUtils.getSDCardDataPath() + LLMModelMgr.getMMProjName(selectModelIndex),
                                     pathSelectedImage,
                                     "What is in the image?",
                                     2,
                                     nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                         } else {
                             //general LLM inference
                             strBenchmarkInfo = ggmljava.llm_inference(
                                     KANTVUtils.getSDCardDataPath() + LLMModelMgr.getName(selectModelIndex),
                                     strUserInput,
                                     1,
                                     nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                         }
                     } else if (isMNISTModel) {
                         //MNIST inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 KANTVUtils.getDataPath() + ggmlMNISTImageFile,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else if (isSDModel) {
                         //Text2Image inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 "a lovely cat"/*strUserInput*/,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else if (isTTSModel) {
                         //TTS inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 "this is an audio generated by bark.cpp"/*strUserInput*/,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else {
                         //ASR inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 KANTVUtils.getDataPath() + ggmlSampleFileName,
                                 benchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     }

                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);
                     isBenchmarking.set(false);

                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             displayInferenceResult(null);
                             //update UI status
                             resetUIAndStatus(false, true);
                         }
                     });
                 }

                 //will be removed in the future
                 //stopUIBuffering();
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
                                 KANTVLog.j(TAG, "stop GGML benchmark");

                                 //terminate background ggml thread when user cancel time-consuming bench task in UI layer
                                 //
                                 //background computing task(it's a blocked task) in native layer might be not finished
                                 //
                                 //for keep (FSM) status sync accurately between UI and native source code, there are might be much efforts to do it
                                 //
                                 //this is the gap between open source project and commercial project
                                 ggmljava.ggml_set_benchmark_status(1);

                                 mProgressDialog.dismiss();
                                 mProgressDialog = null;
                                 isBenchmarking.set(false);
                                 _btnBenchmark.setEnabled(true);
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
                     //Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_stop), Toast.LENGTH_SHORT).show();
                 }
                 String benchmarkTip = "GGML benchmark finished ";
                 KANTVLog.j(TAG, benchmarkTip);
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

     @Override
     public void onActivityResult(int requestCode, int resultCode, Intent data) {
         super.onActivityResult(requestCode, resultCode, data);
         if ((requestCode == SELECT_IMAGE) && (null != data)) {
             Uri selectedImageUri = data.getData();
             try {
                 String[] proj = {MediaStore.Images.Media.DATA};
                 CursorLoader loader = new CursorLoader(mContext, selectedImageUri, proj, null, null, null);
                 Cursor cursor = loader.loadInBackground();
                 int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                 cursor.moveToFirst();
                 String realImagePath = cursor.getString(column_index);
                 cursor.close();
                 KANTVLog.g(TAG, "realImagePath " + realImagePath);
                 selectedImageUri = Uri.fromFile(new File(realImagePath));

                 Bitmap bitmap = decodeUri(selectedImageUri, false);
                 Bitmap rgba = bitmap.copy(Bitmap.Config.ARGB_8888, true);
                 // resize to 227x227
                 // bitmapSelectedImage = Bitmap.createScaledBitmap(rgba, 227, 227, false);
                 // scale to 227x227 in native layer
                 bitmapSelectedImage = Bitmap.createBitmap(rgba);
                 rgba.recycle();
                 {
                     String imgPath = selectedImageUri.getPath();
                     KANTVLog.g(TAG, "image path:" + imgPath);
                     //xiaomi14: image path:/raw//storage/emulated/0/Pictures/mnist-7.png, skip /raw/
                     if (imgPath.startsWith("/raw/"))
                         imgPath = imgPath.substring(6);
                     pathSelectedImage = imgPath;
                     KANTVLog.g(TAG, "image path:" + imgPath);
                     displayImage(imgPath);
                 }
             } catch (FileNotFoundException e) {
                 KANTVLog.g(TAG, "FileNotFoundException: " + e.toString());
                 KANTVUtils.showMsgBox(mActivity, "FileNotFoundException: " + e.toString());
             }
         } else {
             KANTVLog.g(TAG, "it shouldn't happen, pls check why?");
             KANTVUtils.showMsgBox(mActivity, "it shouldn't happen, pls check why");
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
                 _txtASRInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP) || (arg1 == KANTV_INFO_ASR_FINALIZE)) {
                     return;
                 }

                 if (content.startsWith("reset")) {
                     _txtASRInfo.setText("");
                     return;
                 }

                 if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                     if (content.contains("not initialized")) {
                         bASROK = false;
                     }
                 }

                 //make UI happy when disable GGML_USE_HEXAGON manually
                 if (content.startsWith("ggml-hexagon")) {
                     _txtASRInfo.setText(content);
                     return;
                 }

                 if (content.startsWith("unknown")) {

                 } else {
                     if (content.startsWith("llama-timings")) {
                         KANTVLog.j(TAG, "LLM timings");
                         displayInferenceResult(content);
                     } else {
                         nLogCounts++;
                         if (nLogCounts > 100) {
                             //_txtASRInfo.setText("");
                             nLogCounts = 0;
                         }
                         if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                             if (!ggmljava.llm_is_running()) {
                                 return;
                             }
                         }
                         _txtASRInfo.append(content);
                         int offset = _txtASRInfo.getLineCount() * _txtASRInfo.getLineHeight();
                         int screenHeight = KANTVUtils.getScreenHeight();
                         int maxHeight = 500;
                         KANTVLog.j(TAG, "offset:" + offset);
                         KANTVLog.j(TAG, "screenHeight:" + screenHeight);
                         if (offset > maxHeight)
                             _txtASRInfo.scrollTo(0, offset - maxHeight);
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

         resetUIAndStatus(true, false);
     }

     /* will be removed in the future
     private void displayFileStatus(String sampleFilePath, String modelFilePath) {
         _txtGGMLStatus.setText("");

         File sampleFile = new File(sampleFilePath);
         if (sampleFile.exists()) {
             _txtGGMLStatus.append("sample file exist:" + sampleFile.getAbsolutePath());
         } else {
             KANTVLog.j(TAG, "sample file not exist:" + sampleFile.getAbsolutePath());
             _txtGGMLStatus.append("\nsample file not exist: " + sampleFile.getAbsolutePath());
         }

         _txtGGMLStatus.append("\n");

         File modelFile = new File(modelFilePath);
         if (modelFile.exists()) {
             _txtGGMLStatus.append("model   file exist:" + modelFile.getAbsolutePath());
         } else {
             KANTVLog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
             _txtGGMLStatus.append("model   file not exist: " + modelFile.getAbsolutePath());
         }
     }
     */

     private boolean isGGMLInfernce() {
         if (benchmarkIndex < KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())
             return true;
         else
             return false;
     }

     private boolean isNCNNInference() {
         if (benchmarkIndex >= KANTVUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())
             return true;
         else
             return false;
     }

     private void displayImage(String imgPath) {
         if (_ivInfo != null) {
             _ivInfo.setVisibility(View.INVISIBLE);
             _llInfoLayout.removeView(_ivInfo);
             _ivInfo = null;
         }

         Uri uri = Uri.fromFile(new File(imgPath));
         BitmapFactory.Options opts = new BitmapFactory.Options();
         opts.inJustDecodeBounds = true;
         BitmapFactory.decodeFile(imgPath, opts);
         int imgWidth = opts.outWidth;
         int imgHeight = opts.outHeight;
         KANTVLog.g(TAG, "img width=" + imgWidth + ", img height=" + imgHeight);

         ViewGroup.LayoutParams vlp = new LinearLayout.LayoutParams(
                 ViewGroup.LayoutParams.WRAP_CONTENT,
                 ViewGroup.LayoutParams.WRAP_CONTENT
         );
         if ((0 == imgWidth) || (0 == imgHeight)) {
             KANTVLog.g(TAG, "invalid image width and height");
             return;
         }
         _ivInfo = new ImageView(mActivity);
         _ivInfo.setLayoutParams(vlp);
         _llInfoLayout.addView(_ivInfo);
         _llInfoLayout.setGravity(Gravity.CENTER);
         _ivInfo.setImageURI(uri);
         _ivInfo.setVisibility(View.VISIBLE);
         _ivInfo.setScaleType(ImageView.ScaleType.FIT_CENTER);
         _ivInfo.setAdjustViewBounds(true);
     }


     private Bitmap decodeUri(Uri uriSelectedImage, boolean scaled) throws FileNotFoundException {
         // Decode image size
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inJustDecodeBounds = true;
         BitmapFactory.decodeStream(mActivity.getContentResolver().openInputStream(uriSelectedImage), null, options);

         // The new size we want to scale to
         final int REQUIRED_SIZE = 400;

         // Find the correct scale value. It should be the power of 2.
         int width_tmp = options.outWidth;
         int height_tmp = options.outHeight;
         int scale = 1;
         while (true) {
             if (width_tmp / 2 < REQUIRED_SIZE
                     || height_tmp / 2 < REQUIRED_SIZE) {
                 break;
             }
             width_tmp /= 2;
             height_tmp /= 2;
             scale *= 2;
         }

         // Decode with inSampleSize
         options = new BitmapFactory.Options();
         if (scaled)
             options.inSampleSize = scale;
         else
             options.inSampleSize = 1;
         return BitmapFactory.decodeStream(mActivity.getContentResolver().openInputStream(uriSelectedImage), null, options);
     }


     private void playAudioFile() {
         try {
             MediaPlayer mediaPlayer = new MediaPlayer();
             KANTVLog.j(TAG, "audio file:" + KANTVUtils.getDataPath() + ggmlSampleFileName);
             mediaPlayer.setDataSource(KANTVUtils.getDataPath() + ggmlSampleFileName);
             mediaPlayer.prepare();
             mediaPlayer.start();
         } catch (IOException ex) {
             KANTVLog.j(TAG, "failed to play audio file:" + ex.toString());
         } catch (Exception ex) {
             KANTVLog.j(TAG, "failed to play audio file:" + ex.toString());
         }
     }

     private void initUIAndStatus() {
         isBenchmarking.set(true);
         //Toast.makeText(mContext, mContext.getString(R.string.ggml_benchmark_start), Toast.LENGTH_LONG).show();

         _txtASRInfo.setText("");
         _btnBenchmark.setEnabled(false);
         _btnBenchmark.setBackgroundColor(0xffa9a9a9);

         WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
         attributes.screenBrightness = 1.0f;
         mActivity.getWindow().setAttributes(attributes);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
     }

     private void resetUIAndStatus( boolean removeInferenceResult, boolean dispImage) {
         isBenchmarking.set(false);
         _btnBenchmark.setEnabled(true);
         _btnBenchmark.setBackgroundColor(0xC3009688);

         //for LLM multimodal
         if (!dispImage) {
             if (_ivInfo != null) {
                 _ivInfo.setVisibility(View.INVISIBLE);
                 _llInfoLayout.removeView(_ivInfo);
                 _ivInfo = null;
             }
         } else {
             if (isMNISTModel) {
                 String imgPath = KANTVUtils.getDataPath() + ggmlMNISTImageFile;
                 displayImage(imgPath);
                 bitmapSelectedImage = null;
                 pathSelectedImage = null;
             }

             if (isLLMVModel) {
                 if (pathSelectedImage != null && !pathSelectedImage.isEmpty()) {
                     displayImage(pathSelectedImage);
                 }
                 bitmapSelectedImage = null;
                 pathSelectedImage = null;
             }
         }

         if (removeInferenceResult)
             _txtASRInfo.setText("");

         isLLMModel = false;
         isSDModel = false;
         isMNISTModel = false;
         isTTSModel = false;
         isASRModel = false;
         isLLMVModel = false;
         isLLMOModel = false;

         selectModeFileName = "";
     }

     private void displayInferenceResult(String content) {
         _txtASRInfo.scrollTo(0, 0);
         if (strBenchmarkInfo.startsWith("unknown")) {
             return;
         }
         String backendDesc = KANTVUtils.getGGMLBackendDesc(backendIndex);
         String benchmarkTip = "\nBench:" + KANTVUtils.getBenchmarkDesc(benchmarkIndex) + " (model: " + selectModeFileName
                 + " ,threads: " + nThreadCounts
                 + " ,backend: " + backendDesc
                 + " ) cost " + duration + " milliseconds";
         //04-07-2024(April,7,2024), add timestamp
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         benchmarkTip += ", on " + timestamp;

         benchmarkTip += "\n";

         if (!strBenchmarkInfo.startsWith("unknown")) {
             if (!strBenchmarkInfo.startsWith("asr_result")) {
                 benchmarkTip += strBenchmarkInfo;
             }
         }

         if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
             if (!bASROK) {
                 return;
             }
             if (strBenchmarkInfo.startsWith("asr_result")) { //when got asr result, playback the audio file
                 playAudioFile();
             }
         }

         KANTVLog.j(TAG, benchmarkTip);

         String dispInfo;
         if (benchmarkIndex == KANTVUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())
             dispInfo = KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_ASR);
         else
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

     private void setTextGGMLInfo(String LLMModelFileName) {
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(KANTVUtils.getDeviceInfo(mActivity, KANTVUtils.INFERENCE_ASR));
         _txtGGMLInfo.append("\n" + "LLM model:" + LLMModelFileName);
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         _txtGGMLInfo.append("\n");
         _txtGGMLInfo.append("running timestamp:" + timestamp);
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
