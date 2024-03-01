package com.cdeos.kantv.mvp.presenter;

import com.cdeos.kantv.utils.interf.presenter.BaseMvpPresenter;

import java.util.List;


public interface ScanManagerPresenter extends BaseMvpPresenter {

    void saveNewVideo(List<String> videoPath);

    void listFolder(String path);
}
