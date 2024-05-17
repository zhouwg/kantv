package com.cdeos.kantv.mvp.impl;

import android.database.Cursor;
import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.bean.ScanFolderBean;
import com.cdeos.kantv.bean.event.UpdateFragmentEvent;
import com.cdeos.kantv.mvp.presenter.VideoScanFragmentPresenter;
import com.cdeos.kantv.mvp.view.VideoScanFragmentView;
import com.cdeos.kantv.ui.fragment.LocalMediaFragment;
import com.cdeos.kantv.utils.Constants;
import com.cdeos.kantv.utils.database.DataBaseManager;
import com.cdeos.kantv.utils.database.callback.QueryAsyncResultCallback;

import org.greenrobot.eventbus.EventBus;

import java.util.ArrayList;
import java.util.List;


public class VideoScanFragmentPresenterImpl extends BaseMvpPresenterImpl<VideoScanFragmentView> implements VideoScanFragmentPresenter {


    public VideoScanFragmentPresenterImpl(VideoScanFragmentView view, LifecycleOwner lifecycleOwner) {
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
    public void addScanFolder(String path, boolean isScan) {
        int scanType = isScan ? Constants.ScanType.SCAN : Constants.ScanType.BLOCK;
        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .insert()
                .param("folder_path", path)
                .param("folder_type", scanType)
                .postExecute();

        queryScanFolderList(isScan);
        EventBus.getDefault().post(UpdateFragmentEvent.updatePlay(LocalMediaFragment.UPDATE_SYSTEM_DATA));
    }

    @Override
    public void queryScanFolderList(boolean isScan) {
        int scanType = isScan ? Constants.ScanType.SCAN : Constants.ScanType.BLOCK;
        DataBaseManager.getInstance()
                .selectTable("scan_folder")
                .query()
                .where("folder_type", String.valueOf(scanType))
                .postExecute(new QueryAsyncResultCallback<List<ScanFolderBean>>(getLifecycle()) {
                    @Override
                    public List<ScanFolderBean> onQuery(Cursor cursor) {
                        if (cursor == null)
                            return new ArrayList<>();
                        List<ScanFolderBean> folderList = new ArrayList<>();
                        while (cursor.moveToNext()) {
                            folderList.add(new ScanFolderBean(cursor.getString(1), false));
                        }
                        return folderList;
                    }

                    @Override
                    public void onResult(List<ScanFolderBean> result) {
                        getView().updateFolderList(result);
                    }
                });


    }

    @Override
    public void deleteScanFolder(String path, boolean isScan) {
        int scanType = isScan ? Constants.ScanType.SCAN : Constants.ScanType.BLOCK;
        if (Constants.DefaultConfig.SYSTEM_VIDEO_PATH.equals(path)) {
            int newScanType = isScan ? Constants.ScanType.BLOCK : Constants.ScanType.SCAN;
            DataBaseManager.getInstance()
                    .selectTable("scan_folder")
                    .update()
                    .where("folder_path", path)
                    .where("folder_type", String.valueOf(scanType))
                    .param("folder_type", newScanType)
                    .postExecute();
        } else {
            DataBaseManager.getInstance()
                    .selectTable("scan_folder")
                    .delete()
                    .where("folder_path", path)
                    .where("folder_type", String.valueOf(scanType))
                    .postExecute();
        }
        EventBus.getDefault().post(UpdateFragmentEvent.updatePlay(LocalMediaFragment.UPDATE_SYSTEM_DATA));
    }
}
