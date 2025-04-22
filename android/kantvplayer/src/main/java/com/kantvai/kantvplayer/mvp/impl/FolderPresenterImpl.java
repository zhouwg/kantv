package com.kantvai.kantvplayer.mvp.impl;

import android.annotation.SuppressLint;
import android.database.Cursor;
import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.mvp.presenter.FolderPresenter;
import com.kantvai.kantvplayer.mvp.view.FolderView;

import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.utils.database.callback.QueryAsyncResultCallback;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import kantvai.media.player.KANTVLog;

public class FolderPresenterImpl extends BaseMvpPresenterImpl<FolderView> implements FolderPresenter {
    public FolderPresenterImpl(FolderView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }
    private static final String TAG = FolderPresenterImpl.class.getName();

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

    @SuppressLint("CheckResult")
    @Override
    public void getVideoList(String folderPath) {
        KANTVLog.j(TAG, "enter getVideoList with folderPath:" + folderPath);
        DataBaseManager.getInstance()
                .selectTable("file")
                .query()
                .where("folder_path", folderPath)
                .postExecute(new QueryAsyncResultCallback<List<VideoBean>>(getLifecycle()) {
                    @Override
                    public List<VideoBean> onQuery(Cursor cursor) {
                        if (cursor == null) return new ArrayList<>();
                        List<VideoBean> videoBeans = new ArrayList<>();
                        while (cursor.moveToNext()) {
                            String filePath = cursor.getString(2);
                            File file = new File(filePath);
                            if (!file.exists()) {
                                DataBaseManager.getInstance()
                                        .selectTable("file")
                                        .delete()
                                        .where("folder_path", folderPath)
                                        .where("file_path", filePath)
                                        .postExecute();
                                continue;
                            }

                            VideoBean videoBean = new VideoBean();
                            videoBean.setVideoPath(filePath);
                            videoBean.setCurrentPosition(cursor.getInt(3));
                            videoBean.setVideoDuration(Long.parseLong(cursor.getString(4)));
                            videoBean.setEpisodeId(cursor.getInt(5));
                            videoBean.setVideoSize(Long.parseLong(cursor.getString(6)));
                            videoBean.set_id(cursor.getInt(7));
                            videoBean.setZimuPath(cursor.getString(8));
                            videoBeans.add(videoBean);
                        }
                        return videoBeans;
                    }

                    @Override
                    public void onResult(List<VideoBean> result) {
                        getView().refreshAdapter(result);
                        getView().hideLoading();
                    }
                });
    }
}
