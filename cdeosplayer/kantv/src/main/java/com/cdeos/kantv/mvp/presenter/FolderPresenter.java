package com.cdeos.kantv.mvp.presenter;

import com.cdeos.kantv.bean.VideoBean;
import com.cdeos.kantv.utils.interf.presenter.BaseMvpPresenter;

import java.util.List;

public interface FolderPresenter extends BaseMvpPresenter {
    void getVideoList(String folderPath);
}
