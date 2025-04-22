package com.kantvai.kantvplayer.ui.weight.dialog;

import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;


import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseRvAdapter;
import com.kantvai.kantvplayer.ui.weight.item.SubtitleItem;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;
import kantvai.media.player.bean.SubtitleBean;


public class SelectSubtitleDialog extends Dialog {

    @BindView(R.id.dialog_title_tv)
    TextView dialogTitleTv;
    @BindView(R.id.dialog_rv)
    RecyclerView dialogRv;

    private List<SubtitleBean> subtitleList;
    private BaseRvAdapter<SubtitleBean> adapter;
    private SubtitleItem.SubtitleSelectCallBack callBack;

    public SelectSubtitleDialog(@NonNull Context context, List<SubtitleBean> subtitleList, SubtitleItem.SubtitleSelectCallBack callBack) {
        super(context, R.style.Dialog);
        this.subtitleList = subtitleList;
        this.callBack = callBack;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dialog_select_subtitle);
        ButterKnife.bind(this);

        dialogTitleTv.setText("选择字幕");

        adapter = new BaseRvAdapter<SubtitleBean>(subtitleList) {
            @NonNull
            @Override
            public AdapterItem<SubtitleBean> onCreateItem(int viewType) {
                return new SubtitleItem((fileName, link) -> {
                    SelectSubtitleDialog.this.dismiss();
                    callBack.onSelect(fileName, link);
                });
            }
        };
        dialogRv.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
        dialogRv.setAdapter(adapter);
    }

    @Override
    public void show() {
        super.show();
        Window window = getWindow();
        if (window != null){
            WindowManager.LayoutParams layoutParams = getWindow().getAttributes();
            layoutParams.gravity = Gravity.CENTER;
            layoutParams.width= WindowManager.LayoutParams.MATCH_PARENT;
            layoutParams.height= WindowManager.LayoutParams.WRAP_CONTENT;

            getWindow().getDecorView().setPadding(0, 0, 0, 0);
            getWindow().setAttributes(layoutParams);
        }
    }

    @OnClick(R.id.dialog_cancel_iv)
    public void onViewClicked() {
        SelectSubtitleDialog.this.dismiss();
    }
}
