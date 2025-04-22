package com.kantvai.kantvplayer.mvp.impl;

import android.annotation.SuppressLint;
import android.database.Cursor;
import android.os.Bundle;
import android.provider.MediaStore;

import androidx.lifecycle.LifecycleOwner;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.bean.event.UpdateFragmentEvent;
import com.kantvai.kantvplayer.mvp.presenter.ScanManagerPresenter;
import com.kantvai.kantvplayer.mvp.view.ScanManagerView;
import com.kantvai.kantvplayer.ui.fragment.LocalMediaFragment;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;

import org.greenrobot.eventbus.EventBus;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


public class ScanManagerPresenterImpl extends BaseMvpPresenterImpl<ScanManagerView> implements ScanManagerPresenter {
    private int newAddCount = 0;

    public ScanManagerPresenterImpl(ScanManagerView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {

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
    public void saveNewVideo(List<String> pathList) {
        newAddCount = 0;
        IApplication.getSqlThreadPool().execute(() -> {
            for (String videoPath : pathList) {
                String folderPath = FileUtils.getDirName(videoPath);
                DataBaseManager.getInstance()
                        .selectTable("file")
                        .query()
                        .where("folder_path", folderPath)
                        .where("file_path", videoPath)
                        .executeAsync(cursor -> {
                            if (!cursor.moveToNext()) {
                                VideoBean videoBean = queryFormSystem(videoPath);
                                DataBaseManager.getInstance()
                                        .selectTable("file")
                                        .insert()
                                        .param("folder_path", folderPath)
                                        .param("file_path", videoBean.getVideoPath())
                                        .param("duration", String.valueOf(videoBean.getVideoDuration()))
                                        .param("file_size", String.valueOf(videoBean.getVideoSize()))
                                        .param("file_id", videoBean.get_id())
                                        .executeAsync();
                                EventBus.getDefault().post(UpdateFragmentEvent.updatePlay(LocalMediaFragment.UPDATE_DATABASE_DATA));
                                newAddCount++;
                            }
                        });
            }
            ToastUtils.showShort("扫描完成，新增：" + newAddCount);
        });
    }

    @SuppressLint("CheckResult")
    @Override
    public void listFolder(String rootFilePath) {
        File rootFile = new File(rootFilePath);
        saveNewVideo(getVideoList(rootFile));
    }


    private List<String> getVideoList(File file) {
        List<String> fileList = new ArrayList<>();
        if (file.isDirectory()) {
            File[] fileArray = file.listFiles();
            if (fileArray == null || fileArray.length == 0) {
                return new ArrayList<>();
            } else {
                for (File childFile : fileArray) {
                    if (childFile.isDirectory()) {
                        fileList.addAll(getVideoList(childFile));
                    } else if (childFile.exists() && childFile.canRead() && CommonUtils.isMediaFile(childFile.getAbsolutePath())) {
                        fileList.add(childFile.getAbsolutePath());
                    }
                }
            }
        } else if (file.exists() && file.canRead() && CommonUtils.isMediaFile(file.getAbsolutePath())) {
            fileList.add(file.getAbsolutePath());
        }
        return fileList;
    }


    private VideoBean queryFormSystem(String path) {
        VideoBean videoBean = new VideoBean();
        Cursor cursor = getView().getContext().getContentResolver().query(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                new String[]{MediaStore.Video.Media._ID, MediaStore.Video.Media.DURATION},
                MediaStore.Video.Media.DATA + " = ?",
                new String[]{path}, null);
        File file = new File(path);
        videoBean.setVideoPath(path);
        videoBean.setVideoSize(file.length());
        if (cursor != null && cursor.moveToNext()) {
            videoBean.setVideoDuration(cursor.getLong(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DURATION)));
            videoBean.set_id(cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID)));
            cursor.close();
        } else {
            if (cursor != null)
                cursor.close();
            videoBean.setVideoDuration(0);
            videoBean.set_id(0);
        }
        return videoBean;
    }
}
