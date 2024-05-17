package com.cdeos.kantv.mvp.view;

import android.content.Context;

import com.cdeos.kantv.bean.FolderBean;
import com.cdeos.kantv.utils.interf.view.BaseMvpView;
import com.cdeos.kantv.utils.interf.view.LoadDataView;

import java.util.List;


public interface LocalMediaFragmentView extends BaseMvpView, LoadDataView {
    void refreshAdapter(List<FolderBean> beans);

    Context getContext();

    void deleteFolderSuccess();
}
