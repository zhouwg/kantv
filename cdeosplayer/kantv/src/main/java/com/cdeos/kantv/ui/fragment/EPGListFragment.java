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
 package com.cdeos.kantv.ui.fragment;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.content.Intent;
 import android.content.res.Resources;
 import android.database.sqlite.SQLiteDatabase;
 import android.graphics.drawable.Drawable;
 import android.net.Uri;
 import android.os.Environment;
 import android.text.method.LinkMovementMethod;
 import android.view.View;
 import android.widget.Button;
 import android.widget.LinearLayout;
 import android.widget.TextView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.appcompat.app.AppCompatActivity;

 import com.cdeos.kantv.R;
 import com.cdeos.kantv.base.BaseMvpFragment;
 import com.cdeos.kantv.bean.event.SendMsgEvent;
 import com.cdeos.kantv.mvp.impl.EPGListPresenterImpl;
 import com.cdeos.kantv.mvp.presenter.EPGListPresenter;
 import com.cdeos.kantv.mvp.view.EPGListView;
 import com.cdeos.kantv.player.common.utils.Constants;
 import com.cdeos.kantv.ui.activities.play.PlayerManagerActivity;
 import com.cdeos.kantv.utils.AppConfig;
 import com.cdeos.kantv.utils.Settings;


 import org.greenrobot.eventbus.EventBus;
 import org.greenrobot.eventbus.*;

 import java.io.File;
 import java.io.FileInputStream;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.util.ArrayList;
 import java.util.Arrays;
 import java.util.List;

 import butterknife.BindView;
 import cdeos.media.player.KANTVJNIDecryptBuffer;
 import cdeos.media.player.KANTVDRM;
 import cdeos.media.player.CDEAssetLoader;
 import cdeos.media.player.CDEContentDescriptor;
 import cdeos.media.player.CDELog;
 import cdeos.media.player.CDEMediaType;
 import cdeos.media.player.CDEUrlType;
 import cdeos.media.player.CDEUtils;
 import cdeos.media.player.KANTVDRMManager;


 public class EPGListFragment extends BaseMvpFragment<EPGListPresenter> implements EPGListView {
     private final static String TAG = EPGListFragment.class.getName();

     @BindView(R.id.epg_list_layout)
     LinearLayout layout;

     private CDEMediaType mMediaType = CDEMediaType.MEDIA_TV;
     private List<CDEContentDescriptor> mContentList = new ArrayList<CDEContentDescriptor>();
     private List<CDEContentDescriptor> mIPTVContentList = new ArrayList<CDEContentDescriptor>();
     private boolean mTroubleshootingMode = false;
     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;
     private final static KANTVDRM mKANTVDRM = KANTVDRM.getInstance();

     private boolean mEPGDataLoadded = false;

     public static EPGListFragment newInstance() {
         return new EPGListFragment();
     }

     @NonNull
     @Override
     protected EPGListPresenter initPresenter() {
         return new EPGListPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_epglist;
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

         loadEPGData();
         mEPGDataLoadded = true;
         layoutUI(mActivity);

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
         if (mEPGDataLoadded) {
             CDELog.d(TAG, "epg data already loaded");
         } else {
             CDELog.d(TAG, "epg data not loaded");
             loadEPGData();
             mEPGDataLoadded = true;
             layoutUI(mActivity);
         }
     }

     @Override
     public void onStop() {
         super.onStop();
         mEPGDataLoadded = false;
     }


     private void initPlayback() {
         if (AppConfig.getInstance().getPlayerType() == Constants.PLAYERENGINE__FFmpeg) {
             KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
             mKANTVDRM.ANDROID_JNI_Init(mActivity.getBaseContext(), CDEAssetLoader.getDataPath(mActivity.getBaseContext()));
             mKANTVDRM.ANDROID_JNI_SetLocalEMS(CDEUtils.getLocalEMS());
         }
         if (AppConfig.getInstance().getPlayerType() == Constants.PLAYERENGINE__Exoplayer) {
             if (KANTVDRMManager.openPlaybackService() != 0) {
                 CDELog.e(TAG, "drm init failed");
                 return;
             }
             mKANTVDRM.ANDROID_JNI_SetLocalEMS(CDEUtils.getLocalEMS());
             KANTVDRMManager.setMultiDrmInfo(CDEUtils.getDrmScheme());
         }
     }


     private void loadEPGData() {
         CDELog.j(TAG, "epg updated status:" + CDEUtils.getEPGUpdatedStatus(mMediaType));
         mContentList.clear();

         String remoteEPGUrl = "http://" + CDEUtils.getKANTVServer() + "/kantv/epg/" + "test.m3u";
         String localTestEPGUrl = "test.m3u";
         CDEUtils.copyAssetFile(mContext, localTestEPGUrl, CDEUtils.getDataPath(mContext) + "/test.m3u");
         File fileLocalTestEPGUrl = new File(CDEUtils.getDataPath(mContext) + "/test.m3u");
         CDELog.j(TAG, "remote epg url:" + remoteEPGUrl);
        /*
        mIPTVContentList = CDEUtils.loadIPTVProgram(mContext, remoteEPGUrl);

        if ((mIPTVContentList != null) && (mIPTVContentList.size() > 0)) {
            CDELog.j(TAG, "iptv item size:" + mIPTVContentList.size());
            mContentList = mIPTVContentList;
            CDELog.j(TAG, "program item size:" + mContentList.size());
        } else */
         if (fileLocalTestEPGUrl.exists()) {
             CDELog.j(TAG, "using local epg file:" + CDEUtils.getDataPath(mContext) + "/test.m3u");
             mIPTVContentList = CDEUtils.loadIPTVProgram(mContext, CDEUtils.getDataPath(mContext) + "/test.m3u");
             mContentList = mIPTVContentList;
         } else {
             long beginTime = 0;
             long endTime = 0;
             beginTime = System.currentTimeMillis();
             String encryptedEPGFileName = "kantv.bin";
             KANTVJNIDecryptBuffer dataEntity = KANTVJNIDecryptBuffer.getInstance();
             KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
             byte[] realPayload = null;
             int nPaddingLen = 0;

             boolean usingRemoteEPG = false;
             File xmlRemoteFile = new File(CDEUtils.getDataPath(mContext), "kantv.bin");
             if (xmlRemoteFile.exists()) {
                 CDELog.j(TAG, "new remote epg file exist: " + xmlRemoteFile.getAbsolutePath());
                 usingRemoteEPG = true;
                 String xmlLoaddedFileName = xmlRemoteFile.getAbsolutePath();
                 CDELog.j(TAG, "load epg from file: " + xmlLoaddedFileName + " which download from customized EPG server:" + CDEUtils.getKANTVServer());

                 //TODO:merge codes of load epg info from encrypt file
                 {
                     beginTime = System.currentTimeMillis();
                     File file = new File(xmlLoaddedFileName);
                     int fileLength = (int) file.length();


                     try {
                         byte[] fileContent = new byte[fileLength];
                         FileInputStream in = new FileInputStream(file);
                         in.read(fileContent);
                         in.close();
                         CDELog.j(TAG, "encrypted file length:" + fileLength);
                         if (fileLength < 2) {
                             CDELog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                             return;
                         }
                         byte[] paddingInfo = new byte[2];
                         paddingInfo[0] = 0;
                         paddingInfo[1] = 0;
                         System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                         CDELog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                         CDELog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                         CDELog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                         nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                         CDELog.j(TAG, "padding len:" + nPaddingLen);

                         byte[] payload = new byte[fileLength - 2];
                         System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                         CDELog.j(TAG, "encrypted file length:" + fileLength);
                         mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                         CDELog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());
                         if (dataEntity.getData() != null) {
                             CDELog.j(TAG, "decrypted ok");
                         } else {
                             CDELog.j(TAG, "decrypted failed");
                         }
                         CDELog.j(TAG, "load encryted epg data ok now");
                         endTime = System.currentTimeMillis();
                         CDELog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                         realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                         System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                         if (!CDEUtils.getReleaseMode()) {
                             CDEUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "download_tv_decrypt_debug.xml");
                         }
                     } catch (Exception ex) {
                         CDELog.j(TAG, "load epg failed:" + ex.toString());
                         usingRemoteEPG = false;
                     }

                 }
                 mContentList = CDEUtils.EPGXmlParser.getContentDescriptors(realPayload);
                 if ((mContentList == null) || (0 == mContentList.size())) {
                     CDELog.j(TAG, "failed to parse xml file:" + xmlLoaddedFileName);
                     usingRemoteEPG = false;
                 } else {
                     CDELog.j(TAG, "ok to parse xml file:" + xmlLoaddedFileName);
                     CDELog.j(TAG, "epg items: " + mContentList.size());
                 }
             } else {
                 CDELog.j(TAG, "no remote epg file exist:" + xmlRemoteFile.getAbsolutePath());
             }


             if (!usingRemoteEPG) {
                 CDELog.j(TAG, "load internal encrypted epg data");
                 CDELog.j(TAG, "encryptedEPGFileName: " + encryptedEPGFileName);
                 CDEAssetLoader.copyAssetFile(mContext, encryptedEPGFileName, CDEAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
                 CDELog.j(TAG, "encrypted asset path:" + CDEAssetLoader.getDataPath(mContext) + encryptedEPGFileName);//asset path:/data/data/com.cdeos.player/kantv.bin
                 try {
                     File file = new File(CDEAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
                     int fileLength = (int) file.length();


                     byte[] fileContent = new byte[fileLength];
                     FileInputStream in = new FileInputStream(file);
                     in.read(fileContent);
                     in.close();

                     CDELog.j(TAG, "encrypted file length:" + fileLength);
                     if (fileLength < 2) {
                         CDELog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                         return;
                     }
                     byte[] paddingInfo = new byte[2];
                     paddingInfo[0] = 0;
                     paddingInfo[1] = 0;
                     System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                     CDELog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                     CDELog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                     CDELog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                     nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                     CDELog.j(TAG, "padding len:" + nPaddingLen);

                     byte[] payload = new byte[fileLength - 2];
                     System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                     CDELog.j(TAG, "encrypted file length:" + fileLength);
                     mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                     CDELog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());
                     if (dataEntity.getData() != null) {
                         CDELog.j(TAG, "decrypted ok");
                     } else {
                         CDELog.j(TAG, "decrypted failed");
                     }
                 } catch (FileNotFoundException e) {
                     CDELog.j(TAG, "load encrypted epg data failed:" + e.getMessage());
                     return;
                 } catch (IOException e) {
                     e.printStackTrace();
                     CDELog.j(TAG, "load encryted epg data failed:" + e.getMessage());
                     return;
                 }
                 CDELog.j(TAG, "load encryted epg data ok now");
                 endTime = System.currentTimeMillis();
                 CDELog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                 realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                 System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                 if (!CDEUtils.getReleaseMode()) {
                     CDEUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "tv_decrypt_debug.xml");
                 }

                 mContentList = CDEUtils.EPGXmlParser.getContentDescriptors(realPayload);
                 if ((mContentList == null) || (0 == mContentList.size())) {
                     CDELog.j(TAG, "xml parse failed");
                     return;
                 }
                 CDELog.j(TAG, "content counts:" + mContentList.size());
             }
         }

     }

     private void layoutUI(Activity activity) {
         layout.removeAllViews();
         Resources res = mActivity.getResources();

         if (mContentList == null) {
             CDELog.j(TAG, "xml parse failed, can't layout ui");
             Button contentButton = new Button(activity);
             contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
             contentButton.setText(mContext.getString(R.string.VideoView_error_text_unknown));
             contentButton.setOnClickListener(new View.OnClickListener() {
                 @Override
                 public void onClick(View v) {

                 }
             });
             layout.addView(contentButton);
             return;
         }

         if (0 == mContentList.size()) {
             Button contentButton = new Button(activity);
             contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
             contentButton.setText(mContext.getString(R.string.VideoView_error_text_unknown));
             contentButton.setAllCaps(false);
             layout.addView(contentButton);
             return;
         } else {
             for (int index = 0; index < mContentList.size(); index++) {
                 CDEContentDescriptor descriptor = mContentList.get(index);
                 CDELog.d(TAG, "url:" + descriptor.getUrl());
                 Button contentButton = new Button(activity);

                 CDEUrlType urlType = descriptor.getType();
                 Drawable icon = null;
                 if (urlType == CDEUrlType.HLS) {
                     icon = res.getDrawable(R.drawable.hls);

                 } else if (urlType == CDEUrlType.RTMP) {
                     icon = res.getDrawable(R.drawable.rtmp);
                 } else if (urlType == CDEUrlType.DASH) {
                     icon = res.getDrawable(R.drawable.dash);
                 } else if (urlType == CDEUrlType.MKV) {
                     icon = res.getDrawable(R.drawable.mkv);
                 } else if (urlType == CDEUrlType.TS) {
                     icon = res.getDrawable(R.drawable.ts);
                 } else if (urlType == CDEUrlType.MP4) {
                     icon = res.getDrawable(R.drawable.mp4);
                 } else if (urlType == CDEUrlType.MP3) {
                     icon = res.getDrawable(R.drawable.mp3);
                 } else if (urlType == CDEUrlType.OGG) {
                     icon = res.getDrawable(R.drawable.ogg);
                 } else if (urlType == CDEUrlType.AAC) {
                     icon = res.getDrawable(R.drawable.aac);
                 } else if (urlType == CDEUrlType.AC3) {
                     icon = res.getDrawable(R.drawable.ac3);
                 } else if (urlType == CDEUrlType.FILE) {
                     icon = res.getDrawable(R.drawable.dash);
                 } else {
                     CDELog.d(TAG, "unknown type:" + urlType.toString());
                     icon = res.getDrawable(R.drawable.file);
                 }
                 // Drawable rightIcon = descriptor.getDrawable(res, R.drawable.hls);
                 Drawable rightIcon = icon;
                 Drawable leftIcon = null;

                 if (descriptor.getIsProtected()) {
                     leftIcon = res.getDrawable(R.drawable.lock);
                 }

                 String txtTitle = String.valueOf(index + 1) + "  " + descriptor.getName().trim();
                 String tvTitle = descriptor.getName().trim();
                 contentButton.setText(tvTitle);
                 contentButton.setAllCaps(false);

                 contentButton.setOnClickListener(new View.OnClickListener() {
                     @Override
                     public void onClick(View v) {
                         long playPos = 0;

                         if (!CDEUtils.isNetworkAvailable(mActivity)) {
                             Toast.makeText(getContext(), "pls check network connection", Toast.LENGTH_LONG).show();
                             return;
                         }
                         if (CDEUtils.getNetworkType() == CDEUtils.NETWORK_MOBILE) {
                             Toast.makeText(getContext(), "you are using 4G/5G network to watch online media", Toast.LENGTH_LONG).show();
                             //return;
                         }

                         if (mSettings.getPlayMode() != CDEUtils.PLAY_MODE_NORMAL) {
                             CDEUtils.perfGetLoopPlayBeginTime();
                         }
                         CDEUtils.perfGetPlaybackBeginTime();
                         CDELog.d(TAG, "beginPlaybackTime: " + CDEUtils.perfGetPlaybackBeginTime());
                         String name = descriptor.getName();
                         String url = descriptor.getUrl();
                         String drmScheme = descriptor.getDrmScheme();
                         String drmLicenseURL = descriptor.getLicenseURL();
                         boolean isLive = descriptor.getIsLive();
                         CDELog.j(TAG, "name:" + name.trim());
                         CDELog.j(TAG, "url:" + url.trim());

                         if (descriptor.getIsProtected()) {
                             CDELog.d(TAG, "drm scheme:" + drmScheme);
                             CDELog.d(TAG, "drm license url:" + drmLicenseURL);
                         } else {
                             CDELog.d(TAG, "clear content");
                         }
                         if (isLive) {
                             CDELog.d(TAG, "live content");
                         } else {
                             CDELog.d(TAG, "vod content");
                         }
                         CDELog.d(TAG, "mediaType: " + mMediaType.toString());
                         CDELog.j(TAG, "url:" + url + " ,name:" + name.trim() + " ,mediaType:" + mMediaType.toString());

                         if (descriptor.getIsProtected()) {
                             CDELog.j(TAG, "drm scheme:" + drmScheme);
                             CDELog.j(TAG, "drm license url:" + drmLicenseURL);
                         } else {
                             CDELog.j(TAG, "clear content");
                         }
                         CDELog.d(TAG, "mediaType: " + mMediaType.toString());


                         initPlayback();

                         PlayerManagerActivity.launchPlayerStream(getContext(), name, url, 0, 0);


                     }
                 });
                 layout.addView(contentButton);
             }

             if (mTroubleshootingMode) {
                 Button contentButton = new Button(activity);
                 contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
                 contentButton.setText(mContext.getString(R.string.settings));
                 contentButton.setOnClickListener(new View.OnClickListener() {
                     @Override
                     public void onClick(View v) {

                     }
                 });
                 layout.addView(contentButton);

                 String activityName = activity.getClass().getName();
                 if (CDEUtils.isRunningOnPhone()) {
                     if (activityName.contains("TroubleshootingActivity")) {
                         contentButton = new Button(activity);
                         contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
                         contentButton.setText(R.string.exit);
                         contentButton.setOnClickListener(new View.OnClickListener() {
                             @Override
                             public void onClick(View v) {
                                 CDEUtils.exitAPK(mActivity);
                             }
                         });
                         layout.addView(contentButton);
                     }
                 }

             }
         }
     }
 }
