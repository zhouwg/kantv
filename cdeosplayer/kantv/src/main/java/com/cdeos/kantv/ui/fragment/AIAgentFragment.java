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

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.app.AlertDialog;
 import android.app.ProgressDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.pm.ActivityInfo;
 import android.content.res.Configuration;
 import android.content.res.Resources;
 import android.hardware.Camera;
 import android.media.MediaPlayer;
 import android.os.Build;
 import android.text.method.ScrollingMovementMethod;
 import android.view.MenuItem;
 import android.view.Surface;
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

 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.mvp.impl.AIAgentPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.AIAgentPresenter;
 import com.cdeos.kantv.mvp.view.AIAgentView;
 import com.cdeos.kantv.utils.Settings;


 import org.ggml.ggmljava;

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
 import cdeos.media.encoder.RtmpHandler;
 import cdeos.media.player.CDEAssetLoader;
 import cdeos.media.player.CDELibraryLoader;
 import cdeos.media.player.CDELog;
 import cdeos.media.player.CDEUtils;
 import cdeos.media.player.KANTVEvent;
 import cdeos.media.player.KANTVEventListener;
 import cdeos.media.player.KANTVEventType;
 import cdeos.media.player.KANTVException;
 import cdeos.media.player.KANTVMgr;
 import cdeos.media.encoder.RtmpHandler;
 import cdeos.media.encoder.SrsCameraView;
 import cdeos.media.encoder.SrsEncodeHandler;
 import cdeos.media.encoder.SrsPublisher;
 import cdeos.media.encoder.SrsRecordHandler;
 import cdeos.media.encoder.magicfilter.utils.MagicFilterType;

 public class AIAgentFragment extends BaseMvpFragment<AIAgentPresenter> implements AIAgentView, RtmpHandler.RtmpListener,
         SrsRecordHandler.SrsRecordListener, SrsEncodeHandler.SrsEncodeListener {

     private static final String TAG = AIAgentFragment.class.getName();
     TextView _txtLLMInfo;
     TextView _txtGGMLInfo;
     TextView _txtGGMLStatus;
     EditText _txtUserInput;
     Button _btnInference;

     private int nThreadCounts = 8;
     private int benchmarkIndex = 0;

     private String strBackend = "cpu";
     private int backendIndex = 0; //QNN_CPU

     private long beginTime = 0;
     private long endTime = 0;
     private long duration = 0;
     private String strBenchmarkInfo;
     //private String strUserInput = "how many days in March 2024?";
     private String strUserInput = "introduce the movie Once Upon a Time in America briefly.";

     private AtomicBoolean isBenchmarking = new AtomicBoolean(false);
     private ProgressDialog mProgressDialog;


     private String ggmlModelFileName = "gemma-2b.Q8_0.gguf";    // 2.67 GB

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private KANTVMgr mKANTVMgr = null;
     private AIAgentFragment.MyEventListener mEventListener = new AIAgentFragment.MyEventListener();

     private RtmpHandler.RtmpListener rtmpListener = this;
     private SrsEncodeHandler.SrsEncodeListener srsEncodeListener = this;
     private SrsRecordHandler.SrsRecordListener srsRecordListener = this;


     private Button btnSwitchCamera;
     private Button btnRecord;

     private String rtmpUrl = "rtmp://192.168.0.20/live/livestream";
     private String recPath = CDEUtils.getDataPath() + "/test.mp4";


     private SrsPublisher mPublisher;
     private SrsCameraView mCameraView;

     private int mPreviewWidth = 1280;
     private int mPreviewHeight = 720;
     private int mEncodeWidth = 720;
     private int mEncodeHeight = 480;
     private boolean isPermissionGranted = CDEUtils.getPermissionGranted();
     private boolean mCameraInit = false;

     private final static int LIVE_STATE_IDLE = 0x0;
     private final static int LIVE_STATE_PREVIEW = 0x1;
     private final static int LIVE_STATE_PUBLISH = 0x10;
     private final static int LIVE_STATE_RECORD = 0x100;
     private int mState = LIVE_STATE_IDLE;

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

         try {
             initKANTVMgr();
         } catch (Exception e) {
             CDELog.j(TAG, "failed to initialize asr subsystem");
             return;
         }

         CDELog.j(TAG, "load ggml's LLAMACPP info");
         String systemInfo = ggmljava.llm_get_systeminfo();
         String phoneInfo = "Device info:" + " "
                 + "Brand:" + Build.BRAND + " "
                 + "Hardware:" + Build.HARDWARE + " "
                 + "OS:" + "Android " + android.os.Build.VERSION.RELEASE + " "
                 + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")";
         CDELog.j(TAG, phoneInfo);
         //Toast.makeText(mContext, phoneInfo, Toast.LENGTH_SHORT).show();


         btnSwitchCamera = mActivity.findViewById(R.id.swCam);
         btnRecord = mActivity.findViewById(R.id.record);
         mCameraView = mActivity.findViewById(R.id.glsurfaceview_camera);


         btnSwitchCamera.setOnClickListener(v -> {
             if (!mCameraInit)
                 return;

             mPublisher.switchCameraFace((mPublisher.getCameraId() + 1) % Camera.getNumberOfCameras());
         });

         btnRecord.setOnClickListener(v -> {
             if (!mCameraInit)
                 return;

             Integer tmp = Integer.valueOf(mState);
             CDELog.j(TAG, "state : 0x" + tmp.toHexString(tmp));
             recPath = CDEUtils.getDataPath() + getRandomAlphaDigitString(10) + getRandomAlphaString(10) + ".mp4";
             CDELog.j(TAG, "recPath:" + recPath);
             if (mState == LIVE_STATE_PREVIEW) {
                 CDELog.j(TAG, "state is PREVIEW");
                 mPublisher.switchToHardEncoder();
                 mPublisher.startPublish(rtmpUrl);

                 if (mPublisher.startRecord(recPath)) {
                     btnRecord.setText("stop record");
                     btnSwitchCamera.setEnabled(false);
                     mState |= LIVE_STATE_RECORD;
                     tmp = Integer.valueOf(mState);
                     CDELog.j(TAG, "state : 0x" + tmp.toHexString(tmp));
                 } else {
                     CDELog.j(TAG, "launch record failure\n");
                     Toast.makeText(mContext, "launch record failure", Toast.LENGTH_SHORT).show();
                 }

             } else if ((mState | LIVE_STATE_RECORD) != 0) {
                 mPublisher.stopPublish();
                 btnRecord.setText("record");
                 mPublisher.stopRecord();
                 btnSwitchCamera.setEnabled(true);
                 mState ^= LIVE_STATE_RECORD;
                 tmp = Integer.valueOf(mState);
                 CDELog.j(TAG, "state : 0x" + tmp.toHexString(tmp));
             }
         });


         endTime = System.currentTimeMillis();
         CDELog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }


     @Override
     public void initListener() {

     }


     private void displayFileStatus(String modelFilePath) {
         _txtGGMLStatus.setText("");
         File modelFile = new File(modelFilePath);
         if (!modelFile.exists()) {
             CDELog.j(TAG, "model file not exist:" + modelFile.getAbsolutePath());
         }
     }

     public void initCamera() {
         if (mCameraInit) {
             CDELog.j(TAG, "camera already initialized");
             return;
         }
         getPreviewRotateDegree();

         CDELog.j(TAG, "running service: " + CDEUtils.getRunningServicesInfo(mContext));
         //rtmpUrl = mSettings.getRTMPServerUrl();
         CDELog.j(TAG, "rtmp url: " + rtmpUrl);
         CDELog.j(TAG, "recPath: " + recPath);
         try {
             CDELog.j(TAG, "create srspublisher");
             mPublisher = new SrsPublisher(mCameraView);
             CDELog.j(TAG, "after create srspublisher");
             mPublisher.setEncodeHandler(new SrsEncodeHandler(srsEncodeListener));
             CDELog.j(TAG, "after setEncodeHandler");
             mPublisher.setRtmpHandler(new RtmpHandler(rtmpListener));
             CDELog.j(TAG, "here");
             mPublisher.setRecordHandler(new SrsRecordHandler(srsRecordListener));
             CDELog.j(TAG, "here");
             mPublisher.setPreviewResolution(mPreviewWidth, mPreviewHeight);
             CDELog.j(TAG, "here");
             mPublisher.setOutputResolution(mEncodeWidth, mEncodeHeight);
             CDELog.j(TAG, "here");
             //mPublisher.setVideoHDMode();
             mPublisher.setVideoSmoothMode();
             CDELog.j(TAG, "here");
             mPublisher.setSendVideoOnly(true);
             CDELog.j(TAG, "here");
             mPublisher.switchToHardEncoder();

             mPublisher.setPreviewOrientation(Configuration.ORIENTATION_PORTRAIT);
             CDELog.j(TAG, "startCamera");
             if (isPermissionGranted)
                 mPublisher.startCamera();
             else {
                 CDELog.j(TAG, "camera permission was disabled");
                 Toast.makeText(mContext, "camera permission was disabled", Toast.LENGTH_SHORT).show();
                 return;
             }

             mCameraView.setCameraCallbacksHandler(new SrsCameraView.CameraCallbacksHandler() {
                 @Override
                 public void onCameraParameters(Camera.Parameters params) {
                     //params.setFocusMode("custom-focus");
                     //params.setWhiteBalance("custom-balance");
                 }
             });
             mCameraInit = true;
             mState = LIVE_STATE_PREVIEW;

         } catch (Exception e) {
             e.printStackTrace();
             CDELog.j(TAG, "init failed:" + e.getMessage());
             Toast.makeText(mContext, "init failed:" + e.getMessage(), Toast.LENGTH_SHORT).show();
         }

     }

     public void finalizeCamera() {
         if (mCameraInit) {
             mPublisher.stopPublish();
             mPublisher.stopRecord();
             mPublisher.stopCamera();
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
             mPublisher.resumeRecord();
             mPublisher.startCamera();
         }
     }

     @Override
     public void onPause() {
         CDELog.j(TAG, "onPause");
         super.onPause();
         if (mCameraInit) {
             mPublisher.pauseRecord();
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


     private static String getRandomAlphaString(int length) {
         String base = "abcdefghijklmnopqrstuvwxyz";
         Random random = new Random();
         StringBuilder sb = new StringBuilder();
         for (int i = 0; i < length; i++) {
             int number = random.nextInt(base.length());
             sb.append(base.charAt(number));
         }
         CDELog.j(TAG, "string: " + sb.toString());
         return sb.toString();
     }


     private static String getRandomAlphaDigitString(int length) {
         String base = "abcdefghijklmnopqrstuvwxyz0123456789";
         Random random = new Random();
         StringBuilder sb = new StringBuilder();
         for (int i = 0; i < length; i++) {
             int number = random.nextInt(base.length());
             sb.append(base.charAt(number));
         }
         CDELog.j(TAG, "string: " + sb.toString());
         return sb.toString();
     }


     private void handleException(Exception e) {
         try {
             Toast.makeText(mContext, e.getMessage(), Toast.LENGTH_SHORT).show();
             if (mCameraInit) {
                 mPublisher.stopPublish();
                 mPublisher.stopRecord();
                 btnRecord.setText("record");
             }
         } catch (Exception ex) {
             ex.printStackTrace();
             CDELog.d(TAG, "error:" + ex.getMessage());
         }
     }


     @Override
     public void onRtmpConnecting(String msg) {
         Toast.makeText(mContext, msg, Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRtmpConnected(String msg) {
         Toast.makeText(mContext, msg, Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRtmpVideoStreaming() {
     }

     @Override
     public void onRtmpAudioStreaming() {
     }

     @Override
     public void onRtmpStopped() {
         CDELog.j(TAG, "stopped");
         Toast.makeText(mContext, "Stopped", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRtmpDisconnected() {
         CDELog.j(TAG, "disconnected");
         Toast.makeText(mContext, "Disconnected", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRtmpVideoFpsChanged(double fps) {
         CDELog.d(TAG, String.format("Output Fps: %f", fps));
     }

     @Override
     public void onRtmpVideoBitrateChanged(double bitrate) {
         int rate = (int) bitrate;
         if (rate / 1000 > 0) {
             CDELog.d(TAG, String.format("Video bitrate: %f kbps", bitrate / 1000));
         } else {
             CDELog.j(TAG, String.format("Video bitrate: %d bps", rate));
         }
     }

     @Override
     public void onRtmpAudioBitrateChanged(double bitrate) {
         int rate = (int) bitrate;
         if (rate / 1000 > 0) {
             CDELog.j(TAG, String.format("Audio bitrate: %f kbps", bitrate / 1000));
         } else {
             CDELog.j(TAG, String.format("Audio bitrate: %d bps", rate));
         }
     }

     @Override
     public void onRtmpSocketException(SocketException e) {
         handleException(e);
     }

     @Override
     public void onRtmpIOException(IOException e) {
         handleException(e);
     }

     @Override
     public void onRtmpIllegalArgumentException(IllegalArgumentException e) {
         handleException(e);
     }

     @Override
     public void onRtmpIllegalStateException(IllegalStateException e) {
         handleException(e);
     }

     @Override
     public void onRecordPause() {
         Toast.makeText(mContext, "Record paused", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRecordResume() {
         Toast.makeText(mContext, "Record resumed", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRecordStarted(String msg) {
         Toast.makeText(mContext, "Recording file: " + msg, Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRecordFinished(String msg) {
         Toast.makeText(mContext, "MP4 file saved: " + msg, Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onRecordIOException(IOException e) {
         handleException(e);
     }

     @Override
     public void onRecordIllegalArgumentException(IllegalArgumentException e) {
         handleException(e);
     }

     @Override
     public void onNetworkWeak() {
         Toast.makeText(mContext, "Network weak", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onNetworkResume() {
         Toast.makeText(mContext, "Network resume", Toast.LENGTH_SHORT).show();
     }

     @Override
     public void onEncodeIllegalArgumentException(IllegalArgumentException e) {
         handleException(e);
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
                     if (content.startsWith("llama-timings")) {
                         _txtGGMLStatus.setText("");
                         _txtGGMLStatus.append(content);
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
         switch (id) {
             case R.id.cool_filter:
                 mPublisher.switchCameraFilter(MagicFilterType.COOL);
                 break;

             case R.id.beauty_filter:
                 mPublisher.switchCameraFilter(MagicFilterType.BEAUTY);
                 break;

             case R.id.nostalgia_filter:
                 mPublisher.switchCameraFilter(MagicFilterType.NOSTALGIA);
                 break;

             case R.id.romance_filter:
                 mPublisher.switchCameraFilter(MagicFilterType.ROMANCE);
                 break;

             case R.id.original_filter:
             default:
                 mPublisher.switchCameraFilter(MagicFilterType.NONE);
                 break;
         }

         if (id != R.id.original_filter)
             mActivity.setTitle("AI Agent Demo (filter:" + item.getTitle() + ")");
         else
             mActivity.setTitle("AI Agent Demo");
     }
 }