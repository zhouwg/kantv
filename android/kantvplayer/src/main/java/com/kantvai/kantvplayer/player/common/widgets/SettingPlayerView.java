package com.kantvai.kantvplayer.player.common.widgets;

import android.annotation.SuppressLint;
import android.content.Context;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.content.pm.ActivityInfo;
import android.util.AttributeSet;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.Switch;
import android.widget.TextView;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.player.common.adapter.StreamAdapter;

import com.kantvai.kantvplayer.player.common.utils.CommonPlayerUtils;
import com.kantvai.kantvplayer.player.ffplayer.FFPlayerView;
import com.kantvai.kantvplayer.player.ffplayer.media.IRenderView;

import java.util.ArrayList;
import java.util.List;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.bean.TrackInfoBean;

public class SettingPlayerView extends LinearLayout implements View.OnClickListener {
    private final static String TAG = SettingPlayerView.class.getName();
    private LinearLayout speedCtrlLL;
    private TextView speed50Tv, speed75Tv, speed100Tv, speed125Tv, speed150Tv, speed200Tv;

    private RecyclerView audioRv;
    private Switch orientationChangeSw;

    private Switch tvRecordingSw;
    private Switch landscapeChangeSw;
    private Switch tvASRSw;

    private LinearLayout audioRl;
    private RadioGroup mAspectRatioOptions;

    private StreamAdapter audioStreamAdapter;
    private boolean isExoPlayer = false;
    private boolean isAllowScreenOrientation = true;

    private List<TrackInfoBean> audioTrackList = new ArrayList<>();
    private SettingVideoListener listener;

    public SettingPlayerView(Context context) {
        this(context, null);
    }

    @SuppressLint("ClickableViewAccessibility")
    public SettingPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        View.inflate(context, R.layout.view_setting_video, this);

        speedCtrlLL = this.findViewById(R.id.speed_ctrl_ll);
        speed50Tv = this.findViewById(R.id.speed50_tv);
        speed75Tv = this.findViewById(R.id.speed75_tv);
        speed100Tv = this.findViewById(R.id.speed100_tv);
        speed125Tv = this.findViewById(R.id.speed125_tv);
        speed150Tv = this.findViewById(R.id.speed150_tv);
        speed200Tv = this.findViewById(R.id.speed200_tv);
        audioRv = this.findViewById(R.id.audio_track_rv);
        audioRl = this.findViewById(R.id.audio_track_ll);
        mAspectRatioOptions = this.findViewById(R.id.aspect_ratio_group);
        orientationChangeSw = this.findViewById(R.id.orientation_change_sw);

        landscapeChangeSw = this.findViewById(R.id.landscape_change_sw);
        tvRecordingSw     = this.findViewById(R.id.tv_recording_sw);
        tvASRSw           = this.findViewById(R.id.tv_asr_sw);

