package com.kantvai.kantvplayer.utils.net;

import android.app.ProgressDialog;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;

import com.blankj.utilcode.util.LogUtils;
import com.google.gson.JsonSyntaxException;

import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import kantvai.media.player.KANTVLog;

public abstract class CommOtherDataObserver<T> implements Observer<T> {

    private final static String TAG = CommOtherDataObserver.class.getName();

    private LifecycleOwner lifecycleOwner;
    private ProgressDialog progressDialog;
    private Disposable mDisposable;

    public CommOtherDataObserver() {
        lifecycleOwner = null;
    }

    public CommOtherDataObserver(LifecycleOwner lifecycleOwner) {
        this.lifecycleOwner = lifecycleOwner;
    }

    public CommOtherDataObserver(LifecycleOwner lifecycleOwner, ProgressDialog progressDialog) {
        this.lifecycleOwner = lifecycleOwner;
        this.progressDialog = progressDialog;
        progressDialog.setOnCancelListener(dialog -> mDisposable.dispose());
    }

    @Override
    public void onSubscribe(Disposable d) {
        mDisposable = d;
        if (!isNotDestroyed()) {
            d.dispose();
        }
    }

    @Override
    public void onNext(T value) {
        if (isNotDestroyed()) {
            onSuccess(value);
        }
    }

    @Override
    public void onError(Throwable e) {
        if (isNotDestroyed()) {
            onError(-1, getErrorMessage(e));
        }
    }

    @Override
    public void onComplete() {
        if (progressDialog != null && progressDialog.isShowing()) {
            progressDialog.dismiss();
        }
    }

    private String getErrorMessage(Throwable e) {
        e.printStackTrace();
        KANTVLog.d(TAG, e.toString());

        if (e instanceof JsonSyntaxException) {
            LogUtils.i("error", e.toString());
            KANTVLog.j(TAG, "error:" + e.toString());
            return "数据异常";
        } else if (e instanceof UnknownHostException) {
            LogUtils.i("error", e.toString());
            KANTVLog.j(TAG, "error:" + e.toString());
            return "网络连接中断";
        } else if (e instanceof SocketTimeoutException) {
            KANTVLog.j(TAG, "server is busy");
            return "服务器繁忙";
        } else {
            KANTVLog.j(TAG, "其它异常: " + e.toString());
            return "其它异常: " + e.getClass();
        }
    }

    private boolean isNotDestroyed() {
        return lifecycleOwner == null ||
                lifecycleOwner.getLifecycle().getCurrentState() != Lifecycle.State.DESTROYED;
    }

    public abstract void onSuccess(T t);

    public abstract void onError(int errorCode, String message);

    public void onProgress(int progress, long total) {
    }

    //断开回调
    public void dispose() {
        if (mDisposable != null)
            mDisposable.dispose();
    }
}
