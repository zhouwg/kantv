 /*
  * Copyright (c) 2024- KanTV Author
  *
  * 03-26-2024, weiguo, this is a skeleton/initial UI for study llama.cpp on Android phone
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
 import static cdeos.media.player.KANTVEvent.KANTV_INFO_LLM_FINALIZE;
 import static cdeos.media.player.KANTVEvent.KANTV_INFO_LLM_STOP;

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
 import com.cdeos.kantv.mvp.impl.LLMResearchPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.LLMResearchPresenter;
 import com.cdeos.kantv.mvp.view.LLMResearchView;
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
 import cdeos.media.player.KANTVEvent;
 import cdeos.media.player.KANTVEventListener;
 import cdeos.media.player.KANTVEventType;
 import cdeos.media.player.KANTVException;
 import cdeos.media.player.KANTVMgr;


 public class LLMResearchFragment extends BaseMvpFragment<LLMResearchPresenter> implements LLMResearchView {
     @BindView(R.id.ggmlLayoutLLM)
     LinearLayout layout;

     private static final String TAG = LLMResearchFragment.class.getName();
     TextView _txtLLMInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;

     Button _btnBenchmark;


     private int nThreadCounts = 1;
     private int benchmarkIndex = 0;
     private String strModeName = "tiny";

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;

     // https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF
     // https://huggingface.co/TheBloke/Llama-2-7B-Chat-GGUF/resolve/main/llama-2-7b-chat.Q4_K_M.gguf
     private String ggmlModelFileName = "llama-2-7b-chat.Q4_K_M.gguf"; //4.08 GB

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;


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
         _txtGGMLStatus = (TextView) mActivity.findViewById(R.id.ggmlStatusLLM);

         _txtLLMInfo.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
         displayFileStatus(CDEUtils.getDataPath() + ggmlModelFileName);

         CDELog.j(TAG, "load LLM model");
         String phoneInfo = "Device info:" + "\n"
                 + "Brand:" + Build.BRAND + "\n"
                 + "Hardware:" + Build.HARDWARE + "\n"
                 + "OS:" + "Android " + android.os.Build.VERSION.RELEASE + "\n"
                 + "Arch:" + Build.CPU_ABI ;
         _txtGGMLInfo.setText("");
         _txtGGMLInfo.append(phoneInfo + "\n");
         _txtGGMLInfo.append("Powered by llama.cpp(https://github.com/ggerganov/llama.cpp)\n");


         endTime = System.currentTimeMillis();
         CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
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
         _txtGGMLStatus.setText("");
         File modelFile = new File(modelFilePath);
         if (modelFile.exists()) {
             _txtGGMLStatus.append("model   file exist:" + modelFile.getAbsolutePath());
         } else {
             CDELog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
             _txtGGMLStatus.append("model   file not exist: " + modelFile.getAbsolutePath());
         }
     }
 }