        mAspectRatioOptions.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.aspect_fit_parent) {
                listener.setAspectRatio(IRenderView.AR_ASPECT_FIT_PARENT);
            } else if (checkedId == R.id.aspect_fit_screen) {
                listener.setAspectRatio(IRenderView.AR_ASPECT_FILL_PARENT);
            } else if (checkedId == R.id.aspect_16_and_9) {
                listener.setAspectRatio(IRenderView.AR_16_9_FIT_PARENT);
            } else if (checkedId == R.id.aspect_4_and_3) {
                listener.setAspectRatio(IRenderView.AR_4_3_FIT_PARENT);
            }
        });

        speed50Tv.setOnClickListener(this);
        speed75Tv.setOnClickListener(this);
        speed100Tv.setOnClickListener(this);
        speed125Tv.setOnClickListener(this);
        speed150Tv.setOnClickListener(this);
        speed200Tv.setOnClickListener(this);

        audioStreamAdapter = new StreamAdapter(R.layout.item_video_track, audioTrackList);
        audioRv.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
        audioRv.setItemViewCacheSize(10);
        audioRv.setAdapter(audioStreamAdapter);

        audioStreamAdapter.setOnItemChildClickListener((adapter, view, position) -> {
            if (isExoPlayer) {
                for (int i = 0; i < audioTrackList.size(); i++) {
                    if (i == position)
                        audioTrackList.get(i).setSelect(true);
                    else
                        audioTrackList.get(i).setSelect(false);
                }
                listener.selectTrack(audioTrackList.get(position), true);
            } else {
                //deselectAll except position
                for (int i = 0; i < audioTrackList.size(); i++) {
                    if (i == position) continue;
                    listener.deselectTrack(audioTrackList.get(i), true);
                    audioTrackList.get(i).setSelect(false);
                }
                //select or deselect position
                if (audioTrackList.get(position).isSelect()) {
                    listener.deselectTrack(audioTrackList.get(position), true);
                    audioTrackList.get(position).setSelect(false);
                } else {
                    listener.selectTrack(audioTrackList.get(position), true);
                    audioTrackList.get(position).setSelect(true);
                }
            }
            audioStreamAdapter.notifyDataSetChanged();
        });

        this.setOnTouchListener((v, event) -> true);



        orientationChangeSw.setOnCheckedChangeListener((buttonView, isChecked) -> {
            isAllowScreenOrientation = isChecked;
            listener.setOrientationStatus(isChecked);
            KANTVLog.d(TAG, "isAllowScreenOrientation:" + isAllowScreenOrientation);
        });


        landscapeChangeSw.setOnCheckedChangeListener((buttonView, isChecked) -> {
           listener.setLandscapeStatus(isChecked);
           KANTVLog.d(TAG, "landscape changed:" + isChecked);
        });

        tvRecordingSw.setOnCheckedChangeListener((buttonView, isChecked) -> {
            KANTVLog.d(TAG, "tv recording:" + isChecked);
            listener.setTVRecordingStatus(isChecked);
        });
        KANTVLog.d(TAG, "isRecording:" + KANTVUtils.getTVRecording());
        //if (KANTVUtils.getTVRecording())
        //    tvRecordingSw.setChecked(true);
        //else
         //   tvRecordingSw.setChecked(false);
        tvASRSw.setOnCheckedChangeListener((buttonView, isChecked) -> {
            KANTVLog.d(TAG, "tv ASR status:" + isChecked);
            listener.setTVASRStatus(isChecked);
        });
        KANTVLog.d(TAG, "isASR enabled:" + KANTVUtils.getTVASR());
        setPlayerSpeedView(3);
    }

    public void updateUIStatus(boolean isRecording) {
        KANTVLog.d(TAG, "isRecording:" + isRecording);
        KANTVLog.d(TAG, "isRecording:" + KANTVUtils.getTVRecording());
        if (KANTVUtils.getTVRecording())
            tvRecordingSw.setChecked(true);
        else
            tvRecordingSw.setChecked(false);

    }

    public void updateASRUIStatus(boolean isASR) {
        if (isASR)
            tvASRSw.setChecked(true);
        else
            tvASRSw.setChecked(false);
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        if (id == R.id.speed50_tv) {
            listener.setSpeed(0.5f);
            setPlayerSpeedView(1);
        } else if (id == R.id.speed75_tv) {
            listener.setSpeed(0.75f);
            setPlayerSpeedView(2);
        } else if (id == R.id.speed100_tv) {
            listener.setSpeed(1.0f);
            setPlayerSpeedView(3);
        } else if (id == R.id.speed125_tv) {
            listener.setSpeed(1.25f);
            setPlayerSpeedView(4);
        } else if (id == R.id.speed150_tv) {
            listener.setSpeed(1.5f);
            setPlayerSpeedView(5);
        } else if (id == R.id.speed200_tv) {
            listener.setSpeed(2.0f);
            setPlayerSpeedView(6);
        }
    }

    public SettingPlayerView setExoPlayerType() {
        this.isExoPlayer = true;
        return this;
    }

    public void setOrientationAllow(boolean isAllow) {
        isAllowScreenOrientation = isAllow;
        orientationChangeSw.setChecked(isAllow);
    }

    public SettingPlayerView setSettingListener(SettingVideoListener listener) {
        this.listener = listener;
        return this;
    }

    public void setAudioTrackList(List<TrackInfoBean> audioTrackList) {
        this.audioTrackList.clear();
        this.audioTrackList.addAll(audioTrackList);
        this.audioStreamAdapter.setNewData(audioTrackList);
        this.audioRl.setVisibility(audioTrackList.size() < 1 ? GONE : VISIBLE);
    }

    public void setSpeedCtrlLLVis(boolean visibility) {
        speedCtrlLL.setVisibility(visibility ? VISIBLE : GONE);
    }

    public void setOrientationChangeEnable(boolean isEnable) {
        orientationChangeSw.setChecked(isEnable);
    }

    public void setPlayerSpeedView(int type) {
        switch (type) {
            case 1:
                speed50Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                speed75Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed100Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed125Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed150Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed200Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                break;
            case 2:
                speed50Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed75Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                speed100Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed125Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed150Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed200Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                break;
            case 3:
                speed50Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed75Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed100Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                speed125Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed150Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed200Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                break;
            case 4:
                speed50Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed75Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed100Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed125Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                speed150Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed200Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                break;
            case 5:
                speed50Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed75Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed100Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed125Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed150Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                speed200Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                break;
            case 6:
                speed50Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed75Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed100Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed125Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed150Tv.setBackground(ContextCompat.getDrawable(getContext(), R.drawable.sel_item_background));
                speed200Tv.setBackgroundColor(CommonPlayerUtils.getResColor(getContext(), R.color.selected_view_bg));
                break;
        }
    }

    public boolean isAllowScreenOrientation() {
        return isAllowScreenOrientation;
    }

    public interface SettingVideoListener {
        void selectTrack(TrackInfoBean trackInfoBean, boolean isAudio);

        void deselectTrack(TrackInfoBean trackInfoBean, boolean isAudio);

        void setSpeed(float speed);

        void setAspectRatio(int type);

        void setOrientationStatus(boolean isEnable);

        void setLandscapeStatus(boolean bLandScape);

        void setTVRecordingStatus(boolean bRecording);

        void setTVASRStatus(boolean bEnableASR);
    }
}
