package com.kantvai.kantvplayer.utils;

import android.content.Context;

import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.app.IApplication;


public class SoUtils {
    private static final int BUGLY_APP_ID = 0xA1001234;

    private SoUtils() {

    }

    private static class Holder {
        static SoUtils instance = new SoUtils();
    }

    public static SoUtils getInstance() {
        return Holder.instance;
    }

    public String getBuglyAppId() {
        return getKey(BUGLY_APP_ID);
    }

    private String getKey(int key) {
        return "kantv";
    }
}
