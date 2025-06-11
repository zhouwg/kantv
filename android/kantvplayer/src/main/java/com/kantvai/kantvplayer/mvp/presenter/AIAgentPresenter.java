package com.kantvai.kantvplayer.mvp.presenter;

import android.content.Context;

import com.kantvai.kantvplayer.utils.interf.presenter.BaseMvpPresenter;


public interface AIAgentPresenter extends BaseMvpPresenter {
    void refreshVideo(Context context, boolean reScan);
}