 /*
  * Copyright (c) Project KanTV. 2021-2023
  * Copyright (c) 2024- KanTV Authors
  */
 package com.kantvai.kantvplayer.ui.fragment;

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

 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.base.BaseMvpFragment;
 import com.kantvai.kantvplayer.bean.event.SendMsgEvent;
 import com.kantvai.kantvplayer.mvp.impl.EPGListPresenterImpl;
 import com.kantvai.kantvplayer.mvp.presenter.EPGListPresenter;
 import com.kantvai.kantvplayer.mvp.view.EPGListView;
 import com.kantvai.kantvplayer.player.common.utils.Constants;
 import com.kantvai.kantvplayer.ui.activities.play.PlayerManagerActivity;
 import com.kantvai.kantvplayer.utils.AppConfig;
 import com.kantvai.kantvplayer.utils.Settings;


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
 import kantvai.media.player.KANTVJNIDecryptBuffer;
 import kantvai.media.player.KANTVDRM;
 import kantvai.media.player.KANTVAssetLoader;
 import kantvai.media.player.KANTVContentDescriptor;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVMediaType;
 import kantvai.media.player.KANTVUrlType;
 import kantvai.media.player.KANTVUtils;
 import kantvai.media.player.KANTVDRMManager;


 public class EPGListFragment extends BaseMvpFragment<EPGListPresenter> implements EPGListView {
     private final static String TAG = EPGListFragment.class.getName();

     @BindView(R.id.epg_list_layout)
     LinearLayout layout;

     private KANTVMediaType mMediaType = KANTVMediaType.MEDIA_TV;
     private List<KANTVContentDescriptor> mContentList = new ArrayList<KANTVContentDescriptor>();
     private List<KANTVContentDescriptor> mIPTVContentList = new ArrayList<KANTVContentDescriptor>();
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
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
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
             KANTVLog.d(TAG, "epg data already loaded");
         } else {
             KANTVLog.d(TAG, "epg data not loaded");
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
             mKANTVDRM.ANDROID_JNI_Init(mActivity.getBaseContext(), KANTVAssetLoader.getDataPath(mActivity.getBaseContext()));
             mKANTVDRM.ANDROID_JNI_SetLocalEMS(KANTVUtils.getLocalEMS());
         }
         if (AppConfig.getInstance().getPlayerType() == Constants.PLAYERENGINE__Exoplayer) {
             if (KANTVDRMManager.openPlaybackService() != 0) {
                 KANTVLog.e(TAG, "drm init failed");
                 return;
             }
             mKANTVDRM.ANDROID_JNI_SetLocalEMS(KANTVUtils.getLocalEMS());
             KANTVDRMManager.setMultiDrmInfo(KANTVUtils.getDrmScheme());
         }
     }


     private void loadEPGData() {
         KANTVLog.j(TAG, "epg updated status:" + KANTVUtils.getEPGUpdatedStatus(mMediaType));
         mContentList.clear();

         String remoteEPGUrl = "http://" + KANTVUtils.getKANTVServer() + "/kantv/epg/" + "test.m3u";
         String localTestEPGUrl = "test.m3u";
         KANTVUtils.copyAssetFile(mContext, localTestEPGUrl, KANTVUtils.getDataPath(mContext) + "/test.m3u");
         File fileLocalTestEPGUrl = new File(KANTVUtils.getDataPath(mContext) + "/test.m3u");
         KANTVLog.j(TAG, "remote epg url:" + remoteEPGUrl);
        /*
        mIPTVContentList = KANTVUtils.loadIPTVProgram(mContext, remoteEPGUrl);

        if ((mIPTVContentList != null) && (mIPTVContentList.size() > 0)) {
            KANTVLog.j(TAG, "iptv item size:" + mIPTVContentList.size());
            mContentList = mIPTVContentList;
            KANTVLog.j(TAG, "program item size:" + mContentList.size());
        } else */
         if (fileLocalTestEPGUrl.exists()) {
             KANTVLog.j(TAG, "using local epg file:" + KANTVUtils.getDataPath(mContext) + "/test.m3u");
             mIPTVContentList = KANTVUtils.loadIPTVProgram(mContext, KANTVUtils.getDataPath(mContext) + "/test.m3u");
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
             File xmlRemoteFile = new File(KANTVUtils.getDataPath(mContext), "kantv.bin");
             if (xmlRemoteFile.exists()) {
                 KANTVLog.j(TAG, "new remote epg file exist: " + xmlRemoteFile.getAbsolutePath());
                 usingRemoteEPG = true;
                 String xmlLoaddedFileName = xmlRemoteFile.getAbsolutePath();
                 KANTVLog.j(TAG, "load epg from file: " + xmlLoaddedFileName + " which download from customized EPG server:" + KANTVUtils.getKANTVServer());

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
                         KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                         if (fileLength < 2) {
                             KANTVLog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                             return;
                         }
                         byte[] paddingInfo = new byte[2];
                         paddingInfo[0] = 0;
                         paddingInfo[1] = 0;
                         System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                         KANTVLog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                         KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                         KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                         nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                         KANTVLog.j(TAG, "padding len:" + nPaddingLen);

                         byte[] payload = new byte[fileLength - 2];
                         System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                         KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                         mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                         KANTVLog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());
                         if (dataEntity.getData() != null) {
                             KANTVLog.j(TAG, "decrypted ok");
                         } else {
                             KANTVLog.j(TAG, "decrypted failed");
                         }
                         KANTVLog.j(TAG, "load encryted epg data ok now");
                         endTime = System.currentTimeMillis();
                         KANTVLog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                         realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                         System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                         if (!KANTVUtils.getReleaseMode()) {
                             KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "download_tv_decrypt_debug.xml");
                         }
                     } catch (Exception ex) {
                         KANTVLog.j(TAG, "load epg failed:" + ex.toString());
                         usingRemoteEPG = false;
                     }

                 }
                 mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
                 if ((mContentList == null) || (0 == mContentList.size())) {
                     KANTVLog.j(TAG, "failed to parse xml file:" + xmlLoaddedFileName);
                     usingRemoteEPG = false;
                 } else {
                     KANTVLog.j(TAG, "ok to parse xml file:" + xmlLoaddedFileName);
                     KANTVLog.j(TAG, "epg items: " + mContentList.size());
                 }
             } else {
                 KANTVLog.j(TAG, "no remote epg file exist:" + xmlRemoteFile.getAbsolutePath());
             }


             if (!usingRemoteEPG) {
                 KANTVLog.j(TAG, "load internal encrypted epg data");
                 KANTVLog.j(TAG, "encryptedEPGFileName: " + encryptedEPGFileName);
                 KANTVAssetLoader.copyAssetFile(mContext, encryptedEPGFileName, KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
                 KANTVLog.j(TAG, "encrypted asset path:" + KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);//asset path:/data/data/com.kantvai.player/kantv.bin
                 try {
                     File file = new File(KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
                     int fileLength = (int) file.length();


                     byte[] fileContent = new byte[fileLength];
                     FileInputStream in = new FileInputStream(file);
                     in.read(fileContent);
                     in.close();

                     KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                     if (fileLength < 2) {
                         KANTVLog.j(TAG, "shouldn't happen, pls check encrypted epg data");
                         return;
                     }
                     byte[] paddingInfo = new byte[2];
                     paddingInfo[0] = 0;
                     paddingInfo[1] = 0;
                     System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                     KANTVLog.j(TAG, "padding len:" + Arrays.toString(paddingInfo));
                     KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[0]);
                     KANTVLog.j(TAG, "padding len:" + (int) paddingInfo[1]);
                     nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                     KANTVLog.j(TAG, "padding len:" + nPaddingLen);

                     byte[] payload = new byte[fileLength - 2];
                     System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                     KANTVLog.j(TAG, "encrypted file length:" + fileLength);
                     mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                     KANTVLog.j(TAG, "decrypted data length:" + dataEntity.getDataLength());
                     if (dataEntity.getData() != null) {
                         KANTVLog.j(TAG, "decrypted ok");
                     } else {
                         KANTVLog.j(TAG, "decrypted failed");
                     }
                 } catch (FileNotFoundException e) {
                     KANTVLog.j(TAG, "load encrypted epg data failed:" + e.getMessage());
                     return;
                 } catch (IOException e) {
                     e.printStackTrace();
                     KANTVLog.j(TAG, "load encryted epg data failed:" + e.getMessage());
                     return;
                 }
                 KANTVLog.j(TAG, "load encryted epg data ok now");
                 endTime = System.currentTimeMillis();
                 KANTVLog.j(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                 realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                 System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                 if (!KANTVUtils.getReleaseMode()) {
                     KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "tv_decrypt_debug.xml");
                 }

                 mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
                 if ((mContentList == null) || (0 == mContentList.size())) {
                     KANTVLog.j(TAG, "xml parse failed");
                     return;
                 }
                 KANTVLog.j(TAG, "content counts:" + mContentList.size());
             }
         }

     }

     private void layoutUI(Activity activity) {
         layout.removeAllViews();
         Resources res = mActivity.getResources();

         if (mContentList == null) {
             KANTVLog.j(TAG, "xml parse failed, can't layout ui");
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
                 KANTVContentDescriptor descriptor = mContentList.get(index);
                 KANTVLog.d(TAG, "url:" + descriptor.getUrl());
                 Button contentButton = new Button(activity);

                 KANTVUrlType urlType = descriptor.getType();
                 Drawable icon = null;
                 if (urlType == KANTVUrlType.HLS) {
                     icon = res.getDrawable(R.drawable.hls);

                 } else if (urlType == KANTVUrlType.RTMP) {
                     icon = res.getDrawable(R.drawable.rtmp);
                 } else if (urlType == KANTVUrlType.DASH) {
                     icon = res.getDrawable(R.drawable.dash);
                 } else if (urlType == KANTVUrlType.MKV) {
                     icon = res.getDrawable(R.drawable.mkv);
                 } else if (urlType == KANTVUrlType.TS) {
                     icon = res.getDrawable(R.drawable.ts);
                 } else if (urlType == KANTVUrlType.MP4) {
                     icon = res.getDrawable(R.drawable.mp4);
                 } else if (urlType == KANTVUrlType.MP3) {
                     icon = res.getDrawable(R.drawable.mp3);
                 } else if (urlType == KANTVUrlType.OGG) {
                     icon = res.getDrawable(R.drawable.ogg);
                 } else if (urlType == KANTVUrlType.AAC) {
                     icon = res.getDrawable(R.drawable.aac);
                 } else if (urlType == KANTVUrlType.AC3) {
                     icon = res.getDrawable(R.drawable.ac3);
                 } else if (urlType == KANTVUrlType.FILE) {
                     icon = res.getDrawable(R.drawable.dash);
                 } else {
                     KANTVLog.d(TAG, "unknown type:" + urlType.toString());
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

                         if (!KANTVUtils.isNetworkAvailable(mActivity)) {
                             Toast.makeText(getContext(), "pls check network connection", Toast.LENGTH_LONG).show();
                             return;
                         }
                         if (KANTVUtils.getNetworkType() == KANTVUtils.NETWORK_MOBILE) {
                             Toast.makeText(getContext(), "you are using 4G/5G network to watch online media", Toast.LENGTH_LONG).show();
                             //return;
                         }

                         if (mSettings.getPlayMode() != KANTVUtils.PLAY_MODE_NORMAL) {
                             KANTVUtils.perfGetLoopPlayBeginTime();
                         }
                         KANTVUtils.perfGetPlaybackBeginTime();
                         KANTVLog.d(TAG, "beginPlaybackTime: " + KANTVUtils.perfGetPlaybackBeginTime());
                         String name = descriptor.getName();
                         String url = descriptor.getUrl();
                         String drmScheme = descriptor.getDrmScheme();
                         String drmLicenseURL = descriptor.getLicenseURL();
                         boolean isLive = descriptor.getIsLive();
                         KANTVLog.j(TAG, "name:" + name.trim());
                         KANTVLog.j(TAG, "url:" + url.trim());

                         if (descriptor.getIsProtected()) {
                             KANTVLog.d(TAG, "drm scheme:" + drmScheme);
                             KANTVLog.d(TAG, "drm license url:" + drmLicenseURL);
                         } else {
                             KANTVLog.d(TAG, "clear content");
                         }
                         if (isLive) {
                             KANTVLog.d(TAG, "live content");
                         } else {
                             KANTVLog.d(TAG, "vod content");
                         }
                         KANTVLog.d(TAG, "mediaType: " + mMediaType.toString());
                         KANTVLog.j(TAG, "url:" + url + " ,name:" + name.trim() + " ,mediaType:" + mMediaType.toString());

                         if (descriptor.getIsProtected()) {
                             KANTVLog.j(TAG, "drm scheme:" + drmScheme);
                             KANTVLog.j(TAG, "drm license url:" + drmLicenseURL);
                         } else {
                             KANTVLog.j(TAG, "clear content");
                         }
                         KANTVLog.d(TAG, "mediaType: " + mMediaType.toString());


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
                 if (KANTVUtils.isRunningOnPhone()) {
                     if (activityName.contains("TroubleshootingActivity")) {
                         contentButton = new Button(activity);
                         contentButton.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
                         contentButton.setText(R.string.exit);
                         contentButton.setOnClickListener(new View.OnClickListener() {
                             @Override
                             public void onClick(View v) {
                                 KANTVUtils.exitAPK(mActivity);
                             }
                         });
                         layout.addView(contentButton);
                     }
                 }

             }
         }
     }
 }
