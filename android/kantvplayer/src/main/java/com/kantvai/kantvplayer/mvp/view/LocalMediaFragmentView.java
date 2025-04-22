package com.kantvai.kantvplayer.mvp.view;

import android.content.Context;

import com.kantvai.kantvplayer.bean.FolderBean;
import com.kantvai.kantvplayer.utils.interf.view.BaseMvpView;
import com.kantvai.kantvplayer.utils.interf.view.LoadDataView;

import java.util.List;


public interface LocalMediaFragmentView extends BaseMvpView, LoadDataView {
    void refreshAdapter(List<FolderBean> beans);

    Context getContext();

    void deleteFolderSuccess();
}
