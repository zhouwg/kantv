package com.cdeos.kantv.mvp.presenter;

import com.cdeos.kantv.utils.interf.presenter.BaseMvpPresenter;

public interface VideoScanFragmentPresenter extends BaseMvpPresenter {
    void addScanFolder(String path, boolean isScan);

    void queryScanFolderList(boolean isScan);

    void deleteScanFolder(String path, boolean isScan);
}
