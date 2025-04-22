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

 package com.kantvai.kantvplayer.ui.fragment;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.content.Intent;
 import android.content.res.Resources;
 import android.net.Uri;
 import android.os.Environment;
 import android.view.View;
 import android.widget.AdapterView;
 import android.widget.BaseAdapter;
 import android.widget.GridView;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.appcompat.app.AppCompatActivity;

 import com.kantvai.kantvplayer.mvp.impl.TVGridPresenterImpl;
 import com.kantvai.kantvplayer.mvp.presenter.TVGridPresenter;
 import com.kantvai.kantvplayer.mvp.view.TVGridView;
 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.base.BaseMvpFragment;
 import com.kantvai.kantvplayer.base.BaseRvAdapter;
 import com.kantvai.kantvplayer.bean.FolderBean;
 import com.kantvai.kantvplayer.bean.event.SendMsgEvent;
 import com.kantvai.kantvplayer.player.common.utils.Constants;;
 import com.kantvai.kantvplayer.ui.activities.ShellActivity;
 import com.kantvai.kantvplayer.ui.activities.play.PlayerManagerActivity;
 import com.kantvai.kantvplayer.utils.AppConfig;
 import com.kantvai.kantvplayer.utils.Settings;

 import org.greenrobot.eventbus.EventBus;
 import org.greenrobot.eventbus.*;

 import java.io.File;
 import java.io.FileInputStream;
 import java.io.FileNotFoundException;
 import java.io.IOException;
 import java.lang.reflect.Field;
 import java.util.ArrayList;
 import java.util.Arrays;
 import java.util.List;

 import butterknife.BindView;
 import kantvai.media.player.KANTVJNIDecryptBuffer;
 import kantvai.media.player.KANTVMediaGridAdapter;
 import kantvai.media.player.KANTVMediaGridItem;
 import io.reactivex.disposables.Disposable;
 import kantvai.media.player.KANTVDRM;
 import kantvai.media.player.KANTVAssetLoader;
 import kantvai.media.player.KANTVContentDescriptor;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVMediaType;
 import kantvai.media.player.KANTVUtils;
 import kantvai.media.player.KANTVDRMManager;


 public class TVGridFragment extends BaseMvpFragment<TVGridPresenter> implements TVGridView {
     private static final String TAG = TVGridFragment.class.getName();

     @BindView(R.id.tv_grid_layout)
     GridView layout;

     private KANTVMediaType mMediaType = KANTVMediaType.MEDIA_TV;
     private final static KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
     private boolean mEPGDataLoadded = false;

     private Context mContext;
     private Activity mActivity;
     private Settings mSettings;

     private BaseAdapter mAdapter = null;
     private ArrayList<KANTVMediaGridItem> mData = new ArrayList<KANTVMediaGridItem>();
     private List<KANTVContentDescriptor> mContentList = new ArrayList<KANTVContentDescriptor>();

     private int gridItemImageID = 0;
     private int gridItemTextID = 0;

     public static TVGridFragment newInstance() {
         return new TVGridFragment();
     }

     @NonNull
     @Override
     protected TVGridPresenter initPresenter() {
         return new TVGridPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_tvgrid;
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
         mSettings.updateUILang((AppCompatActivity) mActivity);

         loadEPGData();
         mEPGDataLoadded = true;
         layoutUI(mActivity);

         endTime = System.currentTimeMillis();
         KANTVLog.d(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     @Override
     public void initListener() {
     }


     @Override
     public void onDestroy() {
         super.onDestroy();
         EventBus.getDefault().unregister(this);
     }


     @Override
     public void onResume() {
         super.onResume();
         if (mEPGDataLoadded) {
             KANTVLog.d(TAG, "epg data already loadded");
         } else {
             KANTVLog.d(TAG, "epg data not loadded");
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
                 KANTVLog.e(TAG, "init drm failed");
                 return;
             }
             mKANTVDRM.ANDROID_JNI_SetLocalEMS(KANTVUtils.getLocalEMS());
             KANTVDRMManager.setMultiDrmInfo(KANTVUtils.getDrmScheme());
             KANTVLog.d(TAG, "after calling ServiceManager.openPlaybackService ");
         }
     }


     //TODO:merge codes of local epg info from remote file or local file
     private void loadEPGData() {
         long beginTime = 0;
         long endTime = 0;
         beginTime = System.currentTimeMillis();
         KANTVLog.d(TAG, "epg updated status:" + KANTVUtils.getEPGUpdatedStatus(mMediaType));
         mContentList.clear();
         /* 2025-01-29, remove encrypted kantv.bin
         String encryptedEPGFileName = "kantv.bin";
         KANTVJNIDecryptBuffer dataEntity = KANTVJNIDecryptBuffer.getInstance();
         KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
         byte[] realPayload = null;
         boolean usingRemoteEPG = false;
         File xmlRemoteFile = new File(KANTVUtils.getDataPath(mContext), "kantv.bin");
         int nPaddingLen = 0;

         mData.clear();
         mContentList.clear();

         if (xmlRemoteFile.exists()) {
             KANTVLog.d(TAG, "new remote epg file exist: " + xmlRemoteFile.getAbsolutePath());
             usingRemoteEPG = true;
             String xmlLoaddedFileName = xmlRemoteFile.getAbsolutePath();
             KANTVLog.d(TAG, "load epg from file: " + xmlLoaddedFileName + " which download from Master EPG server:" + KANTVUtils.getKANTVMasterServer());

             {
                 beginTime = System.currentTimeMillis();
                 File file = new File(xmlLoaddedFileName);
                 int fileLength = (int) file.length();
                 KANTVLog.d(TAG, "encrypted file len:" + fileLength);

                 try {

                     byte[] fileContent = new byte[fileLength];
                     FileInputStream in = new FileInputStream(file);
                     in.read(fileContent);
                     in.close();
                     KANTVLog.d(TAG, "encrypted file length:" + fileLength);
                     if (fileLength < 2) {
                         KANTVLog.d(TAG, "shouldn't happen, pls check encrypted epg data");
                         return;
                     }
                     byte[] paddingInfo = new byte[2];
                     paddingInfo[0] = 0;
                     paddingInfo[1] = 0;
                     System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                     KANTVLog.d(TAG, "padding len:" + Arrays.toString(paddingInfo));
                     KANTVLog.d(TAG, "padding len:" + (int) paddingInfo[0]);
                     KANTVLog.d(TAG, "padding len:" + (int) paddingInfo[1]);
                     nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                     KANTVLog.d(TAG, "padding len:" + nPaddingLen);

                     byte[] payload = new byte[fileLength - 2];
                     System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);


                     KANTVLog.d(TAG, "encrypted file length:" + fileLength);
                     mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                     KANTVLog.d(TAG, "decrypted data length:" + dataEntity.getDataLength());
                     if (dataEntity.getData() != null) {
                         KANTVLog.d(TAG, "decrypted ok");
                     } else {
                         KANTVLog.d(TAG, "decrypted failed");
                     }

                     KANTVLog.d(TAG, "load encryted epg data ok now");
                     endTime = System.currentTimeMillis();
                     KANTVLog.d(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
                     realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
                     System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
                     if (!KANTVUtils.getReleaseMode()) {
                         KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "download_tv_decrypt_debug.xml");
                         File sdcardPath = Environment.getExternalStorageDirectory();
                         KANTVLog.d(TAG, "sdcardPath name:" + sdcardPath.getName() + ",sdcardPath:" + sdcardPath.getPath());
                         KANTVLog.d(TAG, "sdcard free size:" + mKANTVDRM.ANDROID_JNI_GetDiskFreeSize(sdcardPath.getAbsolutePath()) + "MBytes");
                     }
                 } catch (Exception ex) {
                     KANTVLog.d(TAG, "load epg failed:" + ex.toString());
                     usingRemoteEPG = false;
                 }

             }
             mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
             if ((mContentList == null) || (0 == mContentList.size())) {
                 KANTVLog.d(TAG, "failed to parse xml file:" + xmlLoaddedFileName);
                 usingRemoteEPG = false;
             } else {
                 KANTVLog.d(TAG, "ok to parse xml file:" + xmlLoaddedFileName);
                 KANTVLog.d(TAG, "epg items: " + mContentList.size());
             }
         } else {
             KANTVLog.d(TAG, "no remote epg file exist:" + xmlRemoteFile.getAbsolutePath());
         }

         if (!usingRemoteEPG) {
             KANTVLog.d(TAG, "load internal encrypted epg data");
             KANTVLog.d(TAG, "encryptedEPGFileName: " + encryptedEPGFileName);
             KANTVAssetLoader.copyAssetFile(mContext, encryptedEPGFileName, KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
             KANTVLog.d(TAG, "encrypted asset path:" + KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);//asset path:/data/data/com.kantvai.player/kantv.bin
             try {
                 File file = new File(KANTVAssetLoader.getDataPath(mContext) + encryptedEPGFileName);
                 int fileLength = (int) file.length();
                 KANTVLog.d(TAG, "encrypted file len:" + fileLength);

                 if (fileLength < 2) {
                     KANTVLog.d(TAG, "shouldn't happen, pls check encrypted epg data");
                     return;
                 }


                 byte[] fileContent = new byte[fileLength];
                 FileInputStream in = new FileInputStream(file);
                 in.read(fileContent);
                 in.close();

                 byte[] paddingInfo = new byte[2];
                 paddingInfo[0] = 0;
                 paddingInfo[1] = 0;
                 System.arraycopy(fileContent, 0, paddingInfo, 0, 2);
                 KANTVLog.d(TAG, "padding len:" + Arrays.toString(paddingInfo));
                 KANTVLog.d(TAG, "padding len:" + (int) paddingInfo[0]);
                 KANTVLog.d(TAG, "padding len:" + (int) paddingInfo[1]);
                 nPaddingLen = (((paddingInfo[0] & 0xFF) << 8) | (paddingInfo[1] & 0xFF));
                 KANTVLog.d(TAG, "padding len:" + nPaddingLen);

                 byte[] payload = new byte[fileLength - 2];
                 System.arraycopy(fileContent, 2, payload, 0, fileLength - 2);

                 KANTVLog.d(TAG, "encrypted file length:" + fileLength);
                 mKANTVDRM.ANDROID_JNI_Decrypt(payload, fileLength - 2, dataEntity, KANTVJNIDecryptBuffer.getDirectBuffer());
                 KANTVLog.d(TAG, "decrypted data length:" + dataEntity.getDataLength());
                 if (dataEntity.getData() != null) {
                     KANTVLog.d(TAG, "decrypted ok");
                 } else {
                     KANTVLog.d(TAG, "decrypted failed");
                 }
             } catch (FileNotFoundException e) {
                 KANTVLog.d(TAG, "load encrypted epg data failed:" + e.getMessage());
                 return;
             } catch (IOException e) {
                 e.printStackTrace();
                 KANTVLog.d(TAG, "load encryted epg data failed:" + e.getMessage());
                 return;
             }
             KANTVLog.d(TAG, "load encryted epg data ok now");
             endTime = System.currentTimeMillis();
             KANTVLog.d(TAG, "load encrypted epg data cost: " + (endTime - beginTime) + " milliseconds");
             realPayload = new byte[dataEntity.getDataLength() - nPaddingLen];
             System.arraycopy(dataEntity.getData(), 0, realPayload, 0, dataEntity.getDataLength() - nPaddingLen);
             if (!KANTVUtils.getReleaseMode()) {
                 KANTVUtils.bytesToFile(realPayload, Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv/", "tv_decrypt_debug.xml");
                 File sdcardPath = Environment.getExternalStorageDirectory();
                 KANTVLog.d(TAG, "sdcardPath name:" + sdcardPath.getName() + ",sdcardPath:" + sdcardPath.getPath());
                 KANTVLog.d(TAG, "sdcard free size:" + mKANTVDRM.ANDROID_JNI_GetDiskFreeSize(sdcardPath.getAbsolutePath()));
             }

             mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(realPayload);
             if ((mContentList == null) || (0 == mContentList.size())) {
                 KANTVLog.d(TAG, "xml parse failed");
                 return;
             }
             KANTVLog.d(TAG, "content counts:" + mContentList.size());
         }
         */

         //2025-01-29,customize TV program in non-encrypted tv.xml
         {
             KANTVUtils.copyAssetFile(mContext, "tv.xml", KANTVUtils.getDataPath(mContext) + "/tv.xml");
             File file = new File(KANTVUtils.getDataPath(mContext), "tv.xml");
             beginTime = System.currentTimeMillis();
             int fileLength = (int) file.length();
             KANTVLog.j(TAG, "clear file len:" + fileLength);
             mData.clear();
             mContentList.clear();

             try {
                 byte[] fileContent = new byte[fileLength];
                 FileInputStream in = new FileInputStream(file);
                 in.read(fileContent);
                 in.close();
                 if (fileLength < 2) {
                     KANTVLog.j(TAG, "shouldn't happen, pls check tv.xml");
                     return;
                 }
                 mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(fileContent);
             } catch (Exception ex) {
                 KANTVLog.j(TAG, "load tv xml failed:" + ex.toString());
                 KANTVLog.d(TAG, "shouldn't happen, pls check tv.xml");
                 return;
             }
         }

         if ((mContentList == null) || (0 == mContentList.size())) {
             KANTVLog.j(TAG, "shouldn't happen, pls check tv.xml");
             return;
         }
         KANTVLog.d(TAG, "epg items: " + mContentList.size());

         if (KANTVMediaType.MEDIA_TV == mMediaType) {
             {
                 KANTVLog.d(TAG, "load tv logos from internal resources");
                 Resources res = mActivity.getResources();

                 if (true) {
                     Field field;
                     String resName = "test";
                     int resID = 0;
                     for (int index = 0; index < mContentList.size(); index++) {
                         resID = 0;
                         KANTVContentDescriptor descriptor = mContentList.get(index);
                         String posterName = descriptor.getPoster();
                         if (posterName != null) {
                             posterName = posterName.trim();
                             resName = posterName.substring(0, posterName.indexOf('.'));
                         } else {
                             KANTVLog.d(TAG, "can't find poster name, pls check epg data");
                         }
                         String title = descriptor.getName();
                         title = title.trim();
                         String url = descriptor.getUrl();
                         url = url.trim();
                         //KANTVLog.d(TAG, "real resource name:" + resName);
                         //KANTVLog.d(TAG, "url:" + url );
                         //KANTVLog.d(TAG, "title:" + title);
                         //KANTVLog.d(TAG, "poster:" + posterName);
                         resID = res.getIdentifier(resName, "mipmap", mActivity.getPackageName());
                         if (resID == 0) {
                             KANTVLog.d(TAG, "can't find resource name:" + posterName);
                             resID = res.getIdentifier("test", "mipmap", mActivity.getPackageName());
                         }
                         mData.add(new KANTVMediaGridItem(resID, title));
                     }
                 } else {
                     mData.add(new KANTVMediaGridItem(R.mipmap.cgtn, "CGTN"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.cgtn_doc, "CGTN Doc"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.bloomberg_tv_emea, "Bloomberg TV"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.dw, "DW"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.abc_news_live, "ABC News Live"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.fox_news_now, "Fox News Live Now"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.cnn_us, "CNN"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.cbn_news, "CBN News"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.fox_5_dc, " FOX 5 Washington DC"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.fox_5_newyork, "FOX 5 New York"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.nbc_news_chicago, "NBC Chicago News"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.news_12_newyork, "New York News"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.nasa_media, "NASA TV Media"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.nasa_uhd, "NASA TV Public"));

                     mData.add(new KANTVMediaGridItem(R.mipmap.bbc, "BBC sports"));
                     mData.add(new KANTVMediaGridItem(R.mipmap.cri, "China Radio"));
                 }
                 KANTVLog.d(TAG, "load internal resource counts:" + mData.size());
             }
         }
     }


     private void layoutUI(Activity activity) {
         int gridItemID = 0;
         if (KANTVMediaType.MEDIA_TV == mMediaType) {
             gridItemID = R.layout.content_grid_item;
             gridItemImageID = R.id.img_icon;
             gridItemTextID = R.id.txt_icon;
         } else if (KANTVMediaType.MEDIA_MOVIE == mMediaType) {
             if (KANTVUtils.isRunningOnTV()) {
                 gridItemID = R.layout.content_grid_item_tv;
                 gridItemImageID = R.id.img_icon_tv;
                 gridItemTextID = R.id.txt_icon_tv;
             } else {
                 gridItemID = R.layout.content_grid_item;
                 gridItemImageID = R.id.img_icon;
                 gridItemTextID = R.id.txt_icon;
             }
         }

         mAdapter = new KANTVMediaGridAdapter<KANTVMediaGridItem>(mData, gridItemID, activity) {
             @Override
             public void bindView(KANTVMediaGridAdapter.ViewHolder holder, KANTVMediaGridItem obj) {
                 try {
                     {
                         KANTVLog.d(TAG, "load img from interl res");
                         holder.setImageResource(gridItemImageID, obj.getItemId());
                     }
                     //holder.setText(gridItemTextID, obj.getItemName());
                 } catch (Exception ex) {
                     KANTVLog.d(TAG, "error: " + ex.toString());
                 }
             }
         };
         layout.setAdapter(mAdapter);

         layout.setOnItemClickListener(new AdapterView.OnItemClickListener() {
             @Override
             public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                 if (!KANTVUtils.isNetworkAvailable(mActivity)) {
                     Toast.makeText(getContext(), "pls check network connection", Toast.LENGTH_LONG).show();
                     return;
                 }
                 if (KANTVUtils.getNetworkType() == KANTVUtils.NETWORK_MOBILE) {
                     Toast.makeText(getContext(), "warning:you are using 3G/4G/5G network to watch online media", Toast.LENGTH_LONG).show();
                     //return;
                 }

                 KANTVUtils.perfGetPlaybackBeginTime();
                 KANTVLog.d(TAG, "beginPlaybackTime: " + KANTVUtils.perfGetPlaybackBeginTime());
                 KANTVMediaGridItem item = (KANTVMediaGridItem) parent.getItemAtPosition(position);
                 KANTVLog.d(TAG, "item position" + position);
                 KANTVContentDescriptor descriptor = mContentList.get(position);
                 String name = descriptor.getName();
                 String url = descriptor.getUrl();
                 String drmScheme = descriptor.getDrmScheme();
                 String drmLicenseURL = descriptor.getLicenseURL();
                 boolean isLive = descriptor.getIsLive();
                 long playPos = 0;

                 KANTVLog.d(TAG, "name:" + name.trim());
                 KANTVLog.d(TAG, "url:" + url.trim());
                 KANTVLog.d(TAG, "mediaType: " + mMediaType.toString());
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
                 KANTVLog.d(TAG, "Program： " + item.getItemName() + " ,url:" + url + " ,name:" + name.trim() + " ,mediaType:" + mMediaType.toString());


                 if (descriptor.getIsProtected()) {
                     KANTVLog.d(TAG, "drm scheme:" + drmScheme);
                     KANTVLog.d(TAG, "drm license url:" + drmLicenseURL);
                 } else {
                     KANTVLog.d(TAG, "clear content");
                 }
                 KANTVLog.d(TAG, "mediaType: " + mMediaType.toString());


                 initPlayback();

                 PlayerManagerActivity.launchPlayerOnline(getContext(), name, url, 0, 0, "");
             }
         });
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
