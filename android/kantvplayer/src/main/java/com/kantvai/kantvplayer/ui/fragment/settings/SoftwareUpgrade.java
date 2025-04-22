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
package com.kantvai.kantvplayer.ui.fragment.settings;


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

import androidx.core.content.FileProvider;

import com.kantvai.kantvplayer.R;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.KANTVDRM;

//TODO: merge download code between SoftwareUpgrade and DownloadModel
public class SoftwareUpgrade {
    private Context mContext;
    private boolean isNew = false;
    private boolean intercept = false;
    private final static String TAG = SoftwareUpgrade.class.getName();
    private KANTVDRM mKANTVDRM = KANTVDRM.getInstance();

    private String savePath = null;
    private String saveAPKFileName = null;
    private String saveVersionFileName = null;
    private String remoteVersionString = "1.0.0";

    private Thread apkDownLoadThread;
    private int progress;

    private Thread versionDownLoadThread;
    TextView text;
    private ProgressBar mProgress = null;
    private AlertDialog updateDialog = null;
    private AlertDialog downloadDialog = null;
    private TextView    mTextSpeed = null;
    private static final int DOWN_UPDATE        = 1;
    private static final int DOWN_OVER_APK      = 2;
    private static final int DOWN_OVER_VERSION  = 3;
    public SoftwareUpgrade(Context context) {
        mContext = context;
        KANTVLog.j(TAG,"enter SoftwareUpgrade");
        savePath = KANTVUtils.getDataPath();
        saveAPKFileName = KANTVUtils.getDataPath() + "kantv-latest.apk";
        saveVersionFileName = KANTVUtils.getDataPath(mContext) + "kantv-latest-version.txt";
    }

    public void checkUpdateInfo() {
        KANTVLog.j(TAG, "check update info");
        try {
            int downloadResult = 0;
            long beginDownloadTime = 0;
            long endDownloadTime = 0;
            beginDownloadTime = System.currentTimeMillis();
            KANTVLog.j(TAG,"version url:" + KANTVUtils.getKANTVUpdateVersionUrl());
            downloadResult = mKANTVDRM.ANDROID_JNI_DownloadFile(KANTVUtils.getKANTVUpdateVersionUrl(), saveVersionFileName);
            endDownloadTime = System.currentTimeMillis();
            KANTVLog.j(TAG, "download version info cost: " + (endDownloadTime - beginDownloadTime) + " milliseconds");
            if (0 == downloadResult) {
                KANTVLog.j(TAG, "download version file succeed");
                //mHandler.sendEmptyMessage(DOWN_OVER_VERSION);
                checkVersion();
            } else {
                KANTVLog.j(TAG, "download version failed");
                String installResult = mContext.getString(R.string.cannot_fetch_version);
                showInstallResultDialog(installResult);
            }
        } catch (Exception ex) {
            KANTVLog.j(TAG, "download version failed: " + ex.toString());
        }
    }

