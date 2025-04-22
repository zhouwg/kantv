package com.kantvai.kantvplayer.player.common.listener;

import android.content.res.Configuration;
import android.view.KeyEvent;


public interface PlayerViewListener {

    void onResume();

    void onPause();

    void onDestroy();

    boolean handleVolumeKey(int keyCode);

    boolean onKeyDown(int keyCode, KeyEvent event);

    boolean onBackPressed();

    void configurationChanged(Configuration configuration);

    void setBatteryChanged(int status, int progress);

    void setSubtitlePath(String subtitlePath);

    void onSubtitleQuery(int size);

    void onScreenLocked();
}
