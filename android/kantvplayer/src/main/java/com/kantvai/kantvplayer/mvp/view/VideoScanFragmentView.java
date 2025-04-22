package com.kantvai.kantvplayer.mvp.view;

import com.kantvai.kantvplayer.bean.ScanFolderBean;
import com.kantvai.kantvplayer.utils.interf.view.BaseMvpView;

import java.util.List;

public interface VideoScanFragmentView extends BaseMvpView{
    void updateFolderList(List<ScanFolderBean> folderList);
}
