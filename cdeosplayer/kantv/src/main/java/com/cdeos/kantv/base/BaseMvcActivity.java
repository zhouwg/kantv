package com.cdeos.kantv.base;

import android.app.Dialog;

import com.cdeos.kantv.R;
import com.cdeos.kantv.ui.weight.dialog.BaseLoadingDialog;
import com.cdeos.kantv.utils.CommonUtils;


public abstract class BaseMvcActivity extends BaseAppCompatActivity {

    protected Dialog dialog;

    @Override
    protected int getToolbarColor() {
        return CommonUtils.getResColor(R.color.theme_color);
    }

    @Override
    protected void setStatusBar() {
        super.setStatusBar();
    }

    public void showLoadingDialog() {
        if (dialog == null || !dialog.isShowing()) {
            dialog = new BaseLoadingDialog(BaseMvcActivity.this);
            dialog.show();
        }
    }

    public void dismissLoadingDialog() {
        if (dialog != null && dialog.isShowing()) {
            dialog.dismiss();
            dialog = null;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        dialog = null;
    }
}
