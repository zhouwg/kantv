package com.kantvai.kantvplayer.player.common.utils;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;

import java.lang.reflect.Method;


public final class NavUtils {

    private NavUtils() {
        throw new RuntimeException("NavUtils cannot be initialized!");
    }


    public static int getNavigationBarHeight(Context activity) {
        if (!checkDeviceHasNavigationBar(activity)) {
            return 0;
        }
        Resources resources = activity.getResources();
        int resourceId = resources.getIdentifier("navigation_bar_height",
                "dimen", "android");

        int height = resources.getDimensionPixelSize(resourceId);
        return height;
    }

    @SuppressLint("PrivateApi")
    public static boolean checkDeviceHasNavigationBar(Context context) {
        boolean hasNavigationBar = false;
        Resources rs = context.getResources();
        int id = rs.getIdentifier("config_showNavigationBar", "bool", "android");
        if (id > 0) {
            hasNavigationBar = rs.getBoolean(id);
        }
        try {
            Class systemPropertiesClass = Class.forName("android.os.SystemProperties");
            Method m = systemPropertiesClass.getMethod("get", String.class);
            String navBarOverride = (String) m.invoke(systemPropertiesClass, "qemu.hw.mainkeys");
            if ("1".equals(navBarOverride)) {
                hasNavigationBar = false;
            } else if ("0".equals(navBarOverride)) {
                hasNavigationBar = true;
            }
        } catch (Exception e) {

        }
        return hasNavigationBar;

    }
}
