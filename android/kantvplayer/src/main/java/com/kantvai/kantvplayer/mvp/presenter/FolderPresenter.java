package com.kantvai.kantvplayer.mvp.presenter;

import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.utils.interf.presenter.BaseMvpPresenter;

import java.util.List;

public interface FolderPresenter extends BaseMvpPresenter {
    void getVideoList(String folderPath);
}
