package com.kantvai.kantvplayer.ui.weight.item;

import android.text.TextUtils;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.bean.LocalPlayHistoryBean;
import com.kantvai.kantvplayer.ui.activities.play.PlayerManagerActivity;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Locale;

import butterknife.BindView;

public class LocalPlayHistoryItem implements AdapterItem<LocalPlayHistoryBean> {

    @BindView(R.id.video_path_tv)
    TextView videoPathTv;
    @BindView(R.id.source_origin_tv)
    TextView sourceOriginTv;
    @BindView(R.id.play_time_tv)
    TextView playTimeTv;
    @BindView(R.id.position_tv)
    TextView positionTv;
    @BindView(R.id.delete_cb)
    CheckBox deleteCb;
    @BindView(R.id.header_click_rl)
    RelativeLayout headerClickRl;
    @BindView(R.id.info_ll)
    LinearLayout infoLl;

    private View view;
    private OnLocalHistoryItemClickListener listener;

    public LocalPlayHistoryItem(OnLocalHistoryItemClickListener listener) {
        this.listener = listener;
    }

    @Override
    public int getLayoutResId() {
        return R.layout.item_local_play_history;
    }

    @Override
    public void initItemViews(View itemView) {
        this.view = itemView;
    }

    @Override
    public void onSetViews() {

    }

    @Override
    public void onUpdateViews(LocalPlayHistoryBean model, int position) {
        positionTv.setText(String.valueOf((position+1)));
        //videoPathTv.setText(CommonUtils.decodeHttpUrl(model.getVideoPath()));
        videoPathTv.setText(CommonUtils.decodeHttpUrl(model.getVideoTitle()));
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
        playTimeTv.setText(dateFormat.format(model.getPlayTime()));

        positionTv.setVisibility(model.isDeleteMode() ? View.GONE : View.VISIBLE);
        deleteCb.setVisibility(model.isDeleteMode() ? View.VISIBLE : View.GONE);
        deleteCb.setChecked(model.isChecked());

        switch (model.getSourceOrigin()) {
            case PlayerManagerActivity.SOURCE_ORIGIN_LOCAL:
                sourceOriginTv.setText("local file");
                break;
            case PlayerManagerActivity.SOURCE_ORIGIN_OUTSIDE:
                sourceOriginTv.setText("external file");
                break;
            case PlayerManagerActivity.SOURCE_ORIGIN_REMOTE:
                sourceOriginTv.setText("remote file");
                break;
            case PlayerManagerActivity.SOURCE_ORIGIN_SMB:
                sourceOriginTv.setText("LAN file");
                break;
            case PlayerManagerActivity.SOURCE_ORIGIN_STREAM:
            case PlayerManagerActivity.SOURCE_ONLINE_PREVIEW:
                sourceOriginTv.setText("online media");
                break;
            default:
                sourceOriginTv.setText("unknown source");
                break;
        }

        infoLl.setOnClickListener(v -> {
            if(model.isDeleteMode()){
                if (listener != null)
                    listener.onCheckedChanged(position);
                return;
            }

            int episodeId = model.getEpisodeId();
            String zimuPath = model.getZimuPath();
            if (TextUtils.isEmpty(zimuPath)){
                zimuPath = "";
            } else {
                File zimuFile = new File(zimuPath);
                if (!zimuFile.exists()) {
                    zimuPath = "";
                }
            }

            PlayerManagerActivity.launchPlayerHistory(
                    view.getContext(),
                    model.getVideoTitle(),
                    model.getVideoPath(),
                    zimuPath,
                    0,
                    episodeId,
                    model.getSourceOrigin());

        });

        infoLl.setOnLongClickListener(v -> {
            if (listener != null)
                return listener.onLongClick(position);
            return false;
        });

        headerClickRl.setOnClickListener(v -> {
            if (listener != null && model.isDeleteMode())
                listener.onCheckedChanged(position);
        });
    }

    public interface OnLocalHistoryItemClickListener {
        boolean onLongClick(int position);

        void onCheckedChanged(int position);
    }
}
