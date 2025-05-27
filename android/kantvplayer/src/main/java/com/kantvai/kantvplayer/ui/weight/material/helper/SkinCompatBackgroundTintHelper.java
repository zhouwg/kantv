package com.kantvai.kantvplayer.ui.weight.material.helper;

import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.View;

import com.kantvai.kantvplayer.R;

import kantvai.media.player.KANTVLog;
import kantvai.tool.skinsupport.widget.SkinCompatBackgroundHelper;

public class SkinCompatBackgroundTintHelper extends SkinCompatBackgroundHelper {
    private View mView;
    private int backgroundTintId = INVALID_ID;
    public SkinCompatBackgroundTintHelper(View view) {
        super(view);
        mView = view;
    }

    @Override
    public void loadFromAttributes(AttributeSet attrs, int defStyleAttr) {
        super.loadFromAttributes(attrs, defStyleAttr);
        TypedArray array = mView.getContext().obtainStyledAttributes(attrs, R.styleable.ViewBackgroundHelper,defStyleAttr,0);
        if (array.hasValue(R.styleable.ViewBackgroundHelper_backgroundTint)) {
            KANTVLog.g("SkinCompatBackgroundTintHelper", "not supported");
            //backgroundTintId = array.getResourceId(R.styleable.ViewBackgroundHelper,)
        }
    }

    @Override
    public void applySkin() {
        super.applySkin();
        KANTVLog.g("SkinCompatBackgroundTintHelper", "not supported");
        if (backgroundTintId != INVALID_ID) {
           //mView.setBackgroundTintList();
        }
    }
}
