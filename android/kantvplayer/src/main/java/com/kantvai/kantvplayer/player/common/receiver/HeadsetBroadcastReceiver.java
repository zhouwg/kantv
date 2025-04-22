package com.kantvai.kantvplayer.player.common.receiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;


public class HeadsetBroadcastReceiver extends BroadcastReceiver {

    private PlayerReceiverListener listener;

    public HeadsetBroadcastReceiver(PlayerReceiverListener listener) {
        this.listener = listener;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction())) {
            listener.onHeadsetRemoved();
        }
    }
}
