package com.cdeos.kantv.mvp.impl;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.AIAgentPresenter;
import com.cdeos.kantv.mvp.view.AIAgentView;


import cdeos.media.player.CDELog;


public class AIAgentPresenterImpl extends BaseMvpPresenterImpl<AIAgentView> implements AIAgentPresenter {

    private static final String TAG = AIAgentPresenterImpl.class.getName();

    public AIAgentPresenterImpl(AIAgentView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {
        CDELog.j(TAG, "init");
    }

    @Override
    public void process(Bundle savedInstanceState) {
        CDELog.j(TAG, "process");
    }

    @Override
    public void resume() {
        CDELog.j(TAG, "resume");
    }

    @Override
    public void pause() {
        CDELog.j(TAG, "pause");
    }

    @Override
    public void destroy() {
        CDELog.j(TAG, "destroy");
    }


    @Override
    public void refreshVideo(Context context, boolean reScan) {

    }
}