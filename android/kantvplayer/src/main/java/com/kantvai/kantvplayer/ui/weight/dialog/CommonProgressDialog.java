package com.kantvai.kantvplayer.ui.weight.dialog;

import android.app.Dialog;
import android.content.Context;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.weight.CircleProgressView;

import butterknife.BindView;
import butterknife.ButterKnife;

public class CommonProgressDialog extends Dialog {

    @BindView(R.id.progress_view)
    CircleProgressView progressView;
    @BindView(R.id.tips_tv)
    TextView tipsTv;

    public CommonProgressDialog(@NonNull Context context) {
        super(context, R.style.Dialog);
        setContentView(R.layout.dialog_common_progerss);
        ButterKnife.bind(this);
        progressView.updateProgress(0);
        setCancelable(false);
        setCanceledOnTouchOutside(false);
    }

    public void updateTips(String tips){
        tipsTv.setText(tips);
    }

    public void updateProgress(int progress) {
        progressView.updateProgress(progress);
    }
}
