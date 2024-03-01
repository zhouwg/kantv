package com.cdeos.kantv.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.LogUtils;
import com.cdeos.kantv.app.IApplication;
import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.MainPresenter;
import com.cdeos.kantv.mvp.view.MainView;
import com.cdeos.kantv.utils.Constants;
import com.cdeos.kantv.utils.database.DataBaseManager;
import com.cdeos.kantv.utils.net.NetworkConsumer;

import java.io.File;

import cdeos.media.player.CDELog;


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
        CDELog.j(TAG, "******** init scan folder ******\n");

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
        CDELog.j(TAG, "******************************************\n");
    }
}
