package com.kantvai.kantvplayer.mvp.impl;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import androidx.lifecycle.LifecycleOwner;

import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.EPGListPresenter;
import com.kantvai.kantvplayer.mvp.view.EPGListView;

import org.greenrobot.eventbus.EventBus;

import kantvai.media.player.KANTVLog;


public class EPGListPresenterImpl extends BaseMvpPresenterImpl<EPGListView> implements EPGListPresenter {

    private static final String TAG = EPGListPresenterImpl.class.getName();

    public EPGListPresenterImpl(EPGListView view, LifecycleOwner lifecycleOwner) {
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