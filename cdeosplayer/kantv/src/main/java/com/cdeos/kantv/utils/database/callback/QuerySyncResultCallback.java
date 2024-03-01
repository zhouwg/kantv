package com.cdeos.kantv.utils.database.callback;

import android.database.Cursor;

import androidx.annotation.Nullable;

public interface QuerySyncResultCallback<T> {
    T onQuery(@Nullable Cursor cursor);
}