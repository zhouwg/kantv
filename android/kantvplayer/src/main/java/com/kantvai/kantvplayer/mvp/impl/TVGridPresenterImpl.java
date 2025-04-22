package com.kantvai.kantvplayer.mvp.impl;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import androidx.lifecycle.LifecycleOwner;

import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.TVGridPresenter;
import com.kantvai.kantvplayer.mvp.view.TVGridView;

import org.greenrobot.eventbus.EventBus;

import kantvai.media.player.KANTVLog;

public class TVGridPresenterImpl extends BaseMvpPresenterImpl<TVGridView> implements TVGridPresenter {
    private static final String TAG = TVGridPresenterImpl.class.getName();

    public TVGridPresenterImpl(TVGridView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {
        KANTVLog.d(TAG, "init");
    }

    @Override
    public void process(Bundle savedInstanceState) {
        KANTVLog.d(TAG, "process");
    }

    @Override
    public void resume() {
        KANTVLog.d(TAG, "resume");
    }

    @Override
    public void pause() {
        KANTVLog.d(TAG, "pause");
    }

    @Override
    public void destroy() {
        KANTVLog.d(TAG, "destroy");
    }


    @Override
    public void refreshVideo(Context context, boolean reScan) {
        KANTVLog.d(TAG, "refreshVideo");
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