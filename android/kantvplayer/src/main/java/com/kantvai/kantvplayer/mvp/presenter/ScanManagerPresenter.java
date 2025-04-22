package com.kantvai.kantvplayer.mvp.presenter;

import com.kantvai.kantvplayer.utils.interf.presenter.BaseMvpPresenter;

import java.util.List;


public interface ScanManagerPresenter extends BaseMvpPresenter {

    void saveNewVideo(List<String> videoPath);

    void listFolder(String path);
}
