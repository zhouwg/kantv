package com.kantvai.kantvplayer.mvp.impl;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import androidx.lifecycle.LifecycleOwner;

import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.AIAgentPresenter;
import com.kantvai.kantvplayer.mvp.view.AIAgentView;


import kantvai.media.player.KANTVLog;


public class AIAgentPresenterImpl extends BaseMvpPresenterImpl<AIAgentView> implements AIAgentPresenter {

    private static final String TAG = AIAgentPresenterImpl.class.getName();

    public AIAgentPresenterImpl(AIAgentView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {
        KANTVLog.j(TAG, "init");
    }

    @Override
    public void process(Bundle savedInstanceState) {
        KANTVLog.j(TAG, "process");
    }

    @Override
    public void resume() {
        KANTVLog.j(TAG, "resume");
    }

    @Override
    public void pause() {
        KANTVLog.j(TAG, "pause");
    }

    @Override
    public void destroy() {
        KANTVLog.j(TAG, "destroy");
    }


    @Override
    public void refreshVideo(Context context, boolean reScan) {

    }
}