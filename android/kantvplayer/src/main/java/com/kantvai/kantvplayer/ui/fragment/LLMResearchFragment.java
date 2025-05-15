 /*
  * Copyright (c) 2024- KanTV Authors
  */
 package com.kantvai.kantvplayer.ui.fragment;

 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_FINALIZE;
 import static kantvai.media.player.KANTVEvent.KANTV_INFO_ASR_STOP;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.content.pm.ActivityInfo;
 import android.content.res.Resources;
 import android.graphics.PixelFormat;
 import android.hardware.Camera;
 import android.view.MenuItem;
 import android.view.Surface;
 import android.view.SurfaceHolder;
 import android.view.SurfaceView;
 import android.view.WindowManager;
 import android.widget.Button;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
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

 import kantvai.ai.KANTVAIModelMgr;
 import kantvai.ai.KANTVAIUtils;
 import kantvai.ai.ggmljava;
 import kantvai.media.player.KANTVEvent;
 import kantvai.media.player.KANTVEventListener;
 import kantvai.media.player.KANTVEventType;
 import kantvai.media.player.KANTVException;
 import kantvai.media.player.KANTVLibraryLoader;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVMgr;
 import kantvai.media.player.KANTVUtils;

 public class LLMResearchFragment extends BaseMvpFragment<LLMResearchPresenter> implements LLMResearchView, SurfaceHolder.Callback {
     private static final String TAG = LLMResearchFragment.class.getName();

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     TextView txtLLMInfo;
     TextView txtGGMLInfo;

     private KANTVMgr mKANTVMgr = null;
     private MyEventListener mEventListener = new MyEventListener();
     private KANTVAIModelMgr LLMModelMgr = KANTVAIModelMgr.getInstance();

     private boolean mCameraInit = false;

     private int facing = 1; //default is front camera

     private SurfaceView cameraView;

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);

     private String LLMModelFullName;
     private String LLMModelURL;
     String selectModelFilePath = "";
     private String strUserInput = "what do you see in this image?";

     private void initLLMModels() {
         LLMModelFullName = LLMModelMgr.getKANTVAIModelFromName("SmolVLM-500M").getName();
         LLMModelURL = LLMModelMgr.getKANTVAIModelFromName("SmolVLM-500M").getUrl();
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
         txtGGMLInfo = mActivity.findViewById(R.id.agentDeviceInfo);

         mActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
         mActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
         mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR);
         mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

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

         try {
             initKANTVMgr();
         } catch (Exception e) {
             KANTVLog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         cameraView = mActivity.findViewById(R.id.cameraview);
         cameraView.getHolder().setFormat(PixelFormat.RGBA_8888);
         cameraView.getHolder().addCallback(this);

         Button buttonSwitchCamera = mActivity.findViewById(R.id.buttonSwitchCamera);
         buttonSwitchCamera.setOnClickListener(arg0 -> {
            reload();
         });

         reload();

         checkLLMModelExist();
         endTime = System.currentTimeMillis();
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }


     @Override
     public void initListener() {

     }

     private void reload() {
         int new_facing = 1 - facing;
         ggmljava.closeCamera();
         ggmljava.openCamera(new_facing);
         facing = new_facing;
     }

     public void reload(int front_camera) {
         int new_facing = 1 - front_camera;
         ggmljava.closeCamera();
         ggmljava.openCamera(new_facing);
         facing = new_facing;
     }

     @Override
     public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
         ggmljava.setOutputWindow(holder.getSurface());
     }

     @Override
     public void surfaceCreated(SurfaceHolder holder) {
     }

     @Override
     public void surfaceDestroyed(SurfaceHolder holder) {
     }

     public void initCamera() {
         if (mCameraInit) {
             KANTVLog.j(TAG, "camera already initialized");
             return;
         }
         getPreviewRotateDegree();

         KANTVLog.j(TAG, "running service: " + KANTVUtils.getRunningServicesInfo(mContext));
         try {
             ggmljava.openCamera(facing);
             mCameraInit = true;
         } catch (Exception e) {
             e.printStackTrace();
             KANTVLog.j(TAG, "init failed:" + e.getMessage());
             Toast.makeText(mContext, "init failed:" + e.getMessage(), Toast.LENGTH_SHORT).show();
         }

     }

     public void finalizeCamera() {
         if (mCameraInit) {
             ggmljava.closeCamera();
             mCameraInit = false;
         } else {
             KANTVLog.j(TAG, "camera already finalized");
         }
     }


     private int getPreviewRotateDegree() {
         int result = 0;
         int phoneDegree = 0;
         try {
             int phoneRotate = mActivity.getWindowManager().getDefaultDisplay().getRotation();
             switch (phoneRotate) {
                 case Surface.ROTATION_0:
                     KANTVLog.j(TAG, "ROTATION_0");
                     phoneDegree = 0;
                     break;
                 case Surface.ROTATION_90:
                     KANTVLog.j(TAG, "ROTATION_90");
                     phoneDegree = 90;
                     break;
                 case Surface.ROTATION_180:
                     KANTVLog.j(TAG, "ROTATION_180");
                     phoneDegree = 180;
                     break;
                 case Surface.ROTATION_270:
                     KANTVLog.j(TAG, "ROTATION_270");
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
             KANTVLog.j(TAG, "result : " + result);
         } catch (Exception e) {
             e.printStackTrace();
             KANTVLog.d(TAG, "init failed:" + e.getMessage());
             Toast.makeText(mContext, "init failed:" + e.getMessage(), Toast.LENGTH_SHORT).show();
         }
         return result;
     }

     @Override
     public void onStart() {
         KANTVLog.j(TAG, "onStart");
         super.onStart();
         initCamera();
     }

     @Override
     public void onResume() {
         KANTVLog.j(TAG, "onResume");
         super.onResume();
         if (mCameraInit) {
             ggmljava.openCamera(facing);
         }
     }

     @Override
     public void onPause() {
         KANTVLog.j(TAG, "onPause");
         super.onPause();
         if (mCameraInit) {
             ggmljava.closeCamera();
         }
     }

     @Override
     public void onDestroy() {
         KANTVLog.j(TAG, "onDestroy");
         super.onDestroy();
         finalizeCamera();
     }

     @Override
     public void onStop() {
         KANTVLog.j(TAG, "onStop");
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
                 if ((arg1 == KANTV_INFO_ASR_STOP)
                         || (arg1 == KANTV_INFO_ASR_FINALIZE)
                 ) {
                     return;
                 }

                 //KANTVLog.j(TAG, "content:" + content);
                 if (content.startsWith("unknown")) {

                 } else {
                     {
                         txtLLMInfo.append(content);

                         int offset = txtLLMInfo.getLineCount() * txtLLMInfo.getLineHeight();
                         if (offset > txtLLMInfo.getHeight())
                             txtLLMInfo.scrollTo(0, offset - txtLLMInfo.getHeight());
                     }
                 }
             }
         }
     }


     private void initKANTVMgr() {
         if (mKANTVMgr != null) {
             KANTVLog.j(TAG, "mgr already initialized");
             return;
         }

         try {
             mKANTVMgr = new KANTVMgr(mEventListener);
             if (mKANTVMgr != null) {
                 mKANTVMgr.initASR();
                 mKANTVMgr.startASR();
             }
         } catch (KANTVException ex) {
             String errorMsg = "An exception was thrown because:\n" + " " + ex.getMessage();
             KANTVLog.j(TAG, "error occurred: " + errorMsg);
             KANTVUtils.showMsgBox(mActivity, errorMsg);
             ex.printStackTrace();
         }
     }

     private void finalizeKANTVMgr() {
         if (mKANTVMgr == null) {
             KANTVLog.j(TAG, "mgr not initialized");
             return;
         }

         try {
             KANTVLog.j(TAG, "release mgr");
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


     public void release() {
         finalizeCamera();
         finalizeKANTVMgr();
     }

     public void stopLLMInference() {
         if (ggmljava.inference_is_running()) {
             ggmljava.inference_stop_inference();
         }
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
         txtGGMLInfo.append(KANTVAIUtils.getDeviceInfo(mActivity, KANTVAIUtils.INFERENCE_LLM));
         txtGGMLInfo.append("\n" + "LLM model:" + LLMModelFullName);
         String timestamp = "";
         SimpleDateFormat fullDateFormat = new SimpleDateFormat("yyyy-MM-dd,HH:mm:ss");
         Date date = new Date(System.currentTimeMillis());
         timestamp = fullDateFormat.format(date);
         txtGGMLInfo.append("\n");
         //txtGGMLInfo.append(" running timestamp:" + timestamp);
     }

     public static native int kantv_anti_remove_rename_this_file();
 }