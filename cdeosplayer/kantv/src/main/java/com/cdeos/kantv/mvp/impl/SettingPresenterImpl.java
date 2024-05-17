package com.cdeos.kantv.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.SettingPresenter;
import com.cdeos.kantv.mvp.view.SettingView;


public class SettingPresenterImpl extends BaseMvpPresenterImpl<SettingView> implements SettingPresenter {

    public SettingPresenterImpl(SettingView view, LifecycleOwner lifecycleOwner) {
        super(view, lifecycleOwner);
    }

    @Override
    public void init() {

    }

    @Override
    public void process(Bundle savedInstanceState) {

    }

    @Override
    public void resume() {

    }

    @Override
    public void pause() {

    }

    @Override
    public void destroy() {

    }
}
