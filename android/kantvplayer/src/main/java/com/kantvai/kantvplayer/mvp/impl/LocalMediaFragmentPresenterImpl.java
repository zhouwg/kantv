package com.kantvai.kantvplayer.mvp.impl;

import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;

import androidx.lifecycle.LifecycleOwner;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.StringUtils;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.bean.FolderBean;
import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.mvp.presenter.LocalMediaFragmentPresenter;
import com.kantvai.kantvplayer.mvp.view.LocalMediaFragmentView;
import com.kantvai.kantvplayer.ui.activities.play.PlayerManagerActivity;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.Constants;
import com.kantvai.kantvplayer.utils.database.DataBaseInfo;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.utils.database.callback.QueryAsyncResultCallback;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


public class LocalMediaFragmentPresenterImpl extends BaseMvpPresenterImpl<LocalMediaFragmentView> implements LocalMediaFragmentPresenter {
    private static final String TAG = LocalMediaFragmentPresenterImpl.class.getName();
    public LocalMediaFragmentPresenterImpl(LocalMediaFragmentView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {
        KANTVLog.j(TAG, "init");
    }

    @Override
    public void process(Bundle savedInstanceState) {

    }

    @Override
    public void resume() {

    }

    @Override
    public void pause() {

    }

    @Override
    public void destroy() {

    }

    @Override
    public void playLastVideo(Context context, String videoPath) {
        DataBaseManager.getInstance()
                .selectTable("file")
                .query()
                .queryColumns( "current_position", "episode_id", "zimu_path")
                .where("file_path", videoPath)
                .postExecute(new QueryAsyncResultCallback<VideoBean>(getLifecycle()) {
                    @Override
                    public VideoBean onQuery(Cursor cursor) {
                        if (cursor == null) return null;
                        VideoBean videoBean = null;
                        if (cursor.moveToNext()) {
                            videoBean = new VideoBean();
                            videoBean.setVideoPath(videoPath);
                            videoBean.setCurrentPosition(cursor.getInt(0));
                            videoBean.setEpisodeId(cursor.getInt(1));
                            videoBean.setZimuPath(cursor.getString(2));
                        }
                        return videoBean;
                    }

                    @Override
                    public void onResult(VideoBean videoBean) {
                        if (videoBean == null)
                            return;

                        File videoFile = new File(videoBean.getVideoPath());
                        if (!videoFile.exists())
                            return;

                        if (!StringUtils.isEmpty(videoBean.getZimuPath())) {
                            File zimuFile = new File(videoBean.getZimuPath());
                            if (!zimuFile.exists())
                                videoBean.setZimuPath("");
                        }

                        PlayerManagerActivity.launchPlayerLocal(
                                context,
                                FileUtils.getFileNameNoExtension(videoBean.getVideoPath()),
                                videoBean.getVideoPath(),
                                videoBean.getZimuPath(),
                                videoBean.getCurrentPosition(),
                                videoBean.getEpisodeId());
                    }
                });
    }


    @Override
    public void refreshVideo(Context context, boolean reScan) {
        KANTVLog.j(TAG, "refreshVideo with reScane:" + reScan);
        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        intent.setData(Uri.fromFile(Environment.getExternalStorageDirectory()));
        if (context != null)
            context.sendBroadcast(intent);

        if (reScan) {
            refreshAllVideo();
        } else {
            refreshDatabaseVideo();
        }
    }

    @Override
    public void filterFolder(String folderPath) {
        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .insert()
                .param("folder_path", folderPath)
                .param("folder_type", Constants.ScanType.BLOCK)
                .postExecute();
    }

    @Override
    public void deleteFolderVideo(String folderPath) {
        File folder = new File(folderPath);
        if (folder.exists() && folder.isDirectory()) {
            for (File file : folder.listFiles()) {
                if (CommonUtils.isMediaFile(file.getAbsolutePath())) {
                    file.delete();
                }
            }
        }
        getView().deleteFolderSuccess();
    }


    private void refreshAllVideo() {
        KANTVLog.j(TAG, "enter refreshAllVideo");
        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .query()
                .queryColumns("folder_path")
                .where("folder_type", String.valueOf(Constants.ScanType.SCAN))
                .postExecute(new QueryAsyncResultCallback<List<FolderBean>>(getLifecycle()) {
                    @Override
                    public List<FolderBean> onQuery(Cursor cursor) {
                        KANTVLog.j(TAG, "enter onQuery******************************************");
                        if (cursor == null) {
                            KANTVLog.j(TAG, "cursor is null, return empty list");
                            return new ArrayList<>();
                        }
                        List<String> scanFolderList = new ArrayList<>();
                        while (cursor.moveToNext()) {
                            scanFolderList.add(cursor.getString(0));
                        }

                        KANTVLog.j(TAG, "scanFolerList.size()=" + scanFolderList.size());
                        for (int i = 0; i < scanFolderList.size(); i++) {
                            KANTVLog.j(TAG, "scanFolderList[" + i + "]=" + scanFolderList.get(i));
                        }

                        scanFolderList.add(KANTVUtils.getDataPath());
                        boolean isScanMediaStore = scanFolderList.remove(Constants.DefaultConfig.SYSTEM_VIDEO_PATH);
                        KANTVLog.j(TAG, "isScanMediaStore=" + isScanMediaStore);
                        if (isScanMediaStore)
                        {
                            queryVideoFromMediaStore();
                        }
                        KANTVLog.j(TAG, "scanFolerList.size()=" + scanFolderList.size());
                        for (int i = 0; i < scanFolderList.size(); i++) {
                            KANTVLog.j(TAG, "scanFolderList[" + i + "]=" + scanFolderList.get(i));
                        }


                        boolean bUsingSpecialCode = false;
                        List<String> myBlockList = new ArrayList<>();
                        if (!bUsingSpecialCode) {
                            queryVideoFromStorage(scanFolderList, myBlockList);
                        } else {
                            scanFolderList.add("/sdcard");
                            myBlockList.add("Camera");
                            myBlockList.add("Pictures");
                            myBlockList.add("VideoEditor");
                            myBlockList.add("WeiXin");
                            queryVideoFromStorage(scanFolderList, myBlockList);
                        }

                        return DataBaseManager.getInstance()
                                .selectTable("scan_folder")
                                .query()
                                .queryColumns("folder_path", "folder_type")
                                .executeAsync(scanTypeCursor -> {
                                    if (scanTypeCursor == null) {
                                        KANTVLog.j(TAG, "scan type sucrsor is null, return empty list");
                                        return new ArrayList<>();
                                    }

                                    List<String> scanList = new ArrayList<>();
                                    List<String> blockList = new ArrayList<>();
                                    for (int i = 0; i < scanFolderList.size(); i++) {
                                        KANTVLog.j(TAG, "scanFolderList[" + i + "]=" + scanFolderList.get(i));
                                    }

                                    for (int i = 0; i < blockList.size(); i++) {
                                        KANTVLog.j(TAG, "blockList[" + i + "]=" + blockList.get(i));
                                    }

                                    while (scanTypeCursor.moveToNext()) {
                                        if (Constants.ScanType.SCAN == scanTypeCursor.getInt(1)) {
                                            scanList.add(scanTypeCursor.getString(0));
                                        } else {
                                            blockList.add(scanTypeCursor.getString(0));
                                        }
                                    }
                                    return getVideoFormDatabase(scanList, blockList);
                                });
                    }

                    @Override
                    public void onResult(List<FolderBean> result) {
                        getView().refreshAdapter(result);
                    }
                });
    }


    private void refreshDatabaseVideo() {
        KANTVLog.j(TAG, "enter refreshDatabaseVideo");
        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .query()
                .queryColumns("folder_path", "folder_type")
                .postExecute(new QueryAsyncResultCallback<List<FolderBean>>(getLifecycle()) {

                    @Override
                    public List<FolderBean> onQuery(Cursor cursor) {
                        if (cursor == null)
                            return new ArrayList<>();
                        List<String> scanList = new ArrayList<>();
                        List<String> blockList = new ArrayList<>();

                        while (cursor.moveToNext()) {
                            if (Constants.ScanType.SCAN == cursor.getInt(1)) {
                                scanList.add(cursor.getString(0));
                            } else {
                                blockList.add(cursor.getString(0));
                            }
                        }
                        for (int i = 0; i < scanList.size(); i++) {
                            KANTVLog.j(TAG, "scanList[" + i + "]=" + scanList.get(i));
                        }

                        for (int i = 0; i < blockList.size(); i++) {
                            KANTVLog.j(TAG, "blockList[" + i + "]=" + blockList.get(i));
                        }
                        return getVideoFormDatabase(scanList, blockList);
                    }

                    @Override
                    public void onResult(List<FolderBean> result) {
                        getView().refreshAdapter(result);
                    }
                });
    }


    private List<FolderBean> getVideoFormDatabase(List<String> scanList, List<String> blockList) {
        List<FolderBean> folderBeanList = new ArrayList<>();
        Map<String, Integer> beanMap = new HashMap<>();
        Map<String, String> deleteMap = new HashMap<>();

        DataBaseManager.getInstance()
                .selectTable("file")
                .query()
                .queryColumns("folder_path", "file_path")
                .executeAsync(cursor -> {
                    while (cursor.moveToNext()) {
                        String folderPath = cursor.getString(0);
                        String filePath = cursor.getString(1);

                        boolean isBlock = false;
                        for (String blockPath : blockList) {
                            if (filePath.startsWith(blockPath)) {
                                isBlock = true;
                                break;
                            }
                        }
                        if (isBlock) continue;

                        if (!scanList.contains(Constants.DefaultConfig.SYSTEM_VIDEO_PATH)) {
                            boolean isNotScan = true;
                            for (String scanPath : scanList) {
                                if (filePath.startsWith(scanPath)) {
                                    isNotScan = false;
                                    break;
                                }
                            }
                            if (isNotScan) continue;
                        }

                        File file = new File(filePath);
                        if (file.exists()) {
                            if (beanMap.containsKey(folderPath)) {
                                Integer number = beanMap.get(folderPath);
                                number = number == null ? 0 : number;
                                beanMap.put(folderPath, ++number);
                            } else {
                                beanMap.put(folderPath, 1);
                            }
                        } else {
                            deleteMap.put(folderPath, filePath);
                        }
                    }
                });

        for (Map.Entry<String, Integer> entry : beanMap.entrySet()) {
            folderBeanList.add(new FolderBean(entry.getKey(), entry.getValue()));
            DataBaseManager.getInstance()
                    .selectTable("folder")
                    .update()
                    .param("file_number", entry.getValue())
                    .where("folder_path", entry.getKey())
                    .executeAsync();
        }


        for (Map.Entry<String, String> entry : deleteMap.entrySet()) {
            DataBaseManager.getInstance()
                    .selectTable("file")
                    .delete()
                    .where("folder_path", entry.getKey())
                    .where("file_path", entry.getValue())
                    .executeAsync();
        }
        return folderBeanList;
    }


    private void saveVideoToDatabase(VideoBean videoBean) {
        String folderPath = FileUtils.getDirName(videoBean.getVideoPath());
        ContentValues values = new ContentValues();

        values.put(DataBaseInfo.getFieldNames()[1][1], folderPath);
        values.put(DataBaseInfo.getFieldNames()[1][2], videoBean.getVideoPath());
        values.put(DataBaseInfo.getFieldNames()[1][5], String.valueOf(videoBean.getVideoDuration()));
        values.put(DataBaseInfo.getFieldNames()[1][7], String.valueOf(videoBean.getVideoSize()));
        values.put(DataBaseInfo.getFieldNames()[1][8], videoBean.get_id());
        /*
        KANTVLog.j(TAG, "folderPath:" + folderPath);
        KANTVLog.j(TAG, "video path:" + videoBean.getVideoPath());
        KANTVLog.j(TAG, "video size:" + videoBean.getVideoSize());
        KANTVLog.j(TAG, "video duration:" + videoBean.getVideoDuration());
        KANTVLog.j(TAG, "video id:" + videoBean.get_id());
         */

        DataBaseManager.getInstance()
                .selectTable("file")
                .query()
                .where("folder_path", folderPath)
                .where("file_path", videoBean.getVideoPath())
                .executeAsync(cursor -> {
                    if (!cursor.moveToNext()) {
                        DataBaseManager.getInstance()
                                .selectTable("file")
                                .insert()
                                .param("folder_path", folderPath)
                                .param("file_path", videoBean.getVideoPath())
                                .param("duration", String.valueOf(videoBean.getVideoDuration()))
                                .param("file_size", String.valueOf(videoBean.getVideoSize()))
                                .param("file_id", videoBean.get_id())
                                .executeAsync();
                    } else {
                        KANTVLog.d(TAG, "can't save to db");
                    }
                });
    }


    private void queryVideoFromMediaStore() {
        KANTVLog.j(TAG, "enter queryVideoFromMediaStore");
        Cursor cursor = IApplication.get_context().getContentResolver().query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                null, null, null, null);
        if (cursor != null) {
            while (cursor.moveToNext()) {
                KANTVLog.j(TAG, "ok .got video info from MediaStore");
                String path = cursor.getString(cursor.getColumnIndex(MediaStore.Video.Media.DATA));
                int _id = cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID));
                long size = cursor.getLong(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.SIZE));
                long duration = cursor.getLong(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DURATION));

                VideoBean videoBean = new VideoBean();
                videoBean.set_id(_id);
                videoBean.setVideoPath(path);
                videoBean.setVideoDuration(duration);
                videoBean.setVideoSize(size);
                saveVideoToDatabase(videoBean);
            }
            cursor.close();
        } else {
            KANTVLog.j(TAG, "cursor is null, it shouldn't happen");
        }
    }


    private void queryVideoFromStorage(List<String> scanFolderList, List<String> bockFolderList) {
        KANTVLog.j(TAG, "scanFolerList.size()=" + scanFolderList.size());
        if (scanFolderList.size() == 0)
            return;
        File[] fileArray = new File[scanFolderList.size()];
        for (int i = 0; i < scanFolderList.size(); i++) {
            KANTVLog.j(TAG, "scanFolderList[" + i + "]=" + scanFolderList.get(i));
            fileArray[i] = new File(scanFolderList.get(i));
            if (fileArray[i] == null) {
                KANTVLog.j(TAG, "fileArray[" + i + "] is null");
            } else {
                KANTVLog.j(TAG, "fileArray[" + i + "] is not null");
            }
        }
        for (int i = 0; i < bockFolderList.size(); i++) {
            KANTVLog.j(TAG, "bockFolderList[" + i + "]=" + bockFolderList.get(i));
        }
        for (File parentFile : fileArray) {
            //KANTVLog.j(TAG, "parentFile path:" + parentFile.getAbsolutePath());
            for (File childFile : listFiles(parentFile, bockFolderList)) {
                String filePath = childFile.getAbsolutePath();
                //KANTVLog.j(TAG, "filePath=" + filePath);
                VideoBean videoBean = new VideoBean();
                videoBean.setVideoPath(filePath);
                videoBean.setVideoDuration(0);
                videoBean.setVideoSize(childFile.length());
                videoBean.set_id(0);
                saveVideoToDatabase(videoBean);
            }
        }
    }


    private List<File> listFiles(File file, List<String> bockFolderList) {
        List<File> fileList = new ArrayList<>();
        if (file.isDirectory()) {
            //KANTVLog.j(TAG, "file is directory:" + file.getAbsolutePath());
            String filePath = file.getAbsolutePath();
            for (int i = 0; i < bockFolderList.size(); i++) {
                if (filePath.contains(bockFolderList.get(i))) {
                    KANTVLog.j(TAG, "skip direroty:" + filePath);
                    return new ArrayList<>();
                }
            }
            File[] fileArray = file.listFiles();
            if (fileArray == null || fileArray.length == 0) {
                return new ArrayList<>();
            } else {
                for (File childFile : fileArray) {
                    if (childFile.isDirectory()) {
                        fileList.addAll(listFiles(childFile, bockFolderList));
                    } else if (childFile.exists() && childFile.canRead() && KANTVUtils.isMediaFile(childFile.getAbsolutePath())) {
                        fileList.add(childFile);
                    }
                }
            }
        } else if (file.exists() && file.canRead() && KANTVUtils.isMediaFile(file.getAbsolutePath())) {
            fileList.add(file);
        }
        return fileList;
    }
}