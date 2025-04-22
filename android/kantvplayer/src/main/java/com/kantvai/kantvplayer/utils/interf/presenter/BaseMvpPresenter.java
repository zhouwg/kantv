package com.kantvai.kantvplayer.utils.interf.presenter;

import android.os.Bundle;


public interface BaseMvpPresenter extends Presenter {

    void init();

    void initPage();

    void process(Bundle savedInstanceState);
}
