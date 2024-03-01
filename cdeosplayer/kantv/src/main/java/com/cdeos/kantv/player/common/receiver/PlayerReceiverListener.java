package com.cdeos.kantv.player.common.receiver;


public interface PlayerReceiverListener{
    void onBatteryChanged(int status, int progress);

    void onScreenLocked();

    void onHeadsetRemoved();
}