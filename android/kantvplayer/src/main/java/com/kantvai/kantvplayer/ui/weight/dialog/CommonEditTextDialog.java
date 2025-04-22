package com.kantvai.kantvplayer.ui.weight.dialog;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.blankj.utilcode.util.KeyboardUtils;
import com.blankj.utilcode.util.StringUtils;
import com.google.android.material.textfield.TextInputLayout;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.ui.activities.play.PlayerManagerActivity;
import com.kantvai.kantvplayer.utils.CommonUtils;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;
import butterknife.OnClick;


public class CommonEditTextDialog extends Dialog{

    public static final int NETWORK_LINK = 0;
    public static final int SEARCH_SUBTITLE = 1;

    @BindView(R.id.edit_layout)
    TextInputLayout inputLayout;
    @BindView(R.id.edit_et)
    EditText editText;
    @BindView(R.id.dialog_title_tv)
    TextView titleTv;

    private int type;
    private CommonEditTextListener listener;
    private CommonEditTextFullListener fullListener;
    private List<String> blockList;

    public CommonEditTextDialog(@NonNull Context context, int type) {
        super(context, R.style.Dialog);
        this.type = type;
        this.blockList = new ArrayList<>();
    }

    public CommonEditTextDialog(@NonNull Context context, int type, CommonEditTextListener listener) {
        super(context, R.style.Dialog);
        this.type = type;
        this.listener = listener;
        this.blockList = new ArrayList<>();
    }

    public CommonEditTextDialog(@NonNull Context context, int type, List<String> blockList, CommonEditTextListener listener) {
        super(context, R.style.Dialog);
        this.type = type;
        this.listener = listener;
        this.blockList = blockList;
    }

    public CommonEditTextDialog(@NonNull Context context, int type, CommonEditTextFullListener listener) {
        super(context, R.style.Dialog);
        this.type = type;
        this.fullListener = listener;
    }

    @SuppressLint("SetTextI18n")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dialog_common_edittext);
        ButterKnife.bind(this);

        switch (type) {
            case NETWORK_LINK:
                titleTv.setText(getContext().getString(R.string.input_network_content));
                editText.setHint("http://");
                editText.setMaxLines(5);
                break;
            case SEARCH_SUBTITLE:
                titleTv.setText("search subtitle");
                editText.setHint("pls input video name");
                editText.setMaxLines(1);
                break;
        }
    }

    @OnClick({R.id.cancel_tv, R.id.confirm_tv})
    public void onViewClicked(View view) {
        switch (view.getId()) {
            case R.id.cancel_tv:
                CommonEditTextDialog.this.dismiss();
                break;
            case R.id.confirm_tv:
                doConfirm();
                break;
        }
    }

    private void doConfirm() {
        String inputData = editText.getText().toString();
        switch (type) {
            case NETWORK_LINK:
                if (StringUtils.isEmpty(inputData)) {
                    inputLayout.setErrorEnabled(true);
                    inputLayout.setError("链接不能为空");
                } else {
                    KeyboardUtils.hideSoftInput(editText);
                    int lastEx = inputData.lastIndexOf("/") + 1;
                    String title = inputData;
                    if (lastEx < inputData.length())
                        title = inputData.substring(lastEx);
                    PlayerManagerActivity.launchPlayerStream(getContext(), title, inputData,0, 0);
                    CommonEditTextDialog.this.dismiss();
                }
                break;

            case SEARCH_SUBTITLE:
                if (StringUtils.isEmpty(inputData)) {
                    inputLayout.setErrorEnabled(true);
                    inputLayout.setError("video name can not empty");
                } else if (listener != null) {
                    listener.onConfirm(inputData);
                    CommonEditTextDialog.this.dismiss();
                }
                break;
        }
    }

    private boolean traverseBlock(String blockText) {
        boolean isContains = false;
        for (String text : blockList) {
            if (text.contains(blockText)) {
                isContains = true;
                break;
            }
        }
        return isContains;
    }

    public interface CommonEditTextListener {
        void onConfirm(String... data);
    }

    public interface CommonEditTextFullListener {
        void onConfirm(String... data);

        void onCancel();
    }
}