    private void checkVersion() {
        int result = 0;
        int localVersionValue = 0;
        int remoteVersionValue = 0;
        if (0 == result) {
            try {
                File file = new File(saveVersionFileName);
                FileInputStream fis = new FileInputStream(file);
                InputStreamReader isr = new InputStreamReader(fis, StandardCharsets.UTF_8);
                BufferedReader br = new BufferedReader(isr);

                String line;
                while((line = br.readLine()) != null){
                      //process the line
                      KANTVLog.j(TAG, "line:" + line);
                      remoteVersionString= line;
                }
                br.close();
            } catch (Exception e) {
                e.printStackTrace();
                KANTVLog.j(TAG,"read file failed,reason:" + e.toString());
            }
        } else {
            KANTVLog.j(TAG, "download version file failed");
            Toast.makeText(mContext, "download version file failed", Toast.LENGTH_LONG).show();
            return;
        }
        String localVersionString = KANTVUtils.getKANTVAPKVersion();
        KANTVLog.j(TAG, "remote version string:" + remoteVersionString);
        KANTVLog.j(TAG, "local version string:" + localVersionString);
        try {
            String[] localList = localVersionString.split("\\.");
            String[] remotelList = remoteVersionString.split("\\.");
            KANTVLog.j(TAG, "local  version:" + localList[0] + ":" + localList[1] + ":" + localList[2]);
            KANTVLog.j(TAG, "remote version:" + remotelList[0] + ":" + remotelList[1] + ":" + remotelList[2]);
            if (Integer.valueOf(remotelList[1]).intValue() < Integer.valueOf(localList[1]).intValue()) {
                localVersionValue = Integer.valueOf(localList[0]).intValue() * 100 + Integer.valueOf(localList[1]).intValue() * 10;
                remoteVersionValue = Integer.valueOf(remotelList[0]).intValue() * 100 + Integer.valueOf(remotelList[1]).intValue() * 10;
            } else {
                localVersionValue = Integer.valueOf(localList[0]).intValue() * 100 + Integer.valueOf(localList[1]).intValue() * 10 + Integer.valueOf(localList[2]).intValue();
                remoteVersionValue = Integer.valueOf(remotelList[0]).intValue() * 100 + Integer.valueOf(remotelList[1]).intValue() * 10 + Integer.valueOf(remotelList[2]).intValue();
            }
            KANTVLog.j(TAG, "local version value:" + localVersionValue);
            KANTVLog.j(TAG,"remote version value:" + remoteVersionValue);
        } catch (Exception e) {
            e.printStackTrace();
            KANTVLog.j(TAG,"parse version failed,reason:" + e.toString());
            //return;
            localVersionValue = 1;
            remoteVersionValue = 0;
        }


        if (localVersionValue == remoteVersionValue) {
            KANTVLog.j(TAG, "remote version is equal to local version");
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setTitle(mContext.getString(R.string.software_update));
            builder.setMessage(mContext.getString(R.string.no_new_version));
            builder.setCancelable(true);
            builder.setNegativeButton(mContext.getString(R.string.OK),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
            updateDialog = builder.create();
            updateDialog.show();
            return;
        } else if (localVersionValue > remoteVersionValue) {
            KANTVLog.j(TAG, "remote version is below to local version, it shouldn't happen, pls check server");
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setTitle(mContext.getString(R.string.software_update));
            builder.setMessage(mContext.getString(R.string.server_exception));
            builder.setCancelable(true);
            builder.setNegativeButton(mContext.getString(R.string.OK),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
            updateDialog = builder.create();
            updateDialog.show();
            return;
        } else {
            showUpdateDialog();
        }
    }


    private void showUpdateDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(mContext.getString(R.string.software_update));
        builder.setMessage(mContext.getString(R.string.found_new_version));
        builder.setCancelable(true);
        builder.setPositiveButton(mContext.getString(R.string.Update), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                //updateDialog.hide();
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
        builder.setTitle("software upgrade");
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View v = inflater.inflate(R.layout.progress_bar, null);
        mProgress = (ProgressBar) v.findViewById(R.id.progress);
        mTextSpeed = (TextView) v.findViewById(R.id.txtSpeed);
        builder.setView(v);
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                intercept = true;
            }
        });
        builder.setCancelable(true);
        downloadDialog = builder.create();
        downloadDialog.show();

