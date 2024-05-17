package com.cdeos.kantv.utils.database.callback;

import android.database.Cursor;

import androidx.annotation.NonNull;

public interface QuerySyncCallback {
    void onQuery(@NonNull Cursor cursor);
}