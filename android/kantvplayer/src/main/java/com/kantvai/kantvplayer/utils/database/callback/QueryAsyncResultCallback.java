package com.kantvai.kantvplayer.utils.database.callback;

import android.database.Cursor;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;

import io.reactivex.annotations.Nullable;

public abstract class QueryAsyncResultCallback<T> {

    private LifecycleOwner lifecycleOwner;

    public QueryAsyncResultCallback(LifecycleOwner lifecycleOwner){
        this.lifecycleOwner = lifecycleOwner;
    }

    public abstract T onQuery(@Nullable Cursor cursor);

    public void onPrepared(@Nullable T result){
        if (lifecycleOwner == null || lifecycleOwner.getLifecycle().getCurrentState() != Lifecycle.State.DESTROYED){
            onResult(result);
        }
    }

    public abstract void onResult(@Nullable T result);
}
