package com.cdeos.kantv.mvp.presenter;

import android.content.Context;

import com.cdeos.kantv.utils.interf.presenter.BaseMvpPresenter;


public interface LocalMediaFragmentPresenter extends BaseMvpPresenter {

    void refreshVideo(Context context, boolean reScan);

    void filterFolder(String folderPath);

    void deleteFolderVideo(String folderPath);

    void playLastVideo(Context context, String videoPath);
}
