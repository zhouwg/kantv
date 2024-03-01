package com.cdeos.kantv.mvp.view;

import com.cdeos.kantv.bean.VideoBean;
import com.cdeos.kantv.utils.interf.view.BaseMvpView;
import com.cdeos.kantv.utils.interf.view.LoadDataView;

import java.util.List;


public interface FolderView extends BaseMvpView, LoadDataView {

    void refreshAdapter(List<VideoBean> beans);

    void openIntentVideo(VideoBean videoBean);
}
