 /*
  * Copyright (c) 2024- KanTV Authors
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
 import kantvai.ai.KANTVAIModelMgr;
 import kantvai.ai.KANTVAIUtils;
 import kantvai.ai.ggmljava;
 import kantvai.media.player.KANTVAssetLoader;
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
     TextView txtASRInfo;
     TextView txtGGMLInfo;
     EditText txtUserInput;
     ImageView ivInfo;
     LinearLayout llInfoLayout;
     Button btnBenchmark;
     Button btnStop;
     Button btnSelectImage;

     Spinner spinnerBackendType = null;
     Spinner spinnerModelName = null;

     ArrayAdapter<String> adapterGGMLBackendType = null;

     private static final int SELECT_IMAGE = 1;

     private int nThreadCounts = 1;
     private int nBenchmarkIndex = KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal();
     private int nPreviousBenchmakrIndex = 0;
     private String strModeName = "tiny.en-q8_0";
     private String strBackend = "ggml";

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

     private KANTVAIModelMgr AIModelMgr = KANTVAIModelMgr.getInstance();
     private int selectModelIndex = KANTVAIModelMgr.getInstance().getDefaultModelIndex(); //index of the default LLM model
     private int selectedUIIndex  = 0; //index of user's selected model in all models(ASR model, StableDiffusion model and LLM model)

     //=============================================================================================
     private String[]        arrayModelName;
     private void initLLMModels() {
         arrayModelName = AIModelMgr.getAllAIModelNickName();
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

         txtASRInfo = mActivity.findViewById(R.id.asrInfo);
         txtGGMLInfo = mActivity.findViewById(R.id.ggmlInfo);
         btnBenchmark = mActivity.findViewById(R.id.btnBenchmark);
         btnStop = mActivity.findViewById(R.id.btnStop);
         btnSelectImage = mActivity.findViewById(R.id.btnSelectImage);
         txtUserInput = mActivity.findViewById(R.id.txtPrompt);
         llInfoLayout = mActivity.findViewById(R.id.llInfoLayout);
         txtASRInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);

         initLLMModels();

         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         KANTVLog.j(TAG, "set ggml's whisper.cpp info");
         setTextGGMLInfo(AIModelMgr.getKANTVAIModelFromName("Gemma3-4B").getName());

         Spinner spinnerBenchType = mActivity.findViewById(R.id.spinnerBenchType);
         String[] arrayBenchType = getResources().getStringArray(R.array.benchType);
         ArrayAdapter<String> adapterBenchType = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBenchType);
         spinnerBenchType.setAdapter(adapterBenchType);
         spinnerBenchType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                 KANTVLog.j(TAG, "bench type:" + arrayBenchType[position]);
                 nBenchmarkIndex = Integer.valueOf(position);
                 KANTVLog.j(TAG, "benchmark index:" + nBenchmarkIndex);

                 if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                     spinnerModelName.setSelection(0); //hardcode to ggml-tiny.en-q8_0.bin for purpose of validate various models more easily on Android phone
                 }
                 if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                     //hardcode to gemma-3-4b-it-Q8_0.gguf for purpose of validate LLM multimodal more easily on Android phone
                     spinnerModelName.setSelection(AIModelMgr.getDefaultModelIndex() + AIModelMgr.getNonLLMModelCounts());
                     txtUserInput.setText("introduce the movie Once Upon a Time in America briefly, less then 100 words\n");
                 }

                 if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_TEXT2IMAGE.ordinal()) {
                     spinnerModelName.setSelection(1); //hardcode to SD model name for purpose of validate LLM multimodal more easily on Android phone
                     txtUserInput.setText("a lovely cat");
                 }

                 if ((nPreviousBenchmakrIndex < KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal()) && (nBenchmarkIndex < KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())) {
                     nPreviousBenchmakrIndex = nBenchmarkIndex;
                     return;
                 }

                 if ((nPreviousBenchmakrIndex >= KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal()) && (nBenchmarkIndex >= KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())) {
                     nPreviousBenchmakrIndex = nBenchmarkIndex;
                     return;
                 }

                 spinnerBackendType.setAdapter(adapterGGMLBackendType);
                 adapterGGMLBackendType.notifyDataSetChanged();

                 nPreviousBenchmakrIndex = nBenchmarkIndex;
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerBenchType.setSelection(KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal());

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
                 selectModelIndex = selectedUIIndex - AIModelMgr.getNonLLMModelCounts();
                 KANTVLog.j(TAG, "strModeName:" + strModeName);
             }

             @Override
             public void onNothingSelected(AdapterView<?> parent) {

             }
         });
         spinnerModelName.setSelection(0); // ggml-tiny.en-q8_0.bin, 42M

         btnSelectImage.setOnClickListener(v -> {
             resetUIAndStatus(null, true, false);
             Intent intent = new Intent(Intent.ACTION_PICK);
             intent.setType("image/*");
             startActivityForResult(intent, SELECT_IMAGE);
         });

         btnStop.setOnClickListener(v -> {
             KANTVLog.g(TAG, "here");
             if (ggmljava.inference_is_running()) {
                 KANTVLog.g(TAG, "here");
                 ggmljava.inference_stop_inference();
             }
             resetUIAndStatus(null,true, false);
         });

         btnBenchmark.setOnClickListener(v -> {
             KANTVLog.g(TAG, "selectUIIndex:" + selectedUIIndex);
             KANTVLog.g(TAG, "selectModeIndex:" + selectModelIndex);
             KANTVLog.g(TAG, "strModeName:" + arrayModelName[selectedUIIndex]);
             KANTVLog.j(TAG, "exec ggml benchmark: type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex)
                     + ", threads:" + nThreadCounts + ", model:" + strModeName + ", backend:" + strBackend);
             String selectModelFilePath = "";

             resetUIAndStatus(null,true, true);

             //sanity check begin
             {
                 isASRModel = KANTVAIUtils.isASRModel(strModeName);
                 if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                     //attention here
                     if (selectModelIndex < 0) {
                         selectModelIndex = 0;
                         selectModeFileName = "ggml-" + strModeName + ".bin";
                     } else {
                         selectModeFileName = AIModelMgr.getKANTVAIModelFromName(strModeName).getName();
                     }
                     isLLMModel = true;
                 } else if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_TEXT2IMAGE.ordinal()) {
                     isSDModel = true;
                     selectModeFileName = AIModelMgr.getKANTVAIModelFromName(strModeName).getName();
                 } else {
                     //https://huggingface.co/ggerganov/whisper.cpp
                     selectModeFileName = AIModelMgr.getKANTVAIModelFromName(strModeName).getName();
                 }
                 setTextGGMLInfo(selectModeFileName);
                 KANTVLog.g(TAG, "selectModeFileName:" + selectModeFileName);

                 if ((KANTVAIUtils.bench_type.GGML_BENCHMARK_MEMCPY.ordinal() == nBenchmarkIndex)
                         || (KANTVAIUtils.bench_type.GGML_BENCHMARK_MULMAT.ordinal() == nBenchmarkIndex)) {
                     //reset to ASR model name
                     setTextGGMLInfo(AIModelMgr.getKANTVAIModelFromIndex(0).getName());
                 } else {
                     if (isASRModel && (nBenchmarkIndex != KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())) {
                         KANTVLog.j(TAG, "1-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex));
                         KANTVUtils.showMsgBox(mActivity, "1-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex));
                         return;
                     }
                     if ((!isASRModel) && (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())) {
                         KANTVLog.j(TAG, "2-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex));
                         KANTVUtils.showMsgBox(mActivity, "2-mismatch between model file:" + selectModeFileName + " and bench type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex));
                         return;
                     }
                 }

                 //FIXME: refine logic here
                 if ((pathSelectedImage != null) && (!pathSelectedImage.isEmpty())) {
                     if (KANTVAIUtils.isLLMVModel(selectModeFileName)) {
                         isLLMVModel = true;
                         txtUserInput.setText("What is in the image?");
                         File mmprModelFile = new File(KANTVUtils.getSDCardDataPath() + AIModelMgr.getMMProjmodelName(selectModelIndex));
                         if (!mmprModelFile.exists()) {
                             KANTVUtils.showMsgBox(mActivity, "LLM mmproj model file:" +
                                     AIModelMgr.getMMProjmodelName(selectModelIndex) +
                                     " not exist, pls download from: "
                                     + AIModelMgr.getMMProjmodelUrl(selectModelIndex) + " in LLM Setting");
                             return;
                         }
                     }
                 }

                 if ((!isMNISTModel) && (!isLLMVModel) && (!isLLMOModel)) {
                     if (ivInfo != null) {
                         ivInfo.setVisibility(View.INVISIBLE);
                         llInfoLayout.removeView(ivInfo);
                         ivInfo = null;
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
                     //ASR inference: built-in ASR model which located in path /sdcard/kantv/ or internal data path
                     selectModelFilePath = KANTVUtils.getDataPath(mContext) + selectModeFileName;
                 else {
                     //LLM inference: LLM model is located on /sdcard/ and can be downloaded in the APK,
                     //               or manually uploaded to the phone by user
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
                 } else if (isSDModel) {
                     if (!selectModeFile.exists()) {
                         KANTVUtils.showMsgBox(mActivity, "StableDiffusion model file:" +
                                 selectModeFile.getAbsolutePath() + " not exist, pls download from: "
                                 + AIModelMgr.getKANTVAIModelFromName("sd-v1-4.ckpt").getUrl());
                         return;
                     }
                 } else {
                     if (!selectModeFile.exists()) {
                         KANTVUtils.showMsgBox(mActivity, "LLM model file:" +
                                 selectModeFile.getAbsolutePath() + " not exist, pls download from: "
                                 + AIModelMgr.getModelUrl(selectModelIndex) + " in LLM Setting");
                         return;
                     }
                 }

                 String strPrompt = txtUserInput.getText().toString();
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
                 int result = ggmljava.asr_reset(selectModelFilePath, ggmljava.get_cpu_core_counts() / 2, KANTVAIUtils.ASR_MODE_BECHMARK, backendIndex);
                 if (0 != result) {
                     KANTVLog.j(TAG, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                     KANTVUtils.showMsgBox(mActivity, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                     return;
                 }
             }

             nLogCounts = 0;

             //will be removed in the future
             //startUIBuffering(mContext.getString(R.string.ggml_benchmark_updating) + "(" + KANTVUtils.getBenchmarkDesc(nBenchmarkIndex) + ")");

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
                                     KANTVUtils.getSDCardDataPath() + AIModelMgr.getModelName(selectModelIndex),
                                     KANTVUtils.getSDCardDataPath() + AIModelMgr.getMMProjmodelName(selectModelIndex),
                                     pathSelectedImage,
                                     strUserInput,
                                     2,
                                     nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                         } else {
                             //general LLM inference
                             strBenchmarkInfo = ggmljava.llm_inference(
                                     KANTVUtils.getSDCardDataPath() + AIModelMgr.getModelName(selectModelIndex),
                                     strUserInput,
                                     1,
                                     nThreadCounts, backendIndex, ggmljava.HWACCEL_CDSP);
                         }
                     } else if (isMNISTModel) {
                         //MNIST inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 KANTVUtils.getDataPath() + ggmlMNISTImageFile,
                                 nBenchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else if (isSDModel) {
                         //Text2Image inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 strUserInput,
                                 nBenchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else if (isTTSModel) {
                         //TTS inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 "this is an audio generated by bark.cpp"/*strUserInput*/,
                                 nBenchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     } else {
                         //ASR inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                 ggmlModelFileName,
                                 KANTVUtils.getDataPath() + ggmlSampleFileName,
                                 nBenchmarkIndex,
                                 nThreadCounts, backendIndex, accelIndex);
                     }

                     endTime = System.currentTimeMillis();
                     duration = (endTime - beginTime);
                     isBenchmarking.set(false);

                     mActivity.runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             displayInferenceResult(null, true);
                             //update UI status
                             resetUIAndStatus(strBenchmarkInfo,false, true);
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
                                 btnBenchmark.setEnabled(true);
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
             } catch (Exception exception) {
                 KANTVLog.g(TAG, "error occurred: " + exception.toString());
                 KANTVUtils.showMsgBox(mActivity, "error occurred: " + exception.toString());
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
                 txtASRInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP) || (arg1 == KANTV_INFO_ASR_FINALIZE)) {
                     return;
                 }

                 if (content.startsWith("reset")) {
                     txtASRInfo.setText("");
                     return;
                 }

                 if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                     if (content.contains("not initialized")) {
                         bASROK = false;
                     }
                 }

                 //make UI happy when disable GGML_USE_HEXAGON manually
                 if (content.startsWith("ggml-hexagon")) {
                     txtASRInfo.setText(content);
                     return;
                 }

                 if (content.startsWith("unknown")) {

                 } else {
                     if (content.startsWith("llama-timings")) {
                         KANTVLog.j(TAG, "LLM timings");
                         displayInferenceResult(content, true);
                     } else if (content.startsWith("text2image-timings")) {
                         KANTVLog.g(TAG, "text2image timings");
                         try {
                             displayImage("/sdcard/output.png");
                         } catch (Exception ex) {
                             KANTVLog.g(TAG, "error: " + ex.toString());
                             KANTVUtils.showMsgBox(mActivity, "error: " + ex.toString());
                         }
                     } else {
                         nLogCounts++;
                         if (nLogCounts > 100) {
                             //txtASRInfo.setText("");
                             nLogCounts = 0;
                         }
                         if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                             if (!ggmljava.inference_is_running()) {
                                 return;
                             }
                         }
                         txtASRInfo.append(content);
                         int offset = txtASRInfo.getLineCount() * txtASRInfo.getLineHeight();
                         int screenHeight = KANTVUtils.getScreenHeight();
                         int maxHeight = 500;
                         KANTVLog.j(TAG, "offset:" + offset);
                         KANTVLog.j(TAG, "screenHeight:" + screenHeight);
                         if (offset > maxHeight)
                             txtASRInfo.scrollTo(0, offset - maxHeight);
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
         if (ggmljava.inference_is_running()) {
             ggmljava.inference_stop_inference();
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
         if (ggmljava.inference_is_running()) {
             ggmljava.inference_stop_inference();
         }

         resetUIAndStatus(null,true, false);
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
         if (nBenchmarkIndex < KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())
             return true;
         else
             return false;
     }

     private boolean isNCNNInference() {
         if (nBenchmarkIndex >= KANTVAIUtils.bench_type.GGML_BENCHMARK_MAX.ordinal())
             return true;
         else
             return false;
     }

     private void displayImage(String imgPath) {
         if (ivInfo != null) {
             ivInfo.setVisibility(View.INVISIBLE);
             llInfoLayout.removeView(ivInfo);
             ivInfo = null;
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
         ivInfo = new ImageView(mActivity);
         ivInfo.setLayoutParams(vlp);
         llInfoLayout.addView(ivInfo);
         llInfoLayout.setGravity(Gravity.CENTER);
         ivInfo.setImageURI(uri);
         ivInfo.setVisibility(View.VISIBLE);
         ivInfo.setScaleType(ImageView.ScaleType.FIT_CENTER);
         ivInfo.setAdjustViewBounds(true);
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

         txtASRInfo.setText("");
         btnBenchmark.setEnabled(false);
         btnBenchmark.setBackgroundColor(0xffa9a9a9);

         WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
         attributes.screenBrightness = 1.0f;
         mActivity.getWindow().setAttributes(attributes);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
     }

     private void resetUIAndStatus(String benchmarkResult, boolean removeInferenceResult, boolean dispImage) {
         isBenchmarking.set(false);
         btnBenchmark.setEnabled(true);
         btnBenchmark.setBackgroundColor(0xC3009688);

         //for LLM multimodal
         if (!dispImage) {
             if (ivInfo != null) {
                 ivInfo.setVisibility(View.INVISIBLE);
                 llInfoLayout.removeView(ivInfo);
                 ivInfo = null;
             }
             bitmapSelectedImage = null;
             pathSelectedImage = null;
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

             if (isSDModel) {
                 if ((strBenchmarkInfo != null) && (!strBenchmarkInfo.startsWith("unknown")))
                    displayImage("/sdcard/output.png");
             }
         }

         if (removeInferenceResult)
             txtASRInfo.setText("");

         isLLMModel = false;
         isSDModel = false;
         isMNISTModel = false;
         isTTSModel = false;
         isASRModel = false;
         isLLMVModel = false;
         isLLMOModel = false;

         selectModeFileName = "";
     }

     private String getBenchmarkTip() {
         String backendDesc = KANTVAIUtils.getGGMLBackendDesc(backendIndex);
         String benchmarkTip = "\nBench:" + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex) + " (model: " + selectModeFileName
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

         return benchmarkTip;
     }

     private void displayInferenceResult(String content, boolean bOnlyDisplayBenchmarkTip) {
         txtASRInfo.scrollTo(0, 0);
         if (strBenchmarkInfo.startsWith("unknown")) {
             return;
         }
         String backendDesc = KANTVAIUtils.getGGMLBackendDesc(backendIndex);
         /*
         String benchmarkTip = "\nBench:" + KANTVUtils.getBenchmarkDesc(nBenchmarkIndex) + " (model: " + selectModeFileName
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
         */

         String benchmarkTip = getBenchmarkTip();
         if (!strBenchmarkInfo.startsWith("unknown")) {
             if (!strBenchmarkInfo.startsWith("asr_result")) {
                 benchmarkTip += strBenchmarkInfo;
             }
         }

         if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
             if (!bASROK) {
                 return;
             }
             if (strBenchmarkInfo.startsWith("asr_result")) { //when got asr result, playback the audio file
                 playAudioFile();
             }
         }

         KANTVLog.j(TAG, benchmarkTip);
         String dispInfo;
         if (!bOnlyDisplayBenchmarkTip) { //compatible with previous showMsgBox
             if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal())
                 dispInfo = KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_ASR);
             else
                 dispInfo = KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_LLM);
             dispInfo += "\n\n";

             dispInfo += benchmarkTip;
             dispInfo += "\n";
         } else { // compatible with new logic since 05/05/2025
             dispInfo = benchmarkTip;
             dispInfo += "\n";
         }

         if (content != null) {
             dispInfo += content;
         }
         //don't call showMsgBox since 05/05/2025
         //KANTVUtils.showMsgBox(mActivity, dispInfo);
         txtASRInfo.append("\n");
         txtASRInfo.append(dispInfo);
     }

     private void setTextGGMLInfo(String LLMModelFileName) {
         txtGGMLInfo.setText("");
         txtGGMLInfo.append(KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_ASR));
         txtGGMLInfo.append("\n" + "AI model:" + LLMModelFileName);
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         txtGGMLInfo.append("\n");
         txtGGMLInfo.append("running timestamp:" + timestamp);
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