        KANTVLog.j(TAG, "begin download apk");
        downloadAPK();

    }

    private void downloadAPK() {
        //android.os.NetworkOnMainThreadException
        apkDownLoadThread = new Thread(apkDownloadRunnable);
        apkDownLoadThread.start();
    }

    private void downloadVersion() {
        versionDownLoadThread = new Thread(versionDownloadRunnable);
        versionDownLoadThread.start();
    }

    private int doDownloadFile(String remoteFile, String localFile, boolean showUIProgress) {
        URL url;
        try {
            url = new URL(remoteFile);
            HttpURLConnection conn = (HttpURLConnection) url
                    .openConnection();
            conn.connect();
            int length = conn.getContentLength();
            InputStream ins = conn.getInputStream();
            File file = new File(savePath);
            if (!file.exists()) {
                file.mkdir();
            }
            File apkFile = new File(localFile);
            FileOutputStream fos = new FileOutputStream(apkFile);
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
                downloadBytes  += numread;
                progress = (int) (((float) count / length) * 100);

                if (showUIProgress) {
                    mHandler.sendEmptyMessage(DOWN_UPDATE);
                    endTime = System.currentTimeMillis();
                    float speed =  ((downloadBytes * 1.0f)/ ((endTime - beginTime) / 1000.0f) / 1024);
                    String speedString = mContext.getString(R.string.download_speed) + ":" + String.format("%.2f", speed) + " KBytes/s";
                    mTextSpeed.setText(speedString);
                }

                if (numread <= 0) {
                    KANTVLog.j(TAG, "download apk succeed:apk url(" + KANTVUtils.getKANTVUpdateAPKUrl() +"),local file(" + saveAPKFileName + ")");
                    //mHandler.sendEmptyMessage(DOWN_OVER);
                    break;
                }
                fos.write(buf, 0, numread);
            }
            fos.close();
            ins.close();
            endTime = System.currentTimeMillis();
            KANTVLog.j(TAG, "download succeed:remote url(" + KANTVUtils.getKANTVUpdateAPKUrl()+"),local file(" + saveAPKFileName + ")");
            KANTVLog.j(TAG, "download bytes:" + downloadBytes);
            KANTVLog.j(TAG, "download cost: " + (endTime - beginTime) + " milliseconds");
            float speed =  ((downloadBytes * 1.0f)/ ((endTime - beginTime) / 1000.0f) / 1024);
            KANTVLog.j(TAG, "download speed:" + String.format("%.2f", speed) + "KBytes/s");
            //mHandler.sendEmptyMessage(DOWN_OVER);
            return 0;
        } catch (Exception e) {
            e.printStackTrace();
            KANTVLog.j(TAG,"download file failed,reason:" + e.toString());
            return 1;
        }
    }


    private Runnable apkDownloadRunnable = new Runnable() {
        @Override
        public void run() {
            int result = 0;
            result = doDownloadFile(KANTVUtils.getKANTVUpdateAPKUrl(), saveAPKFileName, true);
            //downloadDialog.hide();
            downloadDialog.dismiss();
            if (0 == result) {
                KANTVLog.j(TAG, "download apk file succeed");
                mHandler.sendEmptyMessage(DOWN_OVER_APK);
            } else {
                KANTVLog.j(TAG, "download apk failed");
            }

        }
    };


    private Runnable versionDownloadRunnable = new Runnable() {
        @Override
        public void run() {
            int result = 0;
            result = doDownloadFile(KANTVUtils.getKANTVUpdateVersionUrl(), saveVersionFileName, false);
            if (0 == result) {
                KANTVLog.j(TAG, "download version file succeed");
                mHandler.sendEmptyMessage(DOWN_OVER_VERSION);
            } else {
                KANTVLog.j(TAG, "download version failed");
            }
        }
    };


    private void installAPK() {
        try {
        File apkFile = new File(saveAPKFileName);
        if (!apkFile.exists()) {
            KANTVLog.j(TAG, "error");
            return;
        }
        KANTVLog.j(TAG, "install apk:" + saveAPKFileName);
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            Uri uri = FileProvider.getUriForFile(mContext, mContext.getPackageName() + ".fileprovider", apkFile);
            intent.setDataAndType(uri, "application/vnd.android.package-archive");
        } else {
            intent.setDataAndType(Uri.fromFile(apkFile), "application/vnd.android.package-archive");
        }
        mContext.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
            KANTVLog.j(TAG,"install apk failed,reason:" + e.toString());
            Toast.makeText(mContext, "install apk failed,reason:" + e.toString(), Toast.LENGTH_LONG).show();
            String installResult = "install apk failed,reason:" + e.toString();
            showInstallResultDialog(installResult);
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
                case DOWN_OVER_APK:
                    mProgress.setVisibility(View.GONE);
                    installAPK();
                    break;
                case DOWN_OVER_VERSION:
                    checkVersion();
                    break;
                default:
                    break;
            }
        }
    };
}