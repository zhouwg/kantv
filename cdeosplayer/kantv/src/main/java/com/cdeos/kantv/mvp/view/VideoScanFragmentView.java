package com.cdeos.kantv.mvp.view;

import com.cdeos.kantv.bean.ScanFolderBean;
import com.cdeos.kantv.utils.interf.view.BaseMvpView;

import java.util.List;

public interface VideoScanFragmentView extends BaseMvpView{
    void updateFolderList(List<ScanFolderBean> folderList);
}
