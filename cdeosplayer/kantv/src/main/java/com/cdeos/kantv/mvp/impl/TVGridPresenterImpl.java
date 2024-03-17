package com.cdeos.kantv.mvp.impl;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.TVGridPresenter;
import com.cdeos.kantv.mvp.view.TVGridView;

import org.greenrobot.eventbus.EventBus;

import cdeos.media.player.CDELog;

public class TVGridPresenterImpl extends BaseMvpPresenterImpl<TVGridView> implements TVGridPresenter {
    private static final String TAG = TVGridPresenterImpl.class.getName();

    public TVGridPresenterImpl(TVGridView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {
        CDELog.d(TAG, "init");
    }

    @Override
    public void process(Bundle savedInstanceState) {
        CDELog.d(TAG, "process");
    }

    @Override
    public void resume() {
        CDELog.d(TAG, "resume");
    }

    @Override
    public void pause() {
        CDELog.d(TAG, "pause");
    }

    @Override
    public void destroy() {
        CDELog.d(TAG, "destroy");
    }


    @Override
    public void refreshVideo(Context context, boolean reScan) {
        CDELog.d(TAG, "refreshVideo");
        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        intent.setData(Uri.fromFile(Environment.getExternalStorageDirectory()));
        if (context != null)
            context.sendBroadcast(intent);

        if (reScan) {
            //refreshAllVideo();
        } else {
            //refreshDatabaseVideo();
        }
    }
}