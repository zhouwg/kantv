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
 package com.kantvai.kantvplayer.ui.fragment.settings;

 import static com.kantvai.kantvplayer.R.*;

 import java.io.BufferedReader;
 import java.io.File;
 import java.io.FileInputStream;
 import java.io.FileOutputStream;
 import java.io.InputStream;
 import java.io.InputStreamReader;
 import java.net.HttpURLConnection;
 import java.net.URL;
 import java.nio.charset.StandardCharsets;
 import java.util.Locale;

 import android.app.AlertDialog;
 import android.content.Context;
 import android.content.DialogInterface;
 import android.content.Intent;
 import android.net.Uri;
 import android.os.Build;
 import android.os.Environment;
 import android.os.Handler;
 import android.view.LayoutInflater;
 import android.view.View;
 import android.view.WindowManager;
 import android.widget.ProgressBar;
 import android.widget.TextView;
 import android.widget.Toast;

 import com.kantvai.kantvplayer.R;

 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;

 //TODO: merge codes between SoftwareUpgrade and DownloadModel
 public class DownloadModel {
     private Context mContext;
     private boolean intercept = false;
     private final static String TAG = DownloadModel.class.getName();

     private Thread modelDownLoadThread;
     private int progress;

     TextView text;
     private ProgressBar mProgress = null;
     private AlertDialog updateDialog = null;
     private AlertDialog downloadDialog = null;
     private TextView mTextSpeed = null;
     private TextView mTextInfo  = null;

     private boolean mIsDownloadLLMModel = false;
     private static final int DOWN_UPDATE = 1;
     private static final int DOWN_OVER_FILE = 2;
     private static final int DOWN_OVER_VERSION = 3;
     private static final int DOWN_FILE_FAILED = 4;

     private String title = null;
     private String savePath;
     private String modelFileNameA;  //name of the main model for a specified LLM model
     private String modelFileNameB;  //name of the mmproj model for a specified LLM model
     private String localModleFileA = null;
     private String localModleFileB = null;
     private String remoteModleFileA;
     private String remoteModleFileB;

     private long sizeOfLocalModelFileA = 0; //expected size of the main model for a specified LLM model
     private long sizeOfLocalModelFileB = 0; //expected size of the mmproj model for a specified LLM model
     private String errorString = "";

     public DownloadModel(Context context) {
         mContext = context;
         KANTVLog.j(TAG, "enter update manager");
         savePath = KANTVUtils.getDataPath(mContext);

     }

     public void setTitle(String titleName) {
         title = titleName;
     }

     public void setModelName(String name) {
     }

     public void setModelName(String engineName, String modelA, String modelB) {
         modelFileNameA = modelA;
         modelFileNameB = modelB;

         KANTVLog.j(TAG, "engineName: " + engineName + " file:" + modelA);
         KANTVLog.j(TAG, "modelName: " + engineName + "  file:" + modelB);

         //  /sdcardk/kantv/jfk.wav
         //  /sdcard/kantv/ggml-xxx.bin
         localModleFileA = KANTVUtils.getDataPath() + modelFileNameA;
         localModleFileB = KANTVUtils.getDataPath() + modelFileNameB;

         // http://www.kantvai.com/kantv/ggml/jfk.wav, available
         remoteModleFileA = "http://" + KANTVUtils.getKANTVMasterServer() + "/kantv/" + engineName + "/" + modelFileNameA;
         // replace with upstream url since 03-22-2024
         // the GGML whispercpp model could be found at https://huggingface.co/ggerganov/whisper.cpp/tree/main
         remoteModleFileB = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/" + modelFileNameB;

         KANTVLog.j(TAG, "remote model url:" + remoteModleFileB);
     }

     public void setLLMModelName(String engineName, String modelName, String mmprojModelName, String modelURL, String mmprojModelURL) {
         modelFileNameA = modelName;
         modelFileNameB = mmprojModelName;

         KANTVLog.j(TAG, "engineName: " + engineName + " modelName:" + modelName);
         KANTVLog.j(TAG, "modelName: " + engineName + "  mmprojModelName:" + mmprojModelName);

         //  /sdcardk/xxx.gguf
         //  /sdcard/mmproj-xxx.gguf.bin
         localModleFileA = KANTVUtils.getSDCardDataPath() + modelFileNameA;
         remoteModleFileA = modelURL;

         if (mmprojModelName != null) {
             localModleFileB = KANTVUtils.getSDCardDataPath() + modelFileNameB;
             remoteModleFileB = mmprojModelURL;
         }

         mIsDownloadLLMModel = true;

         KANTVLog.j(TAG, "remote model url:" + remoteModleFileA);
     }

     public void setLLMModelSize(long modelSize, long mmprojModelSize) {
         sizeOfLocalModelFileA = modelSize;
         sizeOfLocalModelFileB = mmprojModelSize;
     }

     public void showUpdateDialog() {
         if ((modelFileNameA == null) && (modelFileNameB == null)) {
             KANTVLog.g(TAG, "please calling setModeName firstly");
             return;
         }
         AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
         builder.setTitle(title);
         builder.setMessage("");
         builder.setCancelable(true);
         builder.setPositiveButton("Download", new DialogInterface.OnClickListener() {
             @Override
             public void onClick(DialogInterface dialog, int which) {
                 updateDialog.dismiss();
                 showDownloadDialog();
             }
         });
         builder.setNegativeButton(R.string.Cancel,
                 new DialogInterface.OnClickListener() {
                     @Override
                     public void onClick(DialogInterface dialog, int which) {
                         dialog.dismiss();
                     }
                 });
         updateDialog = builder.create();
         updateDialog.show();
     }


     private void showDownloadDialog() {
         AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
         if (mIsDownloadLLMModel)
             builder.setTitle("Downloading LLM model");
         else
            builder.setTitle("Downloading ASR model");
         LayoutInflater inflater = LayoutInflater.from(mContext);
         View v = inflater.inflate(R.layout.progress_bar, null);
         mProgress = v.findViewById(R.id.progress);
         mTextSpeed = v.findViewById(R.id.txtSpeed);
         mTextInfo  = v.findViewById(R.id.txtDownloadInfo);
         builder.setView(v);
         builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
             @Override
             public void onClick(DialogInterface dialog, int which) {
                 intercept = true;
                 checkDownloadedModels();
             }
         });
         builder.setCancelable(true);
         downloadDialog = builder.create();
         downloadDialog.show();

         KANTVLog.j(TAG, "begin download file");
         downloadModel();
     }

     private void downloadModel() {
         modelDownLoadThread = new Thread(modelDownloadRunnable);
         modelDownLoadThread.start();
     }


     private int doDownloadFile(String remoteFileUrl, String localFileUrl, long expectedSize, boolean showUIProgress) {
         URL url;
         mTextInfo.setText("Downloading from " + remoteFileUrl + " to " + localFileUrl + ", expectedSize:" + expectedSize + " bytes");
         try {
             url = new URL(remoteFileUrl);
             HttpURLConnection conn = (HttpURLConnection) url.openConnection();
             conn.connect();
             int length = conn.getContentLength();
             InputStream ins = conn.getInputStream();
             File file = new File(savePath);
             if (!file.exists()) {
                 file.mkdir();
             }
             File localFile = new File(localFileUrl);
             FileOutputStream fos = new FileOutputStream(localFile);
             int count = 0;
             int numread = 0;
             byte[] buf = new byte[1024];
             long beginTime = 0;
             long endTime = 0;
             long downloadBytes = 0;
             beginTime = System.currentTimeMillis();

             while (!intercept) {
                 numread = ins.read(buf);
                 count += numread;
                 downloadBytes += numread;
                 progress = (int) (((float) count / length) * 100);

                 if (showUIProgress) {
                     mHandler.sendEmptyMessage(DOWN_UPDATE);
                     endTime = System.currentTimeMillis();
                     float speed = ((downloadBytes * 1.0f) / ((endTime - beginTime) / 1000.0f) / 1024);
                     //String speedString = mContext.getString(R.string.download_speed) + ":" + String.format("%-8.2f", speed) + " KBytes/s";
                     String speedString  = mContext.getString(R.string.download_speed) + ":" +
                             String.format(Locale.US, "%s", KANTVUtils.formatedBitRate(downloadBytes, (endTime - beginTime)));
                     mTextSpeed.setText(speedString);
                 }

                 if (numread <= 0) {
                     KANTVLog.g(TAG, "download file succeed:remote url(" + remoteFileUrl + "),local file(" + localFileUrl + ")");
                     break;
                 }
                 fos.write(buf, 0, numread);
             }
             fos.close();
             ins.close();
             endTime = System.currentTimeMillis();
             KANTVLog.g(TAG, "download file succeed: remote url(" + remoteFileUrl + "),local file(" + localFileUrl + ")");
             KANTVLog.g(TAG, "download bytes:" + downloadBytes);
             KANTVLog.g(TAG, "download cost: " + (endTime - beginTime) + " milliseconds");
             float speed = ((downloadBytes * 1.0f) / ((endTime - beginTime) / 1000.0f) / 1024);
             KANTVLog.g(TAG, "download speed:" + String.format("%-8.2f", speed) + "KBytes/s");
             //FIXME: better approach to check whether the AI model has downloaded successfully
             KANTVLog.g(TAG, "expectedSize :" + expectedSize);
             boolean download_ok      = ((expectedSize - downloadBytes) == 1);
             boolean download_failure = ((expectedSize - downloadBytes) > (200 * 1024 * 1024));
             if (download_ok) {
                 KANTVLog.g(TAG, "download ok");
                 return 0;
             }
             if (download_failure) {
                 KANTVLog.g(TAG, "download bytes " + downloadBytes + " is not equal to the expected size " + expectedSize);
                 //errorString = "download bytes " + downloadBytes + " is not equal to the expected size " + expectedSize;
                 checkDownloadedModels();
                 return 2;
             }
             return 0;
         } catch (Exception e) {
             checkDownloadedModels();
             e.printStackTrace();
             KANTVLog.g(TAG, "download file failed,reason:" + e.toString());
             errorString ="download file failed,reason:" + e.toString();
             return 1;
         }
     }


     private Runnable modelDownloadRunnable = new Runnable() {
         @Override
         public void run() {
             int result = 0;
             result = doDownloadFile(remoteModleFileA, localModleFileA, sizeOfLocalModelFileA, true);
             if (localModleFileB != null)
                result = doDownloadFile(remoteModleFileB, localModleFileB, sizeOfLocalModelFileB, true);
             downloadDialog.dismiss();
             KANTVLog.g(TAG, "result= " + result);
             if (0 == result) {
                 KANTVLog.g(TAG, "download  file succeed:" + remoteModleFileA);
                 mHandler.sendEmptyMessage(DOWN_OVER_FILE);
             } else {
                 KANTVLog.g(TAG, "download file failed:" + remoteModleFileA);
                 mHandler.sendEmptyMessage(DOWN_FILE_FAILED);
             }
         }
     };


     private void onFinishDownload() {
         try {
             int result = checkDownloadedModels();
             KANTVLog.g(TAG,"result " + result);
             KANTVUtils.showMsgBox(mContext, errorString);
         } catch (Exception e) {
             e.printStackTrace();
             KANTVLog.g(TAG, "download failed");
             KANTVUtils.showMsgBox(mContext, "download failed");
         }
     }

     private Handler mHandler = new Handler() {
         public void handleMessage(android.os.Message msg) {
             switch (msg.what) {
                 case DOWN_UPDATE:
                     mProgress.setProgress(progress);
                     break;

                 case DOWN_OVER_FILE:
                     mProgress.setVisibility(View.GONE);
                     onFinishDownload();
                     break;

                 case DOWN_FILE_FAILED:
                     checkDownloadedModels();
                     //Toast.makeText(mContext, errorString, Toast.LENGTH_SHORT).show();
                     KANTVUtils.showMsgBox(mContext, errorString);
                     break;

                 default:
                     break;
             }
         }
     };

     private int checkDownloadedModels() {
         int result = 0;
         try {
             File modelFile = new File(localModleFileA);
             if (modelFile.exists()) {
                 if (mIsDownloadLLMModel) {
                     KANTVLog.g(TAG, "sizeOfLocalModelFileA:" + sizeOfLocalModelFileA);
                     KANTVLog.g(TAG, "modelFile.length():" + modelFile.length());
                     //FIXME: better approach to check whether the AI model has downloaded successfully
                     boolean download_failure = ((sizeOfLocalModelFileA - modelFile.length()) > (200 * 1024 * 1024));
                     if (sizeOfLocalModelFileA == modelFile.length()) {
                         //do nothing
                     }
                     if (download_failure) {
                         KANTVLog.g(TAG, "file: " + localModleFileA + " exist but not complete, remove it");
                         errorString = "file: " + localModleFileA + " exist but file size " + modelFile.length()
                                 + " is not equal to the expected size " + sizeOfLocalModelFileA + ", remove it";
                         modelFile.delete();
                         result = 1;
                     }
                 }
             } else {
                 KANTVLog.g(TAG, "file " + localModleFileA + " not exist");
                 errorString = "file " + localModleFileA + " not exist";
                 result = 1;
             }

             if (modelFileNameB != null) {
                 modelFile = new File(localModleFileB);
                 if (modelFile.exists()) {
                     if (mIsDownloadLLMModel) {
                         KANTVLog.g(TAG, "sizeOfLocalModelFileB:" + sizeOfLocalModelFileB);
                         KANTVLog.g(TAG, "modelFile.length():" + modelFile.length());
                         //FIXME: better approach to check whether the AI model has downloaded successfully
                         boolean download_failure = ((sizeOfLocalModelFileB - modelFile.length()) > (200 * 1024 * 1024));
                         if (sizeOfLocalModelFileB == modelFile.length()) {
                             //do nothing
                         }
                         if (download_failure) {
                             KANTVLog.g(TAG, "file: " + localModleFileB + " exist but not complete, remove it");
                             errorString += " file: " + localModleFileB + " exist but file size " + modelFile.length()
                                     + " is not equal to the expected size " + sizeOfLocalModelFileB + ", remove it";
                             modelFile.delete();
                             result = 2;
                         }
                     }
                 } else {
                     KANTVLog.g(TAG, "file " + localModleFileB + " not exist");
                     errorString += ",file " + localModleFileB + " not exist";
                     result = 2;
                 }
             }
         } catch (Exception e) {
             errorString += "error: " + e.toString();
             result = 3;
         }

         if (0 == result) {
             if (modelFileNameB != null)
                errorString = "succeed to download model file: " + modelFileNameA + " and " + modelFileNameB;
             else
                 errorString = "succeed to download model file: " + modelFileNameA;
         }
         return result;
     }
 }
