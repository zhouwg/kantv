package com.kantvai.kantvplayer.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.LogUtils;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.MainPresenter;
import com.kantvai.kantvplayer.mvp.view.MainView;
import com.kantvai.kantvplayer.utils.Constants;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.utils.net.NetworkConsumer;

import java.io.File;

import kantvai.media.player.KANTVLog;


public class MainPresenterImpl extends BaseMvpPresenterImpl<MainView> implements MainPresenter {
    private static final String TAG = MainPresenterImpl.class.getName();
    public MainPresenterImpl(MainView view, LifecycleOwner lifecycleOwner) {
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
    public void initScanFolder() {
        KANTVLog.d(TAG, "******** init scan folder ******\n");

        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .query()
                .postExecute(cursor -> {
                    if (!cursor.moveToNext()) {
                        DataBaseManager.getInstance()
                                .selectTable("scan_folder")
                                .insert()
                                .param("folder_path", Constants.DefaultConfig.SYSTEM_VIDEO_PATH)
                                /*.param("folder_path", "/sdcard")*/
                                .param("folder_type", Constants.ScanType.SCAN)
                                .postExecute();
                    }
                });
        KANTVLog.d(TAG, "******************************************\n");
    }
}
