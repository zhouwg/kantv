package com.kantvai.kantvplayer.mvp.presenter;

import android.content.Context;

import com.kantvai.kantvplayer.utils.interf.presenter.BaseMvpPresenter;


public interface LocalMediaFragmentPresenter extends BaseMvpPresenter {

    void refreshVideo(Context context, boolean reScan);

    void filterFolder(String folderPath);

    void deleteFolderVideo(String folderPath);

    void playLastVideo(Context context, String videoPath);
}
