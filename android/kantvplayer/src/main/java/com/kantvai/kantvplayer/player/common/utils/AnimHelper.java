package com.kantvai.kantvplayer.player.common.utils;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import androidx.core.view.ViewCompat;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;


public final class AnimHelper {

    private AnimHelper() {
        throw new AssertionError();
    }

    public static void doSlide(View view, int startX, int endX, int duration) {
        ObjectAnimator translationX = ObjectAnimator.ofFloat(view, "translationX", startX, endX);
        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", 0, 1);
        AnimatorSet set = new AnimatorSet();
        set.setDuration(duration);
        set.playTogether(translationX, alpha);
        set.start();
    }

    public static void doShowAnimator(final View view){
        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", 0, 1);
        AnimatorSet set = new AnimatorSet();
        set.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                view.setVisibility(View.VISIBLE);
                super.onAnimationStart(animation);
            }
        });
        set.setDuration(250);
        set.playTogether(alpha);
        set.start();
    }


    public static void doHideAnimator(final View view){
        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", 1, 0);
        AnimatorSet set = new AnimatorSet();
        set.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                view.setVisibility(View.GONE);
                super.onAnimationEnd(animation);
            }
        });
        set.setDuration(250);
        set.playTogether(alpha);
        set.start();
    }


    public static void doClipViewWidth(final View view, int srcWidth, int endWidth, int duration) {
        ValueAnimator valueAnimator = ValueAnimator.ofInt(srcWidth, endWidth).setDuration(duration);
        valueAnimator.addUpdateListener(valueAnimator1 -> {
            int width = (int) valueAnimator1.getAnimatedValue();
            ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
            layoutParams.width = width;
            view.setLayoutParams(layoutParams);
        });
        valueAnimator.setInterpolator(new AccelerateInterpolator());
        valueAnimator.start();
    }


    public static void doClipViewHeight(final View view, int srcHeight, int endHeight, int duration) {
        ValueAnimator valueAnimator = ValueAnimator.ofInt(srcHeight, endHeight).setDuration(duration);
        valueAnimator.addUpdateListener(valueAnimator1 -> {
            ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
            layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT;
            view.setLayoutParams(layoutParams);
        });
        valueAnimator.setInterpolator(new AccelerateInterpolator());
        valueAnimator.start();
    }


    public static void viewTranslationX(View view){
        viewTranslationX(view, view.getWidth());
    }


    public static void viewTranslationX(View view, int transX){
        if (view.getVisibility() == View.GONE)
            view.setVisibility(View.VISIBLE);
        ViewCompat.animate(view).translationX(transX).setDuration(500);
    }


    public static void viewTranslationX(View view, int transX, long duration){
        if (view.getVisibility() == View.GONE)
            view.setVisibility(View.VISIBLE);
        ViewCompat.animate(view).translationX(transX).setDuration(duration);
    }


    public static void viewTranslationY(View view){
        viewTranslationY(view, view.getHeight());
    }


    public static void viewTranslationY(View view, int transY){
        if (view.getVisibility() == View.GONE)
            view.setVisibility(View.VISIBLE);
        ViewCompat.animate(view).translationY(transY).setDuration(300);
    }
}
