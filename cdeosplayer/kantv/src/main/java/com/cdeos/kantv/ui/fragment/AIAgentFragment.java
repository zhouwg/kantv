// ref&author:https://github.com/nihui/ncnn-android-scrfd

// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.


/*
 * Copyright (c) 2024- KanTV Author
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
 import android.content.pm.ActivityInfo;
 import android.content.res.Configuration;
 import android.content.res.Resources;
 import android.graphics.PixelFormat;
 import android.hardware.Camera;
 import android.media.MediaPlayer;
 import android.os.Build;
 import android.text.method.ScrollingMovementMethod;
 import android.view.MenuItem;
 import android.view.Surface;
 import android.view.SurfaceHolder;
 import android.view.SurfaceView;
 import android.view.View;
 import android.view.Window;
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
 import androidx.core.content.ContextCompat;

 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.mvp.impl.AIAgentPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.AIAgentPresenter;
 import com.cdeos.kantv.mvp.view.AIAgentView;
 import com.cdeos.kantv.utils.Settings;


 import org.ggml.ggmljava;
 import org.ncnn.ncnnjava;

 import java.io.File;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.io.RandomAccessFile;
 import java.net.SocketException;
 import java.nio.ByteBuffer;
 import java.nio.ByteOrder;
 import java.text.SimpleDateFormat;
 import java.util.Date;
 import java.util.Random;
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


 public class AIAgentFragment extends BaseMvpFragment<AIAgentPresenter> implements AIAgentView, SurfaceHolder.Callback {
     private static final String TAG = AIAgentFragment.class.getName();

     private String strBackend = "cpu";
     private int backendIndex = 0; //QNN_CPU

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     TextView _txtLLMInfo;
     TextView _txtNCNNInfo;

     private KANTVMgr mKANTVMgr = null;
     private AIAgentFragment.MyEventListener mEventListener = new AIAgentFragment.MyEventListener();

     private boolean mCameraInit = false;

     private ncnnjava ncnnjni = new ncnnjava();
     private int facing = 0;

     private Spinner spinnerModel;
     private Spinner spinnerCPUGPU;
     private int current_model = 0;
     private int current_cpugpu = 0;

     private SurfaceView cameraView;

     public static AIAgentFragment newInstance() {
         return new AIAgentFragment();
     }

     @NonNull
     @Override
     protected AIAgentPresenter initPresenter() {
         return new AIAgentPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_agent;
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

         _txtNCNNInfo = mActivity.findViewById(R.id.ncnnlInfo);


         mActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
         mActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
         mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);


         try {
             CDELibraryLoader.load("ggml-jni");
             CDELog.j(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize ggml jni");
             return;
         }

         String systemInfo = ggmljava.llm_get_systeminfo();
         String phoneInfo = "Device:" + " "
                 + Build.BRAND + " "
                 + Build.HARDWARE + " "
                 + "Android " + android.os.Build.VERSION.RELEASE + " "
                 + "Arch:" + Build.CPU_ABI;
         CDELog.j(TAG, phoneInfo);
         _txtNCNNInfo.setText("");
         _txtNCNNInfo.append(phoneInfo + ",");
         _txtNCNNInfo.append("Powered by NCNN(https://github.com/Tencent/ncnn)\n");

         try {
             initKANTVMgr();
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         cameraView = mActivity.findViewById(R.id.cameraview);

         cameraView.getHolder().setFormat(PixelFormat.RGBA_8888);
         cameraView.getHolder().addCallback(this);

         Button buttonSwitchCamera = mActivity.findViewById(R.id.buttonSwitchCamera);
         buttonSwitchCamera.setOnClickListener(arg0 -> {

             int new_facing = 1 - facing;

             ncnnjni.closeCamera();

             ncnnjni.openCamera(new_facing);

             facing = new_facing;
         });

         spinnerModel = mActivity.findViewById(R.id.spinnerModel);
         spinnerModel.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
             {
                 if (position != current_model)
                 {
                     current_model = position;
                     reload();
                 }
             }

             @Override
             public void onNothingSelected(AdapterView<?> arg0)
             {
             }
         });

         spinnerCPUGPU = mActivity.findViewById(R.id.spinnerCPUGPU);
         spinnerCPUGPU.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
             @Override
             public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
             {
                 if (position != current_cpugpu)
                 {
                     current_cpugpu = position;
                     reload();
                 }
             }

             @Override
             public void onNothingSelected(AdapterView<?> arg0)
             {
             }
         });

         reload();

         endTime = System.currentTimeMillis();
         CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }


     @Override
     public void initListener() {

     }

     private void reload()
     {
         boolean ret_init = ncnnjni.loadModel(mContext.getAssets(), 0, current_model, current_cpugpu);
         if (!ret_init)
         {
             CDELog.j(TAG, "ncnn loadModel failed");
         }
     }

     @Override
     public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
     {
         ncnnjni.setOutputWindow(holder.getSurface());
     }

     @Override
     public void surfaceCreated(SurfaceHolder holder)
     {
     }

     @Override
     public void surfaceDestroyed(SurfaceHolder holder)
     {
     }

     public void initCamera() {
         if (mCameraInit) {
             CDELog.j(TAG, "camera already initialized");
             return;
         }
         getPreviewRotateDegree();

         CDELog.j(TAG, "running service: " + CDEUtils.getRunningServicesInfo(mContext));
         try {
             ncnnjni.openCamera(facing);
             mCameraInit = true;
         } catch (Exception e) {
             e.printStackTrace();
             CDELog.j(TAG, "init failed:" + e.getMessage());
             Toast.makeText(mContext, "init failed:" + e.getMessage(), Toast.LENGTH_SHORT).show();
         }

     }

     public void finalizeCamera() {
         if (mCameraInit) {
             ncnnjni.closeCamera();
             mCameraInit = false;
         } else {
             CDELog.j(TAG, "camera already finalized");
         }
     }


     private int getPreviewRotateDegree() {
         int result = 0;
         int phoneDegree = 0;
         try {
             int phoneRotate = mActivity.getWindowManager().getDefaultDisplay().getRotation();
             switch (phoneRotate) {
                 case Surface.ROTATION_0:
                     CDELog.j(TAG, "ROTATION_0");
                     phoneDegree = 0;
                     break;
                 case Surface.ROTATION_90:
                     CDELog.j(TAG, "ROTATION_90");
                     phoneDegree = 90;
                     break;
                 case Surface.ROTATION_180:
                     CDELog.j(TAG, "ROTATION_180");
                     phoneDegree = 180;
                     break;
                 case Surface.ROTATION_270:
                     CDELog.j(TAG, "ROTATION_270");
                     phoneDegree = 270;
                     break;
                 default:
                     break;

             }
             Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
             if (false) {
                 Camera.getCameraInfo(Camera.CameraInfo.CAMERA_FACING_FRONT, cameraInfo);
                 result = (cameraInfo.orientation + phoneDegree) % 360;
                 result = (360 - result) % 360;
             }
             Camera.getCameraInfo(Camera.CameraInfo.CAMERA_FACING_BACK, cameraInfo);
             result = (cameraInfo.orientation - phoneDegree + 360) % 360;
             CDELog.j(TAG, "result : " + result);
         } catch (Exception e) {
             e.printStackTrace();
             CDELog.d(TAG, "init failed:" + e.getMessage());
             Toast.makeText(mContext, "init failed:" + e.getMessage(), Toast.LENGTH_SHORT).show();
         }
         return result;
     }

     @Override
     public void onStart() {
         CDELog.j(TAG, "onStart");
         super.onStart();
         initCamera();
     }

     @Override
     public void onResume() {
         CDELog.j(TAG, "onResume");
         super.onResume();
         if (mCameraInit) {
             ncnnjni.openCamera(facing);
         }
     }

     @Override
     public void onPause() {
         CDELog.j(TAG, "onPause");
         super.onPause();
         if (mCameraInit) {
             ncnnjni.closeCamera();
         }
     }

     @Override
     public void onDestroy() {
         CDELog.j(TAG, "onDestroy");
         super.onDestroy();
         finalizeCamera();
     }

     @Override
     public void onStop() {
         CDELog.j(TAG, "onStop");
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
                 CDELog.j(TAG, "ERROR:" + eventString);
                 _txtLLMInfo.setText("ERROR:" + content);
             }

             if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                 if ((arg1 == KANTV_INFO_ASR_STOP)
                         || (arg1 == KANTV_INFO_ASR_FINALIZE)
                 ) {
                     return;
                 }

                 //CDELog.j(TAG, "content:" + content);
                 if (content.startsWith("unknown")) {

                 } else {
                     {
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
             CDELog.j(TAG, "mgr already initialized");
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

     private void finalizeKANTVMgr() {
         if (mKANTVMgr == null) {
             CDELog.j(TAG, "mgr not initialized");
             return;
         }

         try {
             CDELog.j(TAG, "release mgr");
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


     public void release() {
         finalizeCamera();
         finalizeKANTVMgr();
     }


     public void onMenuItemSelected(MenuItem item) {
         int id = item.getItemId();
         //TODO: various CV manipulations in native layer via OpenCV-Android/OpenGL/FFmpeg
         switch (id) {
             case R.id.cool_filter:

                 break;

             case R.id.beauty_filter:

                 break;

             case R.id.nostalgia_filter:

                 break;

             case R.id.romance_filter:

                 break;

             case R.id.original_filter:
             default:

                 break;
         }

         if (id != R.id.original_filter)
             mActivity.setTitle("AI Agent Demo (filter:" + item.getTitle() + ")");
         else
             mActivity.setTitle("AI Agent Demo");
     }
 }