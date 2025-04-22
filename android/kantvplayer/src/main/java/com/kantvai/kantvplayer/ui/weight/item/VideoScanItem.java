package com.kantvai.kantvplayer.ui.weight.item;

import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.bean.ScanFolderBean;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import butterknife.BindView;


public class VideoScanItem implements AdapterItem<ScanFolderBean> {

    @BindView(R.id.folder_tv)
    TextView folderTv;
    @BindView(R.id.folder_cb)
    CheckBox folderCb;

    private OnFolderCheckListener listener;

    public VideoScanItem(OnFolderCheckListener listener){
        this.listener = listener;
    }

    @Override
    public int getLayoutResId() {
        return R.layout.item_video_scan;
    }

    @Override
    public void initItemViews(View itemView) {

    }

    @Override
    public void onSetViews() {

    }

    @Override
    public void onUpdateViews(ScanFolderBean model, int position) {
        folderTv.setText(model.getFolder());
        folderCb.setChecked(model.isCheck());
        folderCb.setOnCheckedChangeListener((buttonView, isChecked) -> {
            listener.onCheck(isChecked, position);
        });
    }

    public interface OnFolderCheckListener{
        void onCheck(boolean isCheck, int position);
    }
}
