package com.cdeos.kantv.player.common.widgets;

import android.content.Context;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.cdeos.kantv.R;
import com.cdeos.kantv.player.common.receiver.BatteryBroadcastReceiver;
import com.cdeos.kantv.player.common.utils.AnimHelper;
import com.cdeos.kantv.player.common.utils.TimeFormatUtils;

import java.io.File;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;
import cdeos.media.player.KANTVDRM;

public class TopBarView extends FrameLayout implements View.OnClickListener{
    private static final String TAG = TopBarView.class.getName();
    private LinearLayout topBarLL;
    private ImageView backIv;
    private MarqueeTextView titleTv;
    private ProgressBar batteryBar;
    private TextView systemTimeTv;

    private ImageView tvNetworkIv;
    private ImageView tvRecordingIv;
    private TextView  tvDiskFree;
    private ImageView playerSettingIv;
    private SettingPlayerView playerSettingView;
    private ImageView subtitleSettingIv = null;
    private SettingSubtitleView subtitleSettingView = null;

    private TopBarListener listener;

    public TopBarView(Context context) {
        super(context);
    }

    public TopBarView(Context context, AttributeSet attrs) {
        super(context, attrs);
        View.inflate(context, R.layout.layout_top_bar_v2, this);

        topBarLL = this.findViewById(R.id.top_bar_ll);
        backIv = this.findViewById(R.id.iv_back);
        titleTv = this.findViewById(R.id.tv_title);
        batteryBar = this.findViewById(R.id.pb_battery);
        systemTimeTv = this.findViewById(R.id.tv_system_time);

        tvNetworkIv   = this.findViewById(R.id.tv_network_iv);
        tvRecordingIv = this.findViewById(R.id.tv_recording_iv);
        tvDiskFree    = this.findViewById(R.id.tv_disk_free);

        playerSettingIv = this.findViewById(R.id.player_settings_iv);
        playerSettingView = this.findViewById(R.id.player_setting_view);


        subtitleSettingIv = this.findViewById(R.id.subtitle_settings_iv);
        subtitleSettingView = this.findViewById(R.id.subtitle_setting_view);

        backIv.setOnClickListener(this);
        if (subtitleSettingIv != null) {
            subtitleSettingIv.setOnClickListener(this);
        }

        playerSettingIv.setOnClickListener(this);

        CDELog.j(TAG, "isRecording:" + CDEUtils.getTVRecording());
        if (CDEUtils.getTVRecording()) {
            tvRecordingIv.setVisibility(VISIBLE);
            tvDiskFree.setVisibility(VISIBLE);
            //playerSettingView.updateUIStatus(true);
            updateDiskFree();
        } else {
            tvRecordingIv.setVisibility(INVISIBLE);
            tvDiskFree.setVisibility(INVISIBLE);
            //playerSettingView.updateUIStatus(false);
        }
        if (CDEUtils.getNetworkType() == CDEUtils.NETWORK_MOBILE) {
            tvNetworkIv.setImageResource(R.mipmap.ic_mobile);
        }
        updateSystemTime();
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.iv_back){
            listener.onBack();
        }else if (id == R.id.subtitle_settings_iv){
            listener.topBarItemClick();
            AnimHelper.viewTranslationX(subtitleSettingView, 0);
        }else if (id == R.id.player_settings_iv){
            listener.topBarItemClick();
            AnimHelper.viewTranslationX(playerSettingView, 0);
        }
    }


    public void setCallBack(TopBarListener listener){
        this.listener = listener;
    }


    public void setBatteryChanged(int status, int progress){
        if (status == BatteryBroadcastReceiver.BATTERY_STATUS_SPA) {
            batteryBar.setSecondaryProgress(0);
            batteryBar.setProgress(progress);
            batteryBar.setBackgroundResource(R.mipmap.ic_battery_charging);
        } else if (status == BatteryBroadcastReceiver.BATTERY_STATUS_LOW) {
            batteryBar.setSecondaryProgress(progress);
            batteryBar.setProgress(0);
            batteryBar.setBackgroundResource(R.mipmap.ic_battery_red);
        } else if (status == BatteryBroadcastReceiver.BATTERY_STATUS_NOR){
            batteryBar.setSecondaryProgress(0);
            batteryBar.setProgress(progress);
            batteryBar.setBackgroundResource(R.mipmap.ic_battery);
        }
    }


    public void updateSystemTime(){
        systemTimeTv.setText(TimeFormatUtils.getCurFormatTime());
    }

    public void updateDiskFree() {
        KANTVDRM instance = KANTVDRM.getInstance();
        File sdcardFile = Environment.getExternalStorageDirectory();
        String sdcardPath = sdcardFile.getAbsolutePath();
        CDELog.j(TAG, "sdcard path:" + sdcardPath);
        int diskFreeSize = instance.ANDROID_JNI_GetDiskFreeSize(sdcardPath);
        int diskTotalSize = instance.ANDROID_JNI_GetDiskSize(sdcardPath);
        CDELog.j(TAG, "disk free:" + diskFreeSize + "MB");
        CDELog.j(TAG, "disk total:" + diskTotalSize + "MB");
        tvDiskFree.setText(Integer.toString(diskFreeSize) + "/" + Integer.toString(diskTotalSize) + "M");
    }

    public void setTitleText(String title){
        titleTv.setText(title);
    }


    public void setTopBarVisibility(boolean isVisible){
        if (isVisible){
            AnimHelper.viewTranslationY(topBarLL, 0);
        }else {
            AnimHelper.viewTranslationY(topBarLL, -topBarLL.getHeight());
        }
    }

    public void updateTVRecordingVisibility(boolean isRecording) {
        if (isRecording) {
            tvRecordingIv.setVisibility(VISIBLE);
            tvDiskFree.setVisibility(VISIBLE);
            playerSettingView.updateUIStatus(true);
            updateDiskFree();
        } else {
            tvRecordingIv.setVisibility(INVISIBLE);
            tvDiskFree.setVisibility(INVISIBLE);
            playerSettingView.updateUIStatus(false);
        }
    }

    public void updatePlaySettingVisibility(boolean bShow) {
        if (bShow)
            playerSettingView.setVisibility(VISIBLE);
        else
            playerSettingView.setVisibility(GONE);
    }


    public int getTopBarVisibility(){
        return topBarLL.getVisibility();
    }


    public void hideItemView(){
        if (playerSettingView.getTranslationX() == 0){
            AnimHelper.viewTranslationX(playerSettingView);
        }

        if (subtitleSettingView != null) {
            if (subtitleSettingView.getTranslationX() == 0) {
                AnimHelper.viewTranslationX(subtitleSettingView);
            }
        }
    }

    public boolean isItemShowing() {
        return playerSettingView.getTranslationX() == 0 ||
                subtitleSettingView.getTranslationX() == 0;
    }

    public SettingPlayerView getPlayerSettingView(){
        return playerSettingView;
    }


    public SettingSubtitleView getSubtitleSettingView(){
        return subtitleSettingView;
    }

    public interface TopBarListener{
        void onBack();

        void topBarItemClick();
    }
}
