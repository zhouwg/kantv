package com.kantvai.kantvplayer.player.common.receiver;


public interface PlayerReceiverListener{
    void onBatteryChanged(int status, int progress);

    void onScreenLocked();

    void onHeadsetRemoved();
}