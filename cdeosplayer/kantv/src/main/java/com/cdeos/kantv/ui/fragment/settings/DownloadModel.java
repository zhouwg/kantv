 /*
  * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
  *
  * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
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
package com.cdeos.kantv.ui.fragment.settings;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

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
import com.cdeos.kantv.R;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;

 //TODO: merge download code between SoftwareUpgrade and DownloadModel
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
    private static final int DOWN_UPDATE = 1;
    private static final int DOWN_OVER_FILE = 2;
    private static final int DOWN_OVER_VERSION = 3;

    private String title = null;
    private String modelName = null;
    private String savePath = null;
    private String modelFileNameA;
    private String modelFileNameB;
    private String localModleFileA;
    private String localModleFileB;
    private String remoteModleFileA;
    private String remoteModleFileB;

    public DownloadModel(Context context) {
        mContext = context;
        CDELog.j(TAG, "enter update manager");
        savePath = CDEUtils.getDataPath(mContext);

    }

    public void setTitle(String titleName) {
        title = titleName;
    }
    public void setModeName(String name) {
        modelName = name;
    }

    public void setModeName(String engineName, String modelA, String modelB) {
        modelFileNameA = modelA;
        modelFileNameB = modelB;

        CDELog.j(TAG, "engineName: " + engineName + "model file:" + modelA);
        CDELog.j(TAG, "engineName: " + engineName + "model file:" + modelB);
        localModleFileA = CDEUtils.getDataPath(mContext) + modelFileNameA;
        localModleFileB = CDEUtils.getDataPath(mContext) + modelFileNameB;

        remoteModleFileA = "http://" + CDEUtils.getKANTVMasterServer() + "/kantv/" + engineName + "/" + modelFileNameA;
        remoteModleFileB = "http://" + CDEUtils.getKANTVMasterServer() + "/kantv/" + engineName + "/" + modelFileNameB;
    }


    public void showUpdateDialog() {
        if ((modelFileNameB == null) || (modelFileNameB == null)) {
            CDELog.j(TAG, "please calling setModeName firstly");
            return;
        }
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(title);
        builder.setMessage("");
        builder.setCancelable(true);
        builder.setPositiveButton("下载", new DialogInterface.OnClickListener() {
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
        builder.setTitle("下载" + modelName + "模型");
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View v = inflater.inflate(R.layout.progress_bar, null);
        mProgress = (ProgressBar) v.findViewById(R.id.progress);
        mTextSpeed = (TextView) v.findViewById(R.id.txtSpeed);
        builder.setView(v);
        builder.setNegativeButton("取消", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                intercept = true;
                File apkFile = new File(localModleFileA);
                if (apkFile.exists()) {
                    CDELog.j(TAG, "download partial,remove it");
                    apkFile.delete();
                }

                apkFile = new File(localModleFileB);
                if (apkFile.exists()) {
                    CDELog.j(TAG, "download partial,remove it");
                    apkFile.delete();
                }
            }
        });
        builder.setCancelable(true);
        downloadDialog = builder.create();
        downloadDialog.show();

        CDELog.j(TAG, "begin download file");
        downloadModel();
    }

    private void downloadModel() {
        modelDownLoadThread = new Thread(modelDownloadRunnable);
        modelDownLoadThread.start();
    }


    private int doDownloadFile(String remoteFileUrl, String localFileUrl, boolean showUIProgress) {
        URL url;
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
                numread = 0;
                numread = ins.read(buf);
                count += numread;
                downloadBytes += numread;
                progress = (int) (((float) count / length) * 100);

                if (showUIProgress) {
                    mHandler.sendEmptyMessage(DOWN_UPDATE);
                    endTime = System.currentTimeMillis();
                    float speed = ((downloadBytes * 1.0f) / ((endTime - beginTime) / 1000.0f) / 1024);
                    String speedString = mContext.getString(R.string.download_speed) + ":" + String.format("%.2f", speed) + " KBytes/s";
                    mTextSpeed.setText(speedString);
                }

                if (numread <= 0) {
                    CDELog.j(TAG, "download file succeed:remote url(" + remoteFileUrl + "),local file(" + localFileUrl + ")");
                    //mHandler.sendEmptyMessage(DOWN_OVER);
                    break;
                }
                fos.write(buf, 0, numread);
            }
            fos.close();
            ins.close();
            endTime = System.currentTimeMillis();
            CDELog.j(TAG, "download file succeed: remote url(" + remoteFileUrl + "),local file(" + localFileUrl + ")");
            CDELog.j(TAG, "download bytes:" + downloadBytes);
            CDELog.j(TAG, "download cost: " + (endTime - beginTime) + " milliseconds");
            float speed = ((downloadBytes * 1.0f) / ((endTime - beginTime) / 1000.0f) / 1024);
            CDELog.j(TAG, "download speed:" + String.format("%.2f", speed) + "KBytes/s");
            //mHandler.sendEmptyMessage(DOWN_OVER);
            return 0;
        } catch (Exception e) {
            e.printStackTrace();
            CDELog.j(TAG, "download file failed,reason:" + e.toString());
            return 1;
        }
    }


    private Runnable modelDownloadRunnable = new Runnable() {
        @Override
        public void run() {
            int result = 0;
            result = doDownloadFile(remoteModleFileA, localModleFileA, true);
            result = doDownloadFile(remoteModleFileB, localModleFileB, true);
            downloadDialog.dismiss();
            if (0 == result) {
                CDELog.j(TAG, "download apk file succeed");
                mHandler.sendEmptyMessage(DOWN_OVER_FILE);
            } else {
                CDELog.j(TAG, "download apk failed");
            }

        }
    };


    private void onFinishDownload() {
        try {
            File fileA = new File(localModleFileA);
            File fileB = new File(localModleFileB);
            if (
                    (fileA == null) || (!fileA.exists())
                            || (fileB == null) || (!fileB.exists())
            ) {
                Toast.makeText(mContext, modelName + "模型文件下载失败", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(mContext, modelName + "模型文件下载成功", Toast.LENGTH_SHORT).show();
            }


        } catch (Exception e) {
            e.printStackTrace();
            CDELog.j(TAG, "download failed");
        }
    }

    private void showInstallResultDialog(String info) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mContext.getString(R.string.software_update));
        builder.setMessage(info);
        builder.setCancelable(true);
        builder.setNegativeButton(R.string.OK,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
        builder.show();
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

                default:
                    break;
            }
        }
    };

    public static native int kantv_anti_tamper();
}