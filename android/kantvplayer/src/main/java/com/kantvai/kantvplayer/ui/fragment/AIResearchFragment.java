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
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.view.View;
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
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvpFragment;
import com.kantvai.kantvplayer.mvp.impl.AIResearchPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.AIResearchPresenter;
import com.kantvai.kantvplayer.mvp.view.AIResearchView;
import com.kantvai.kantvplayer.ui.fragment.ChatMessage.AttachmentType;
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

 import io.noties.markwon.Markwon;
 import kantvai.ai.KANTVAIModel;
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
     // The root layout id (R.id.airesearchLayout) was removed when the fragment
     // was rewritten as a chat-style UI; the @BindView annotation can no longer
     // bind to it. All other view bindings in this fragment use
     // mActivity.findViewById() in initView() so this drop is local.
     // See fragment_airesearch.xml for the new layout structure.

     private static final String TAG = AIResearchFragment.class.getName();
    TextView txtGGMLInfo;
    EditText txtUserInput;
    Button btnBenchmark;        // doubles as Send / Stop toggle
    Button btnSelectImage;
    Button btnSelectAudio;

    // Chat-style UI controls (added with the chat化 refactor).
    Button btnSettings;
    Button btnClear;
    Button btnRemoveAttachment;
    LinearLayout attachmentPreview;
    ImageView attachmentThumb;
    TextView attachmentPath;
    RecyclerView chatRecyclerView;
    ChatAdapter chatAdapter;

    Markwon markwon;
    String strInferenceResult;

     private static final int SELECT_IMAGE = 1;
     private static final int SELECT_AUDIO = 2;

     // Thread count is no longer configurable from UI. It's fixed to 6 to match
    // the DSP-side worker thread count (max_hw_threads - 2 on 8Elite). The value
    // is also read from ggml-hexagon's config file (default: 6) on the DSP side.
    // Mismatch between CPU-side and DSP-side thread counts causes inference
    // corruption (garbled output) due to timing races.
    private final int nThreadCounts = 6;
     private int nBenchmarkIndex = KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal();
    private int nPreviousBenchmakrIndex = 0;
    // Default model name is intentionally not hard-coded to an ASR
    // name here; the real value is seeded in initView() from
    // mSettings.getLLMModel() + arrayModelName[selectedUIIndex], since
    // the default bench type is LLM (see nBenchmarkIndex above). Leaving
    // the previous "tiny.en-q8_0" default caused a silent bench-type /
    // model-file mismatch on first launch (LLM inference trying to load
    // the ASR model file).
    private String strModeName = "";

     private boolean bASROK = true;

     // Backend type for inference parameter selection:
     //   HEXAGON_BACKEND_CDSP (3): offload to DSP (-ngl 99, flash attention)
     //   HEXAGON_BACKEND_GGML (4): CPU only (-ngl 0)
     // The actual backend implementation is decided at build time (GGML_USE_HEXAGON),
     // but backendIndex selects inference parameters at runtime.
     private int backendIndex = ggmljava.HEXAGON_BACKEND_CDSP;

     //mapping: UI spinner position → bench_type ordinal
     //arrays.xml benchType: [ASR, LLM]
     private int[] benchTypeMapping = {
         KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal(),
         KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal(),
     };

     private String selectModeFileName = "";
     private Bitmap bitmapSelectedImage = null;
     private String pathSelectedMedia = "";

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     private long nLogCounts = 0;
     private boolean isLLMModel = false;
     private boolean isMNISTModel = false;
     private boolean isTTSModel = false;
     private boolean isASRModel = false;

     //05-25-2024, add for MiniCPM-V(A GPT-4V Level Multimodal LLM, https://github.com/OpenBMB/MiniCPM-V) or other GPT-4o style Multimodal LLM)
     private boolean isMTMDModel = false; // multimodal LLM

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     private String ggmlModelFileName = "ggml-tiny.en-q8_0.bin";
     private String ggmlSampleFileName = "jfk.wav";
     private String ggmlMNISTImageFile = "mnist-5.png";
     private String ggmlMNISTModelFile = "mnist-ggml-model-f32.gguf";

     private String strDefaultPrompt = "introduce the movie Once Upon a Time in America briefly\n";
     private String strUserInput = "introduce the movie Once Upon a Time in America briefly\n";
     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;
     private KANTVMgr mKANTVMgr = null;
     private AIResearchFragment.MyEventListener mEventListener = new AIResearchFragment.MyEventListener();

     private KANTVAIModelMgr AIModelMgr = KANTVAIModelMgr.getInstance();
     private int selectModelIndex = KANTVAIModelMgr.getInstance().getDefaultModelIndex(); //index of the default LLM model
     private int selectedUIIndex  = 0; //index of user's selected model in all models(ASR model and LLM models)

     //=============================================================================================
     private String[]        arrayModelName;
     private String[]        arrayBenchType;
     private void initLLMModels() {
         arrayModelName = AIModelMgr.getAllAIModelNickName();
         arrayBenchType = AIModelMgr.getAllAIModelBenchType();
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
         backendIndex = mSettings.getLLMbackend();
         Resources res = mActivity.getResources();

         txtGGMLInfo = mActivity.findViewById(R.id.ggmlInfo);
        btnBenchmark = mActivity.findViewById(R.id.btnBenchmark);
        btnSelectImage = mActivity.findViewById(R.id.btnSelectImage);
        btnSelectAudio = mActivity.findViewById(R.id.btnSelectAudio);
        btnSettings = mActivity.findViewById(R.id.btnSettings);
        btnClear = mActivity.findViewById(R.id.btnClear);
        btnRemoveAttachment = mActivity.findViewById(R.id.btnRemoveAttachment);
        attachmentPreview = mActivity.findViewById(R.id.attachmentPreview);
        attachmentThumb = mActivity.findViewById(R.id.attachmentThumb);
        attachmentPath = mActivity.findViewById(R.id.attachmentPath);
        txtUserInput = mActivity.findViewById(R.id.txtPrompt);
        markwon = Markwon.create(mActivity);

        // Chat-style UI: RecyclerView + ChatAdapter replaces the old
        // giant asrInfo TextView. Streaming tokens from the native side
        // land in the most recent assistant bubble (appendToLast), and
        // the fragment scrolls to keep the new content visible.
        chatRecyclerView = mActivity.findViewById(R.id.chatRecyclerView);
        chatAdapter = new ChatAdapter(mActivity, markwon);
        chatRecyclerView.setLayoutManager(new LinearLayoutManager(mActivity));
        chatRecyclerView.setAdapter(chatAdapter);
        chatRecyclerView.setItemViewCacheSize(8);

         initLLMModels();

         try {
             KANTVLibraryLoader.load("kantv-ai");
             KANTVLog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         KANTVLog.j(TAG, "set ggml's whisper.cpp info");
         {
             KANTVAIModel defaultModel = AIModelMgr.getKANTVAIModelFromName("Gemma3-4B");
             if (defaultModel != null) {
                 setTextGGMLInfo(defaultModel.getName());
             } else {
                 KANTVLog.j(TAG, "warning: Gemma3-4B not found in model list, skip setTextGGMLInfo");
             }
         }

        // Seed the model selection from the LLM Setting page so the chat
        // screen picks up the user's last choice without re-asking.
        try {
            int llmIdx = mSettings.getLLMModel();
            int nonLlm = AIModelMgr.getNonLLMModelCounts();
            int llmCount = AIModelMgr.getLLMModelCounts();
            if (llmIdx >= 0 && llmIdx < llmCount) {
                selectModelIndex = llmIdx;
                selectedUIIndex = llmIdx + nonLlm;
            } else if (llmCount > 0 && nonLlm < arrayModelName.length) {
                // Fresh install / out-of-range stored value: fall back
                // to the first LLM model so nBenchmarkIndex=LLM and
                // strModeName agree on day one. Without this the default
                // bench type is LLM but the default model file would
                // still be the ASR one, and the first Send would crash
                // trying to load ggml-tiny.en-q8_0.bin as an LLM.
                selectModelIndex = 0;
                selectedUIIndex = nonLlm;
            }
        } catch (Exception ex) {
            KANTVLog.j(TAG, "getLLMModel failed: " + ex.toString());
        }
        // Sync strModeName with the seeded selectedUIIndex so the model
        // file the runInference path constructs ("ggml-" + strModeName +
        // ".bin") matches the bench type. The Bench dialog OK handler
        // also re-sets this, but that only runs when the user opens the
        // dialog - first-launch Send without opening it needs this too.
        if (selectedUIIndex >= 0
                && arrayModelName != null
                && selectedUIIndex < arrayModelName.length) {
            strModeName = arrayModelName[selectedUIIndex];
        }

        // If the previous app launch left an "ASR init failed" flag in
        // SharedPreferences (set by IApplication when ggml_jni.asr_init
        // returned non-zero - typically because the whisper model file
        // picked in ASRSetting is missing on disk), surface an
        // actionable toast right here. Without this the user would
        // only learn about the broken ASR subsystem after clicking
        // Send, when the cryptic "asr instance not initialized" JNI
        // error pops up in the chat. The flag is cleared after
        // showing so the toast only fires once per failed launch.
        try {
            android.content.SharedPreferences prefs =
                    android.preference.PreferenceManager.getDefaultSharedPreferences(mContext);
            if (prefs.getBoolean(IApplication.KEY_ASR_INIT_FAILED, false)) {
                Toast.makeText(mContext,
                        "ASR subsystem failed to initialize. Open ASR Setting to verify the whisper model file is available on disk.",
                        Toast.LENGTH_LONG).show();
                prefs.edit().putBoolean(IApplication.KEY_ASR_INIT_FAILED, false).apply();
            }
        } catch (Exception ex) {
            KANTVLog.j(TAG, "ASR init flag check failed: " + ex.toString());
        }

         // Thread count spinner removed: nThreadCounts is fixed to 6 to match
         // the DSP-side worker thread count. See comment at nThreadCounts declaration.
         //backend spinner removed: backend is decided at build time (android_qcom vs android_non_qcom)

         btnSelectImage.setOnClickListener(v -> {
             resetUIAndStatus(null, true, false);
             Intent intent = new Intent(Intent.ACTION_PICK);
             intent.setType("image/*");
             startActivityForResult(intent, SELECT_IMAGE);
         });

         btnSelectAudio.setOnClickListener(v -> {
             resetUIAndStatus(null, true, false);
             Intent intent = new Intent(Intent.ACTION_PICK);
             intent.setType("audio/*");
             startActivityForResult(intent, SELECT_AUDIO);
         });

         // Single Send/Stop toggle button: dispatch to handleSend() when
         // idle and handleStop() while a response is streaming. The
         // button label and enabled state are kept in sync via
         // updateSendStopButton() (called from the inference lifecycle).
         btnBenchmark.setOnClickListener(v -> {
             if (isBenchmarking.get()) {
                 handleStop();
             } else {
                 handleSend();
             }
         });

         btnSettings.setOnClickListener(v -> showSettingsDialog());
         btnClear.setOnClickListener(v -> handleClear());
         btnRemoveAttachment.setOnClickListener(v -> clearAttachment());

         // "send" IME action on the prompt EditText behaves like the Send
         // button. We only fire if the user is not already waiting on a
         // response (the Send button is disabled in that case, but the IME
         // action would otherwise be unguarded).
         txtUserInput.setOnEditorActionListener((v, actionId, event) -> {
             if (actionId == android.view.inputmethod.EditorInfo.IME_ACTION_SEND) {
                 if (!isBenchmarking.get()) {
                     handleSend();
                 }
                 return true;
             }
             return false;
         });

         // Initial state: idle -> "Send" enabled.
         btnBenchmark.setText("Send");
         btnBenchmark.setEnabled(true);

         endTime = System.currentTimeMillis();
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     private final void launchGGMLBenchmarkThread() {
         Thread workThread = new Thread(new Runnable() {
             @RequiresApi(api = Build.VERSION_CODES.O)
             @Override
             public void run() {
                 strBenchmarkInfo = "";

                 // Refresh backendIndex from Settings in case user changed it in LLM Settings
                 // after initView() was called
                 backendIndex = mSettings.getLLMbackend();
                 KANTVLog.g(TAG, "backendIndex: " + backendIndex);

                 initKANTVMgr();

                 while (isBenchmarking.get()) {
                     beginTime = System.currentTimeMillis();
                     ggmljava.ggml_set_benchmark_status(0);

                     if (isLLMModel) {
                         if (isMTMDModel) {
                             //LLM multimodal inference
                             KANTVLog.g(TAG, "multimodal model, media path:" + pathSelectedMedia);
                             if (KANTVAIUtils.isImageFile(pathSelectedMedia)) {
                                 strBenchmarkInfo = ggmljava.mtmd_inference(
                                        KANTVUtils.getSDCardDataPath() + AIModelMgr.getModelName(selectModelIndex),
                                        KANTVUtils.getSDCardDataPath() + AIModelMgr.getMMProjmodelName(selectModelIndex),
                                        pathSelectedMedia,
                                        strUserInput,
                                        1,
                                        backendIndex);
                            } else if (KANTVAIUtils.isAudioFile(pathSelectedMedia)) {
                                strBenchmarkInfo = ggmljava.mtmd_inference(
                                        KANTVUtils.getSDCardDataPath() + AIModelMgr.getModelName(selectModelIndex),
                                        KANTVUtils.getSDCardDataPath() + AIModelMgr.getMMProjmodelName(selectModelIndex),
                                        pathSelectedMedia,
                                        strUserInput,
                                        2,
                                        backendIndex);
                             } else {
                                 endTime = System.currentTimeMillis();
                                 duration = (endTime - beginTime);
                                 isBenchmarking.set(false);
                                 KANTVUtils.showMsgBox(mActivity, "only support MTMD audio and image currently");
                                 return;
                             }
                         } else {
                             //general LLM inference
                            strBenchmarkInfo = ggmljava.llm_inference(
                                    KANTVUtils.getSDCardDataPath() + AIModelMgr.getModelName(selectModelIndex),
                                    strUserInput,
                                    1,
                                    backendIndex);
                         }
                     } else if (isMNISTModel) {
                         //MNIST inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                ggmlModelFileName,
                                KANTVUtils.getDataPath() + ggmlMNISTImageFile,
                                nBenchmarkIndex,
                                backendIndex);
                    } else if (isTTSModel) {
                         //TTS inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                ggmlModelFileName,
                                "this is an audio generated by bark.cpp"/*strUserInput*/,
                                nBenchmarkIndex,
                                backendIndex);
                     } else {
                         //ASR inference
                         strBenchmarkInfo = ggmljava.ggml_bench(
                                ggmlModelFileName,
                                KANTVUtils.getDataPath() + ggmlSampleFileName,
                                nBenchmarkIndex,
                                backendIndex);
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
         // The user may have changed the backend in LLMSettingFragment
         // (or another settings page) while this fragment was hidden.
         // Re-read from Settings so the top info bar always shows the
         // currently selected backend (NPU vs CPU), and so that the
         // next inference uses the new value. launchGGMLBenchmarkThread
         // also refreshes this, but that only happens when the user
         // actually starts an inference - we want the top bar to be
         // correct immediately on tab-switch.
         if (mSettings == null || txtGGMLInfo == null) {
             return;
         }
         int storedBackend = mSettings.getLLMbackend();
         if (storedBackend != backendIndex) {
             backendIndex = storedBackend;
             setTextGGMLInfo(strModeName);
         }
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
                    pathSelectedMedia = imgPath;
                    KANTVLog.g(TAG, "image path:" + imgPath);
                    // Chat-style UI: surface the picked image in the
                    // attachment-preview row above the input bar. The
                    // image is also embedded into the user message
                    // bubble by ChatAdapter when handleSend() is called.
                    showAttachmentPreview(imgPath, AttachmentType.IMAGE);
                }
             } catch (Exception exception) {
                 KANTVLog.g(TAG, "error occurred: " + exception.toString());
                 KANTVUtils.showMsgBox(mActivity, "error occurred: " + exception.toString());
             }
         }

         if ((requestCode == SELECT_AUDIO) && (null != data)) {
             Uri selectedAudioUri = data.getData();
             try {
                 String[] proj = {MediaStore.Images.Media.DATA};
                 CursorLoader loader = new CursorLoader(mContext, selectedAudioUri, proj, null, null, null);
                 Cursor cursor = loader.loadInBackground();
                 int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                 cursor.moveToFirst();
                 String realImagePath = cursor.getString(column_index);
                 cursor.close();
                 KANTVLog.g(TAG, "realAudioPath " + realImagePath);
                 selectedAudioUri = Uri.fromFile(new File(realImagePath));

                 String audioPath = selectedAudioUri.getPath();
                KANTVLog.g(TAG, "audio path:" + audioPath);
                if (audioPath.startsWith("/raw/"))
                    audioPath = audioPath.substring(6);
                pathSelectedMedia = audioPath;
                KANTVLog.g(TAG, "audio path:" + audioPath);
                displayAudio(audioPath);
                // Chat-style UI: also surface the picked audio in the
                // attachment-preview row above the input bar.
                showAttachmentPreview(audioPath, AttachmentType.AUDIO);
             } catch (Exception exception) {
                 KANTVLog.g(TAG, "error occurred: " + exception.toString());
                 KANTVUtils.showMsgBox(mActivity, "error occurred: " + exception.toString());
             }
         }
     }

     protected class MyEventListener implements KANTVEventListener {

        // Streaming tokens can arrive 20+ times per second on the device.
        // Touching the RecyclerView on every token causes a visible flicker
        // (each notifyItemChanged re-binds the bubble, each smoothScroll
        // restarts its animation before the previous one finishes). To avoid
        // that, we batch tokens into an 80ms window: the native side keeps
        // appending to mPendingChunk on the UI thread (we already hop here
        // from the workThread in onEvent), and a single runnable flushes
        // the accumulated chunk into the adapter at a fixed rate.
        //
        // The first version of this used "removeCallbacks + postDelayed"
        // on every chunk, which is broken: when tokens arrive faster than
        // STREAMING_BATCH_MS (50ms/token is typical on Hexagon NPU), the
        // pending runnable gets cancelled before it ever fires, so the
        // buffer is never flushed until the terminal llama-timings event
        // calls flushStreamingChunk() - the user sees "..." the whole
        // time. The fix is to only SCHEDULE the runnable once per window
        // (use mFlushScheduled as a guard) and clear the guard inside the
        // runnable so a chunk that lands during the flush can re-arm it.
        private static final long STREAMING_BATCH_MS = 80L;
        private final Handler mStreamHandler = new Handler(Looper.getMainLooper());
        private final StringBuilder mPendingChunk = new StringBuilder();
        // Guarded by mPendingChunk's monitor: true means a flush runnable
        // is already pending in the Handler queue. New chunks just append
        // to the buffer; they don't (and must not) re-schedule the runnable.
        private boolean mFlushScheduled = false;
        private final Runnable mFlushChunk = new Runnable() {
            @Override
            public void run() {
                // Clear the guard FIRST so a chunk that lands between
                // here and the postDelayed at the end can re-arm us.
                mFlushScheduled = false;
                String chunk;
                synchronized (mPendingChunk) {
                    if (mPendingChunk.length() == 0) {
                        return;
                    }
                    chunk = mPendingChunk.toString();
                    mPendingChunk.setLength(0);
                }
                if (chatAdapter != null) {
                    chatAdapter.appendToLast(chunk);
                }
                // Use scrollToPosition (instant) instead of
                // smoothScrollToPosition. Smooth-scroll is the main source
                // of the up-down jitter the user observed, because each
                // token would otherwise start a fresh 250ms animation that
                // never gets to finish.
                if (chatRecyclerView != null && chatAdapter != null) {
                    chatRecyclerView.scrollToPosition(chatAdapter.getItemCount() - 1);
                }
            }
        };

        MyEventListener() {
        }

        // Called from handleEventOnUiThread for every streaming token.
        // Coalesces the chunk and schedules a single UI flush in 80ms.
        // The mFlushScheduled guard is critical: do NOT call
        // removeCallbacks+postDelayed here unconditionally, because that
        // path cancels a still-pending runnable on every fast token and
        // the buffer is then never flushed in real-time.
        private void enqueueStreamingChunk(String content) {
            synchronized (mPendingChunk) {
                mPendingChunk.append(content);
                if (mFlushScheduled) {
                    return;
                }
                mFlushScheduled = true;
            }
            mStreamHandler.postDelayed(mFlushChunk, STREAMING_BATCH_MS);
        }

        // Force any buffered streaming tokens to be applied to the bubble
        // immediately. Used on the last event (llama-timings / error /
        // stop) so the user never sees a 80ms "tail" of pending text after
        // the inference finishes.
        private void flushStreamingChunk() {
            mStreamHandler.removeCallbacks(mFlushChunk);
            mFlushChunk.run();
        }

        @Override
        public void onEvent(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
            // KANTVMgr is constructed in launchGGMLBenchmarkThread() (see
            // initKANTVMgr()), so this callback fires on the worker thread
            // whose Looper was current at construction time. Touching the
            // chat RecyclerView from the worker thread races with the UI
            // thread's onBindViewHolder and produces visible flicker in the
            // assistant bubble, so we always hop to the UI thread first.
            if (mActivity == null) {
                return;
            }
            final KANTVEventType evtType = eventType;
            final int w = what, a1 = arg1, a2 = arg2;
            final Object o = obj;
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    handleEventOnUiThread(evtType, w, a1, a2, o);
                }
            });
        }

        // Body of the native event handler. Always runs on the UI thread
        // (the wrapper above ensures this), so it's safe to touch the
        // chat adapter / RecyclerView / isBenchmarking / send-stop button
        // without further synchronization.
        private void handleEventOnUiThread(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
            String eventString = "got event from native layer: " + eventType.toString() + " (" + what + ":" + arg1 + " ) :" + (String) obj;
            String content = (String) obj;

            if (eventType.getValue() == KANTVEvent.KANTV_ERROR) {
                KANTVLog.j(TAG, "ERROR:" + eventString);
                // Flush any still-buffered streaming tokens so the error
                // message is the last thing the user sees, not a chunk
                // of pre-error text.
                flushStreamingChunk();
                // Chat-style error reporting: flip the most recent
                // assistant bubble into ERROR state and surface the message
                // there instead of overwriting a hidden TextView.
                if (chatAdapter != null) {
                    if (content != null && !content.isEmpty()) {
                        chatAdapter.appendToLast("\n\n[error] " + content);
                    }
                    chatAdapter.markLastError();
                }
                if (isBenchmarking.compareAndSet(true, false)) {
                    updateSendStopButton();
                }
            }

            if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                if ((arg1 == KANTV_INFO_ASR_STOP) || (arg1 == KANTV_INFO_ASR_FINALIZE)) {
                    return;
                }

                if (content.startsWith("reset")) {
                    // "reset" is sent when a new inference is about to
                    // start. The fragment has already added a fresh
                    // assistant placeholder in handleSend() so there is
                    // nothing to clear here.
                    return;
                }

                if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                    if (content.contains("not initialized")) {
                        bASROK = false;
                    }
                }

                //make UI happy when disable GGML_USE_HEXAGON manually
                if (content.startsWith("ggml-hexagon")) {
                    if (chatAdapter != null) {
                        chatAdapter.appendToLast(content + "\n");
                    }
                    return;
                }

                if (content.startsWith("unknown")) {

                } else {
                    if (content.startsWith("llama-timings")) {
                        // llama-timings is the LAST line the native side
                        // emits for an LLM response. It still has to be
                        // appended to the bubble so the user sees the
                        // timing numbers, but it's also the cue to flip
                        // the bubble into COMPLETE state.
                        KANTVLog.j(TAG, "LLM timings");
                        // Flush any still-buffered streaming tokens so the
                        // last 80ms-worth of text lands in the bubble
                        // before the COMPLETE state (and the markdown
                        // render) takes over.
                        flushStreamingChunk();
                        if (chatAdapter != null) {
                            strInferenceResult += content;
                            chatAdapter.appendToLast("\n" + content);
                            chatAdapter.markLastComplete();
                        }
                        if (isBenchmarking.compareAndSet(true, false)) {
                            updateSendStopButton();
                        }
                    } else {
                        nLogCounts++;
                        if (nLogCounts > 100) {
                            nLogCounts = 0;
                        }
                        if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                            if (!ggmljava.llm_is_running_state()) {
                                return;
                            }
                        }
                        // Streaming text: hand the chunk to the chat
                        // adapter (it will replace the "..." placeholder
                        // and notify the row), and keep a copy in
                        // strInferenceResult for any callers that want the
                        // accumulated response. Markdown rendering is
                        // applied once the bubble is COMPLETE so the user
                        // doesn't see flicker mid-stream. The actual
                        // appendToLast + scroll call is throttled to one
                        // per 80ms via enqueueStreamingChunk - see the
                        // comment on STREAMING_BATCH_MS.
                        strInferenceResult += content;
                        enqueueStreamingChunk(content);
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
         if (ggmljava.llm_is_running_state()) {
             ggmljava.llm_reset_running_state();
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
         if (ggmljava.llm_is_running_state()) {
             ggmljava.llm_reset_running_state();
         }

         resetUIAndStatus(null,true, false);
     }

     public boolean isMTMDInference() {
         if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
             if ((pathSelectedMedia != null) && (!pathSelectedMedia.isEmpty())) {
                 if (KANTVAIUtils.isMTMDModel(selectModeFileName)) {
                     return true;
                 }
             }
         }

         return false;
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

    private void displayAudio(String audioPath) {
        // Surface the attached audio path to the chat so the user gets
        // visible feedback that the file was picked, since the chat UI
        // no longer has the old txtInferenceResult TextView to append to.
        if (chatAdapter != null) {
            chatAdapter.appendToLast("[audio attached] " + audioPath + "\n");
        }
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
        // The Send/Stop toggle button flips to "Stop" (and is disabled)
        // while inference is running. The event listener flips it back
        // when llama-timings arrives, on error, or when the user hits
        // Stop.
        updateSendStopButton();

        WindowManager.LayoutParams attributes = mActivity.getWindow().getAttributes();
        attributes.screenBrightness = 1.0f;
        mActivity.getWindow().setAttributes(attributes);
        mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    private void resetUIAndStatus(String benchmarkResult, boolean removeInferenceResult, boolean dispImage) {
        isBenchmarking.set(false);
        updateSendStopButton();

        //for LLM multimodal
        if (!dispImage) {
            bitmapSelectedImage = null;
            pathSelectedMedia = null;
        } else {
            // For MNIST and MTMD models the previous UI called
            // displayImage() to re-render the processed image into a
            // hidden container (visibility=gone) that the user never
            // actually saw. In the chat UI the relevant image is
            // already embedded in the user message bubble (or the
            // built-in MNIST image is not surfaced in chat - a small
            // regression vs the old UI but acceptable since MNIST is
            // effectively abandoned in this build). Nothing to do here.
        }

        // No more txtInferenceResult to clear. The chat history is the
        // user / assistant conversation; removeInferenceResult used to
        // wipe the old log TextView and is now a no-op (kept for
        // signature compatibility with existing callers).
        if (removeInferenceResult) {
            // intentionally empty
        }

        isLLMModel = false;
        isMNISTModel = false;
        isTTSModel = false;
        isASRModel = false;
        isMTMDModel = false;

        selectModeFileName = "";
        strInferenceResult = "";
    }

     private String getBenchmarkTip() {
         String backendDesc = KANTVAIUtils.getGGMLBackendDesc(backendIndex);
        endTime = System.currentTimeMillis();
        duration = (endTime - beginTime);
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
         // The chat UI renders the full response, including the
         // "llama-timings" trailer line, inside the assistant bubble.
         // The only side-effect we still care about is ASR audio playback
         // (the user expects to hear the file after ASR completes).
         if (strBenchmarkInfo == null || strBenchmarkInfo.startsWith("unknown")) {
             return;
         }

         if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
             if (!bASROK) {
                 return;
             }
             if (strBenchmarkInfo.startsWith("asr_result")) { //when got asr result, playback the audio file
                 playAudioFile();
             }
         }
     }

     private void setTextGGMLInfo(String LLMModelFileName) {
        txtGGMLInfo.setText("");
        txtGGMLInfo.append(KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_LLM));
        // Backend status (NPU / CPU). Sits between the model name and
        // the timestamp so the user can see at a glance which compute
        // device the next / last inference ran on. NPU = Hexagon cDSP,
        // CPU = generic ggml. Colour-coded so the difference is
        // visible at a glance.
        appendBackendStatus();
        txtGGMLInfo.append(" " + "AI model:" + LLMModelFileName);
        SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
        Date date = new Date(System.currentTimeMillis());
        String timestamp = fullDateFormat.format(date);
        txtGGMLInfo.append(" " + "timestamp:" + timestamp);
    }

    // Format a short backend label and append it (coloured) to the top
    // info bar. Reads mSettings.getLLMbackend() directly each call
    // rather than the backendIndex field, so the display is always in
    // sync with the persisted setting even if some other code path
    // changes Settings without our knowledge. The backendIndex field
    // is still used by the JNI inference calls (ggml_inference,
    // asr_reset) and is refreshed from mSettings in
    // launchGGMLBenchmarkThread right before each inference, so the
    // JNI path stays correct too. Defence in depth: display and JNI
    // use different read paths.
    private void appendBackendStatus() {
        if (txtGGMLInfo == null || mSettings == null) return;
        int currentBackend = mSettings.getLLMbackend();
        String label;
        int color;
        if (currentBackend == ggmljava.HEXAGON_BACKEND_CDSP) {
            label = "Backend:NPU";
            color = 0xFF1B8E3A; // green - on-device NPU
        } else if (currentBackend == ggmljava.HEXAGON_BACKEND_GGML) {
            label = "Backend:CPU";
            color = 0xFFC62828; // red - CPU fallback
        } else {
            label = "Backend:unknown";
            color = 0xFF9E9E9E; // grey
        }
        int start = txtGGMLInfo.getText().length();
        txtGGMLInfo.append(" " + label);
        int end = txtGGMLInfo.getText().length();
        // TextView.getText() returns CharSequence, but the underlying
        // buffer is always a SpannableStringBuilder (since we use
        // append()). Cast to Spannable so we can attach a colour span
        // to just the backend label.
        CharSequence cs = txtGGMLInfo.getText();
        if (cs instanceof android.text.Spannable) {
            ((android.text.Spannable) cs).setSpan(
                    new android.text.style.ForegroundColorSpan(color),
                    start, end,
                    android.text.Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
        }
    }

    // -----------------------------------------------------------------
    // Chat-style flow: handleSend / handleStop / handleClear, plus the
    // dialog that picks bench type + model. The actual inference
    // dispatch is delegated to runInference() so the logic mirrors the
    // pre-chat refactor as closely as possible.
    // -----------------------------------------------------------------

    /**
     * Validate the current state, snapshot the user prompt into a chat
     * bubble, reserve an assistant placeholder, and kick off the same
     * sanity-check + benchmark pipeline the old button listener used to
     * run.
     */
    private void handleSend() {
        if (chatAdapter == null) {
            return;
        }
        String prompt = txtUserInput.getText().toString().trim();

        //for self-test
        if (prompt.isEmpty())
            prompt = strDefaultPrompt;

        if (prompt.isEmpty() && (pathSelectedMedia == null || pathSelectedMedia.isEmpty())) {
            Toast.makeText(mContext, "Please enter a prompt or attach a file", Toast.LENGTH_SHORT).show();
            return;
        }
        // Default prompt for image / audio attachments when the user
        // hasn't typed anything - matches the old spinnerBenchType
        // behaviour so multimodal still works out of the box.
        if (prompt.isEmpty() && pathSelectedMedia != null && !pathSelectedMedia.isEmpty()) {
            if (KANTVAIUtils.isAudioFile(pathSelectedMedia)) {
                prompt = "Pls help transcribe this file:" + pathSelectedMedia;
            } else {
                prompt = "What is in the image?";
            }
        }

        // Snapshot the attachment that the next user turn is about to
        // send. clearAttachment() runs after the chat row is added so the
        // bubble gets the right thumbnail and the preview row goes away.
        String attachedMedia = (pathSelectedMedia != null) ? pathSelectedMedia : "";
        AttachmentType attType = AttachmentType.NONE;
        if (!attachedMedia.isEmpty()) {
            attType = KANTVAIUtils.isImageFile(attachedMedia)
                    ? AttachmentType.IMAGE
                    : AttachmentType.AUDIO;
        }
        chatAdapter.addUserMessage(prompt, attachedMedia, attType);
        strUserInput = prompt;
        txtUserInput.setText("");
        clearAttachment();
        scrollToBottom();

        // Reserve the assistant bubble and reset the streaming buffer.
        chatAdapter.addAssistantPlaceholder();
        strInferenceResult = "";
        scrollToBottom();

        runInference();
    }

    /**
     * The same single-button click path also handles Stop while a
     * response is streaming. We ask the native side to abort, then
     * flip the assistant bubble to ERROR so the user can see the
     * interruption.
     */
    private void handleStop() {
        KANTVLog.g(TAG, "handleStop");
        if (ggmljava.llm_is_running_state()) {
            ggmljava.llm_reset_running_state();
        }
        if (chatAdapter != null) {
            chatAdapter.appendToLast("\n[stopped by user]");
            chatAdapter.markLastError();
        }
        resetUIAndStatus(null, true, false);
    }

    /**
     * Wipe the chat history. The bench type / model selection in
     * Settings is untouched.
     */
    private void handleClear() {
        if (chatAdapter != null) {
            chatAdapter.clear();
        }
        strInferenceResult = "";
    }

    /**
     * Settings dialog: lets the user pick bench type (ASR / LLM) and the
     * model underneath. The old "Bench type" + "Model" spinners on the
     * main surface are gone - this dialog replaces them. We create the
     * spinners in code (no separate layout file) to keep the diff small.
     */
    private void showSettingsDialog() {
        if (mActivity == null) {
            return;
        }
        initLLMModels();
        int nonLlm = AIModelMgr.getNonLLMModelCounts();
        int llmCount = AIModelMgr.getLLMModelCounts();

        // Root container for the dialog body.
        LinearLayout root = new LinearLayout(mActivity);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = (int) (16 * mActivity.getResources().getDisplayMetrics().density);
        root.setPadding(pad, pad, pad, 0);

        // ---- Bench type spinner ----
        TextView benchLabel = new TextView(mActivity);
        benchLabel.setText("Bench type");
        root.addView(benchLabel);

        Spinner benchSpinner = new Spinner(mActivity);
        benchSpinner.setId(View.generateViewId());
        // Build entries from the arrayBenchType array that the model
        // manager exposed (same data the old spinner on the main
        // surface used).
        ArrayAdapter<String> benchAdapter = new ArrayAdapter<String>(
                mActivity, android.R.layout.simple_spinner_dropdown_item, arrayBenchType);
        benchSpinner.setAdapter(benchAdapter);
        // Seed selection from current nBenchmarkIndex.
        int currentBenchPos = 0;
        for (int i = 0; i < benchTypeMapping.length; i++) {
            if (benchTypeMapping[i] == nBenchmarkIndex) {
                currentBenchPos = i;
                break;
            }
        }
        benchSpinner.setSelection(currentBenchPos);
        root.addView(benchSpinner);

        // ---- Model spinner ----
        TextView modelLabel = new TextView(mActivity);
        modelLabel.setText("Model");
        modelLabel.setPadding(0, pad, 0, 0);
        root.addView(modelLabel);

        Spinner modelSpinner = new Spinner(mActivity);
        modelSpinner.setId(View.generateViewId());

        // The model list shown depends on the bench type: ASR shows the
        // single ASR model slot, LLM shows the LLM model list.
        Runnable refreshModels = () -> {
            int benchPos = benchSpinner.getSelectedItemPosition();
            int benchIdx = (benchPos >= 0 && benchPos < benchTypeMapping.length)
                    ? benchTypeMapping[benchPos]
                    : nBenchmarkIndex;
            String[] modelEntries;
            int[] modelIndices; // mapping: list-pos -> global model index in AIModelMgr
            if (benchIdx == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                int asrLen = Math.max(0, Math.min(nonLlm, arrayModelName.length));
                modelEntries = new String[asrLen];
                modelIndices = new int[asrLen];
                for (int i = 0; i < asrLen; i++) {
                    modelEntries[i] = arrayModelName[i];
                    modelIndices[i] = i;
                }
            } else {
                int n = Math.max(0, Math.min(llmCount, arrayModelName.length - nonLlm));
                modelEntries = new String[n];
                modelIndices = new int[n];
                for (int i = 0; i < n; i++) {
                    modelEntries[i] = arrayModelName[i + nonLlm];
                    modelIndices[i] = i + nonLlm;
                }
            }
            ArrayAdapter<String> modelAdapter = new ArrayAdapter<String>(
                    mActivity, android.R.layout.simple_spinner_dropdown_item, modelEntries);
            modelSpinner.setAdapter(modelAdapter);

            // Try to keep the current model selection if it's still in
            // the new list. Otherwise pick the first entry.
            int targetGlobal = -1;
            if (benchIdx == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                if (nBenchmarkIndex == benchIdx) {
                    targetGlobal = selectedUIIndex; // ASR slot
                }
            } else {
                targetGlobal = nonLlm + selectModelIndex;
            }
            int targetListPos = 0;
            for (int i = 0; i < modelIndices.length; i++) {
                if (modelIndices[i] == targetGlobal) {
                    targetListPos = i;
                    break;
                }
            }
            if (modelIndices.length > 0) {
                modelSpinner.setSelection(targetListPos);
            }
            // Stash the mapping on the spinner tag for retrieval.
            modelSpinner.setTag(modelIndices);
        };
        refreshModels.run();
        benchSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                refreshModels.run();
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {}
        });
        root.addView(modelSpinner);

        new AlertDialog.Builder(mActivity)
                .setTitle("Bench / Model")
                .setView(root)
                .setPositiveButton("OK", (dialog, which) -> {
                    int benchPos = benchSpinner.getSelectedItemPosition();
                    if (benchPos >= 0 && benchPos < benchTypeMapping.length) {
                        nBenchmarkIndex = benchTypeMapping[benchPos];
                    }
                    int modelListPos = modelSpinner.getSelectedItemPosition();
                    int[] modelIndices = (int[]) modelSpinner.getTag();
                    if (modelIndices != null && modelListPos >= 0 && modelListPos < modelIndices.length) {
                        int globalIdx = modelIndices[modelListPos];
                        selectedUIIndex = globalIdx;
                        if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                            selectModelIndex = globalIdx - nonLlm;
                            // Persist the LLM choice so the LLM Setting
                            // page and the next launch see the same
                            // selection.
                            try {
                                mSettings.updateLLMModelIndex(selectModelIndex);
                            } catch (Exception ex) {
                                KANTVLog.j(TAG, "updateLLMModelIndex failed: " + ex.toString());
                            }
                        }
                        strModeName = arrayModelName[globalIdx];
                        if (globalIdx >= 0 && globalIdx < arrayModelName.length) {
                            KANTVAIModel m = AIModelMgr.getKANTVAIModelFromName(strModeName);
                            if (m != null) {
                                setTextGGMLInfo(m.getName());
                            }
                        }
                    }
                    // When the user switches bench types in the dialog we
                    // bring the prompt box into the expected default
                    // state for that mode:
                    //   - ASR -> "help to transcribe the audio file"
                    //     so an audio-only Send (no typing) is accepted
                    //     by the Send-without-text guard in handleSend()
                    //   - LLM -> "" (so the EditText's hint "Ask AI..."
                    //     shows again), but ONLY if the user hasn't
                    //     already typed a real prompt of their own. We
                    //     treat the ASR default string as the "default
                    //     placeholder" and only clear when the box
                    //     contains exactly that - any other text is the
                    //     user's and must be preserved.
                    if (txtUserInput != null) {
                        final String asrDefault = "help to transcribe the audio file";
                        if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_ASR.ordinal()) {
                            txtUserInput.setText(asrDefault);
                            txtUserInput.setSelection(txtUserInput.getText().length());
                        } else if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                            String current = txtUserInput.getText().toString();
                            if (asrDefault.equals(current)) {
                                txtUserInput.setText("");
                            }
                        }
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    /**
     * Clear the pending attachment (image / audio) and hide the
     * preview row above the input.
     */
    private void clearAttachment() {
        pathSelectedMedia = "";
        bitmapSelectedImage = null;
        if (attachmentPreview != null) {
            attachmentPreview.setVisibility(View.GONE);
        }
        if (attachmentThumb != null) {
            attachmentThumb.setImageDrawable(null);
        }
        if (attachmentPath != null) {
            attachmentPath.setText("");
        }
    }

    /**
     * Update the attachment preview row when the user picks a file.
     */
    private void showAttachmentPreview(String path, AttachmentType type) {
        if (attachmentPreview == null) {
            return;
        }
        if (path == null || path.isEmpty()) {
            attachmentPreview.setVisibility(View.GONE);
            return;
        }
        attachmentPreview.setVisibility(View.VISIBLE);
        if (attachmentPath != null) {
            attachmentPath.setText(path);
        }
        if (attachmentThumb != null) {
            if (type == AttachmentType.IMAGE) {
                try {
                    attachmentThumb.setImageURI(Uri.fromFile(new File(path)));
                } catch (Exception e) {
                    KANTVLog.j(TAG, "failed to load attachment thumb: " + e.toString());
                }
            } else {
                attachmentThumb.setImageResource(android.R.drawable.ic_media_play);
            }
        }
    }

    /**
     * Keep the chat list pinned to the most recent row. Posting through
     * the RecyclerView makes sure we run after the layout pass, so the
     * item count is up-to-date and smoothScrollToPosition() can land on
     * a valid index.
     */
    private void scrollToBottom() {
        if (chatRecyclerView == null || chatAdapter == null) {
            return;
        }
        chatRecyclerView.post(() -> {
            int count = chatAdapter.getItemCount();
            if (count > 0) {
                chatRecyclerView.smoothScrollToPosition(count - 1);
            }
        });
    }

    /**
     * Flip the Send/Stop button label and enabled state to match
     * isBenchmarking. Called from the inference lifecycle (init,
     * reset, on completion / error in the event listener).
     */
    private void updateSendStopButton() {
        if (btnBenchmark == null) {
            return;
        }
        boolean running = isBenchmarking.get();
        if (running) {
            btnBenchmark.setText("Stop");
            btnBenchmark.setEnabled(true);
            btnBenchmark.setBackgroundColor(0xffa9a9a9);
        } else {
            btnBenchmark.setText("Send");
            btnBenchmark.setEnabled(true);
            btnBenchmark.setBackgroundColor(0xC3009688);
        }
    }

    /**
     * The same validation + benchmark dispatch the old btnBenchmark
     * listener ran. Lives in its own method so handleSend() can call
     * it without dragging the click-lambda scaffolding with it.
     */
    private void runInference() {
        KANTVLog.j(TAG, "selectUIIndex:" + selectedUIIndex);
        KANTVLog.j(TAG, "selectModeIndex:" + selectModelIndex);
        if (arrayModelName != null && selectedUIIndex >= 0 && selectedUIIndex < arrayModelName.length) {
            KANTVLog.g(TAG, "strModeName:" + arrayModelName[selectedUIIndex]);
        }
        KANTVLog.j(TAG, "exec ggml benchmark: type: " + KANTVAIUtils.getBenchmarkDesc(nBenchmarkIndex)
                + ", threads:" + nThreadCounts + ", model:" + strModeName);
        String selectModelFilePath = "";

        resetUIAndStatus(null, true, true);

        //sanity check begin
        {
            isASRModel = KANTVAIUtils.isASRModel(strModeName);
            if (nBenchmarkIndex == KANTVAIUtils.bench_type.GGML_BENCHMARK_LLM.ordinal()) {
                if (selectModelIndex < 0) {
                    selectModelIndex = 0;
                    selectModeFileName = "ggml-" + strModeName + ".bin";
                } else {
                    KANTVAIModel m = AIModelMgr.getKANTVAIModelFromName(strModeName);
                    if (m != null) {
                        selectModeFileName = m.getName();
                    }
                }
                isLLMModel = true;
            } else {
                KANTVAIModel m = AIModelMgr.getKANTVAIModelFromName(strModeName);
                if (m != null) {
                    selectModeFileName = m.getName();
                }
            }
            if (!selectModeFileName.isEmpty()) {
                setTextGGMLInfo(selectModeFileName);
            }
            KANTVLog.g(TAG, "selectModeFileName:" + selectModeFileName);

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

            // ASR pre-check: if the ASR subsystem was never successfully
            // initialized (typically because the whisper model picked in
            // ASRSettingFragment is missing on disk, see IApplication.
            // KEY_ASR_INIT_FAILED flag), bail out early with a clear,
            // actionable error in the chat bubble. Without this the
            // user would just see the cryptic JNI "asr instance not
            // initialized" string after a 20-second wait.
            if (isASRModel && !KANTVAIUtils.getASRSubsystemInit()) {
                String msg = "ASR subsystem is not initialized. " +
                        "The whisper model file (" + selectModeFileName +
                        ") may be missing on disk. " +
                        "Open ASR Setting to verify or pick a different model.";
                KANTVLog.j(TAG, msg);
                if (chatAdapter != null) {
                    chatAdapter.appendToLast("\n\n[error] " + msg);
                    chatAdapter.markLastError();
                }
                if (isBenchmarking.compareAndSet(true, false)) {
                    updateSendStopButton();
                }
                return;
            }

            if ((pathSelectedMedia != null) && (!pathSelectedMedia.isEmpty())) {
                if (KANTVAIUtils.isMTMDModel(selectModeFileName)) {
                    isMTMDModel = true;
                    if (KANTVAIUtils.isAudioFile(pathSelectedMedia)) {
                        strUserInput = "Pls help transcribe this file:" + pathSelectedMedia;
                    } else {
                        if (strUserInput == null || strUserInput.trim().isEmpty()) {
                            strUserInput = "What is in the image?";
                        }
                    }
                    File mmprModelFile = new File(KANTVUtils.getSDCardDataPath() + AIModelMgr.getMMProjmodelName(selectModelIndex));
                    if (!mmprModelFile.exists()) {
                        KANTVUtils.showMsgBox(mActivity, "LLM mmproj model file:" +
                                AIModelMgr.getMMProjmodelName(selectModelIndex) +
                                " not exist, pls download from: "
                                + AIModelMgr.getMMProjmodelUrl(selectModelIndex) + " in LLM Setting");
                        if (chatAdapter != null) {
                            chatAdapter.markLastError();
                        }
                        return;
                    }
                }
            }

            // Old code did `if (!isMNISTModel && !isMTMDModel) ivInfo = null;`
            // here. The ivInfo/llInfoLayout pair is gone; the chat UI
            // handles image visibility through the user message bubble
            // and the attachment-preview row, so nothing to clear.

            if (isMTMDModel) {
                if (KANTVAIUtils.isImageFile(pathSelectedMedia)) {
                    if ((bitmapSelectedImage == null) || (pathSelectedMedia.isEmpty())) {
                        KANTVLog.j(TAG, "image is empty");
                        KANTVUtils.showMsgBox(mActivity, "please select a image for LLM multimodal inference");
                        if (chatAdapter != null) {
                            chatAdapter.markLastError();
                        }
                        return;
                    }
                }
            }

            if (isASRModel) {
                selectModelFilePath = KANTVUtils.getDataPath(mContext) + selectModeFileName;
            } else {
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

            if (isMTMDModel) {
                if (KANTVAIUtils.isImageFile(pathSelectedMedia)) {
                    if (!KANTVAIUtils.isMTMD_ImageModel(selectModeFile.getAbsolutePath())) {
                        KANTVUtils.showMsgBox(mActivity, "selected image file " + pathSelectedMedia
                                + ", but the selected multimodal model:" + selectModeFile.getAbsolutePath() + " doesn't hava image capability"
                        );
                        if (chatAdapter != null) {
                            chatAdapter.markLastError();
                        }
                        return;
                    }
                }

                if (KANTVAIUtils.isAudioFile(pathSelectedMedia)) {
                    if (!KANTVAIUtils.isMTMD_AudioModel(selectModeFile.getAbsolutePath())) {
                        KANTVUtils.showMsgBox(mActivity, "selected audio file " + pathSelectedMedia
                                + ", but the selected multimodal model:" + selectModeFile.getAbsolutePath() + " doesn't hava audio capability"
                        );
                        if (chatAdapter != null) {
                            chatAdapter.markLastError();
                        }
                        return;
                    }
                }
            }

            if (isASRModel) {
                if (!selectModeFile.exists() || (!sampleFile.exists())) {
                    KANTVUtils.showMsgBox(mActivity, "pls check whether model file:" +
                            selectModeFile.getAbsolutePath() + " and sample file:" + sampleFile.getAbsolutePath() + " exist");
                    if (chatAdapter != null) {
                        chatAdapter.markLastError();
                    }
                    return;
                }
            } else {
                if (!selectModeFile.exists()) {
                    KANTVUtils.showMsgBox(mActivity, "LLM model file:" +
                            selectModeFile.getAbsolutePath() + " not exist, pls download from: "
                            + AIModelMgr.getModelUrl(selectModelIndex) + " in LLM Setting");
                    if (chatAdapter != null) {
                        chatAdapter.markLastError();
                    }
                    return;
                }
            }

            if (strUserInput == null) {
                strUserInput = "";
            }
            strUserInput = strUserInput.trim();
            KANTVLog.j(TAG, "user input: \n " + strUserInput);
        }
        //sanity check end

        ggmlModelFileName = selectModelFilePath;
        KANTVLog.j(TAG, "model file:" + ggmlModelFileName);
        if (isASRModel) {
            int result = ggmljava.asr_reset(selectModelFilePath, KANTVAIUtils.ASR_MODE_BECHMARK, backendIndex);
            if (0 != result) {
                KANTVLog.j(TAG, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                KANTVUtils.showMsgBox(mActivity, "failed to initialize ASR, pls restart APP before ensure necessary permission has granted to APP and ensure select tiny.en-q8_0 in ASR Setting");
                if (chatAdapter != null) {
                    chatAdapter.markLastError();
                }
                return;
            }
        }

        nLogCounts = 0;
        initUIAndStatus();
        launchGGMLBenchmarkThread();
    }

    public static native int kantv_anti_remove_rename_this_file();
 }
