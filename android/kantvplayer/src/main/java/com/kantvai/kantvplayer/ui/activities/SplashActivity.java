package com.kantvai.kantvplayer.ui.activities;

import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;

import com.blankj.utilcode.util.AppUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvpActivity;
import com.kantvai.kantvplayer.mvp.impl.SplashPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.SplashPresenter;
import com.kantvai.kantvplayer.mvp.view.SplashView;
import com.kantvai.kantvplayer.ui.weight.anim.path.TextPathAnimView;
import com.kantvai.kantvplayer.ui.weight.anim.svg.AnimatedSvgView;
import com.kantvai.kantvplayer.utils.AppConfig;

import butterknife.BindView;


public class SplashActivity extends BaseMvpActivity<SplashPresenter> implements SplashView {
    @BindView(R.id.text_path_view)
    TextPathAnimView textPathView;
    @BindView(R.id.address_ll)
    LinearLayout addressLl;
    @BindView(R.id.app_name_tv)
    TextView appNameTv;


    @Override
    protected void process(Bundle savedInstanceState) {
        super.process(savedInstanceState);
    }

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_splash;
    }

    @Override
    public void initView() {
        presenter.checkToken();

        if (AppConfig.getInstance().isCloseSplashPage()) {
            launchActivity();
            return;
        }

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WALLPAPER);

        AlphaAnimation alphaAnimation = new AlphaAnimation(0, 1);
        alphaAnimation.setRepeatCount(Animation.ABSOLUTE);
        alphaAnimation.setInterpolator(new LinearInterpolator());
        alphaAnimation.setDuration(500);

        String appName = "KanTV v" + AppUtils.getAppVersionName();
        appNameTv.setText(appName);
        appNameTv.setText("");

        textPathView.setAnimListener(new TextPathAnimView.AnimListener() {
            @Override
            public void onStart() {

            }

            @Override
            public void onEnd() {
                if (textPathView != null)
                    textPathView.postDelayed(() -> launchActivity(), 250);
            }

            @Override
            public void onLoop() {

            }
        });

        textPathView.startAnim();
        addressLl.startAnimation(alphaAnimation);
    }

    @Override
    public void initListener() {

    }

    @NonNull
    @Override
    protected SplashPresenter initPresenter() {
        return new SplashPresenterImpl(this, this);
    }

    @Override
    public void onBackPressed() {

    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            return true;
        } else {
            return super.dispatchKeyEvent(event);
        }
    }

    private void launchActivity() {
        Intent intent = new Intent(SplashActivity.this, MainActivity.class);
        startActivity(intent);
        overridePendingTransition(R.anim.anim_activity_enter, R.anim.anim_activity_exit);
        this.finish();
    }
}
