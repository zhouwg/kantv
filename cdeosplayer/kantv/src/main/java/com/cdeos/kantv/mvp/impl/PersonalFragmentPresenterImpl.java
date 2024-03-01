package com.cdeos.kantv.mvp.impl;

import android.os.Bundle;

import androidx.lifecycle.LifecycleOwner;

import com.cdeos.kantv.base.BaseMvpPresenterImpl;
import com.cdeos.kantv.mvp.presenter.PersonalFragmentPresenter;
import com.cdeos.kantv.mvp.view.PersonalFragmentView;


public class PersonalFragmentPresenterImpl extends BaseMvpPresenterImpl<PersonalFragmentView> implements PersonalFragmentPresenter {


    public PersonalFragmentPresenterImpl(PersonalFragmentView view, LifecycleOwner lifecycleOwner) {
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
