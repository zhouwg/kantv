package com.kantvai.kantvplayer.mvp.view;

import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.utils.interf.view.BaseMvpView;
import com.kantvai.kantvplayer.utils.interf.view.LoadDataView;

import java.util.List;


public interface FolderView extends BaseMvpView, LoadDataView {

    void refreshAdapter(List<VideoBean> beans);

    void openIntentVideo(VideoBean videoBean);
}
