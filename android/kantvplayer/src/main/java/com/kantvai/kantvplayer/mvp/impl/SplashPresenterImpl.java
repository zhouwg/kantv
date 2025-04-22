package com.kantvai.kantvplayer.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.kantvai.kantvplayer.base.BaseMvpPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.SplashPresenter;
import com.kantvai.kantvplayer.mvp.view.SplashView;
import com.kantvai.kantvplayer.utils.AppConfig;


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
