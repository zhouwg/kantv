package com.kantvai.kantvplayer.utils.database.callback;

import android.database.Cursor;

import io.reactivex.annotations.NonNull;
import io.reactivex.annotations.Nullable;

public interface QueryAsyncCallback {
    void onQuery(@NonNull Cursor cursor);
}
