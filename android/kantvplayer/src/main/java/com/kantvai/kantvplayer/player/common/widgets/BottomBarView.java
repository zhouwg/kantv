package com.kantvai.kantvplayer.player.common.widgets;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.Nullable;

import com.kantvai.kantvplayer.R;


public class BottomBarView extends LinearLayout implements View.OnClickListener{
    private ImageView mIvPlay;

    private TextView mTvCurTime;

    private TextView mTvEndTime;

    private BottomBarListener listener;

    public BottomBarView(Context context) {
        this(context, null);
    }

    public BottomBarView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        View.inflate(context, R.layout.layout_bottom_bar_v2, this);

        mIvPlay = findViewById(R.id.iv_play);
        mTvCurTime = findViewById(R.id.tv_cur_time);
        mTvEndTime = findViewById(R.id.tv_end_time);

        mIvPlay.setOnClickListener(this);

    }

    @Override
    public void onClick(View v) {
        if (listener != null){
            int id = v.getId();
            if (id == R.id.iv_play){
                listener.onPlayClick();
            }
        }
    }

    public void setPlayIvStatus(boolean isPlaying) {
        mIvPlay.setSelected(isPlaying);
    }

    public void setCurTime(String curTime){
        mTvCurTime.setText(curTime);
    }

    public void setEndTime(String endTime){
        mTvEndTime.setText(endTime);
    }

    public void setSeekMax(int max){

    }

    public void setSeekProgress(int progress){

    }

    public void setSeekSecondaryProgress(int progress){

    }

    public void setSeekCallBack(SeekBar.OnSeekBarChangeListener seekCallBack){

    }

    @SuppressLint("ClickableViewAccessibility")
    public void setSeekBarTouchCallBack(OnTouchListener touchListener){

    }

    public void setCallBack(BottomBarListener listener){
        this.listener = listener;
    }

    public interface BottomBarListener{
        void onPlayClick();
    }
}
