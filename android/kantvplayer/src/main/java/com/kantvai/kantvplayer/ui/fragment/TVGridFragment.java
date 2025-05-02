 /*
  * Copyright (c) Project KanTV. 2021-2023
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

     private int testResID = 0;

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

         try {
             mData.clear();
             mContentList.clear();

             //load EPG from /sdcard/tv.xml because this file can be edited by user
             File file = new File(KANTVUtils.getSDCardDataPath() + "tv.xml");// load EPG from /sdcard/tv.xml
             byte[] fileContent = KANTVUtils.loadContentFromFile(file);
             if (null != fileContent) {
                 mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(fileContent);
             }
             if ((mContentList == null) || (0 == mContentList.size())) {
                 KANTVLog.g(TAG, "failed to load tv xml from:" + KANTVUtils.getSDCardDataPath());
                 //load EPG from internal tv.xml if failed to load EPG from /sdcard/tv.xml
                 File backupFile = new File(KANTVUtils.getDataPath(mContext) + "tv.xml");
                 byte[] backupFileContent = KANTVUtils.loadContentFromFile(backupFile);
                 mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(backupFileContent);
             }
         } catch (Exception ex) {
             KANTVLog.g(TAG, "failed to load tv xml:" + ex.toString());
             //load EPG from internal tv.xml if failed to load EPG from /sdcard/tv.xml
             File backupFile = new File(KANTVUtils.getDataPath(mContext) + "tv.xml");
             byte[] backupFileContent = KANTVUtils.loadContentFromFile(backupFile);
             mContentList = KANTVUtils.EPGXmlParser.getContentDescriptors(backupFileContent);
         }

         if ((mContentList == null) || (0 == mContentList.size())) {
             KANTVLog.g(TAG, "shouldn't happen, pls check tv.xml");
             //failed to load EPG from /sdcard/tv.xml and internal tv.xml
             KANTVUtils.showMsgBox(mActivity, "pls check tv.xml in " + KANTVUtils.getSDCardDataPath());
             return;
         }
         KANTVLog.d(TAG, "epg items: " + mContentList.size());

         if (KANTVMediaType.MEDIA_TV == mMediaType) {
             KANTVLog.d(TAG, "load tv logos from internal resources");
             Resources res = mActivity.getResources();

             Field field;
             String resName = "test";
             int resID = 0;
             if (resName.equals("test")) {
                 resID = res.getIdentifier("test", "mipmap", mActivity.getPackageName());
                 testResID = resID;
                 KANTVLog.g(TAG, "test res id: " + resID);
             }
             for (int index = 0; index < mContentList.size(); index++) {
                 resID = 0;
                 KANTVContentDescriptor descriptor = mContentList.get(index);
                 String posterName = descriptor.getPoster();
                 if (posterName != null) {
                     posterName = posterName.trim();
                     resName = posterName.substring(0, posterName.indexOf('.'));
                 } else {
                     KANTVLog.g(TAG, "can't find poster name, pls check epg data in tv.xml");
                     //can't find a valid poster for a specified TV program, use the default "test.jpg"
                     resName = "test";
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

             KANTVLog.g(TAG, "load tv program counts:" + mData.size());
         }
         endTime = System.currentTimeMillis();
         KANTVLog.g(TAG, "load EPG cost " + (endTime - beginTime) + " milliseconds");
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
                     KANTVLog.d(TAG, "load img from internal res");
                     holder.setImageResource(gridItemImageID, obj.getItemId());
                     if (obj.getItemId() == testResID) {
                         //TODO: dynamically generate a poster from user's specified "text"
                         //      text ---> poster image
                         holder.setText(gridItemTextID, obj.getItemName());
                     }
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
