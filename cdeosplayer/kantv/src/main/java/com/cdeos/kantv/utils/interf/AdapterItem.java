package com.cdeos.kantv.utils.interf;

import android.view.View;

import androidx.annotation.LayoutRes;


public interface AdapterItem<T> {

    @LayoutRes
    int getLayoutResId();

    void initItemViews(final View itemView);

    void onSetViews();

    void onUpdateViews(T model, int position);

}
