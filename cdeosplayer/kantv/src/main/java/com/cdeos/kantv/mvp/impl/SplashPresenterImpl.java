package com.cdeos.kantv.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.SplashPresenter;
import com.cdeos.kantv.mvp.view.SplashView;
import com.cdeos.kantv.utils.AppConfig;


public class SplashPresenterImpl extends BaseMvpPresenterImpl<SplashView> implements SplashPresenter {

    public SplashPresenterImpl(SplashView view, LifecycleOwner lifecycleOwner) {
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

    @Override
    public void checkToken() {
    }
}
