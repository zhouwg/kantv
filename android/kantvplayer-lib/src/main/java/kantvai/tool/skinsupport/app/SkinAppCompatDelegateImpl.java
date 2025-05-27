package kantvai.tool.skinsupport.app;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.MenuInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatCallback;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.view.ActionMode;
import androidx.appcompat.widget.Toolbar;

import java.util.HashMap;
import java.util.Map;

public class SkinAppCompatDelegateImpl {
    private static Map<Activity, AppCompatDelegate> sDelegateMap = new HashMap<>();

    private static AppCompatDelegate mDelegate;
    private static Activity mActivity;

    public static AppCompatDelegate get(Activity activity, AppCompatCallback callback) {
        AppCompatDelegate delegate = sDelegateMap.get(activity);
        if (delegate == null) {
            delegate = AppCompatDelegate.create(activity, callback);
            sDelegateMap.put(activity, delegate);
            mDelegate = delegate;
            mActivity = activity;
        }
        return delegate;
    }



    public ActionBar getSupportActionBar() {
        return mDelegate.getSupportActionBar();
    }


    public void setSupportActionBar(@Nullable Toolbar toolbar) {
        mDelegate.setSupportActionBar(toolbar);
    }


    public MenuInflater getMenuInflater() {
        return mDelegate.getMenuInflater();
    }


    public void onCreate(Bundle savedInstanceState) {
        mDelegate.onCreate(savedInstanceState);
    }


    public void onPostCreate(Bundle savedInstanceState) {
        mDelegate.onPostCreate(savedInstanceState);
    }


    public void onConfigurationChanged(Configuration newConfig) {
        mDelegate.onConfigurationChanged(newConfig);
    }


    public void onStart() {
        mDelegate.onStart();
    }


    public void onStop() {
        mDelegate.onStop();
    }


    public void onPostResume() {
        mDelegate.onPostResume();
    }


    public View findViewById(int id) {
        return mDelegate.findViewById(id);
    }


    public void setContentView(View v) {
        mDelegate.setContentView(v);
    }


    public void setContentView(int resId) {
        mDelegate.setContentView(resId);
    }


    public void setContentView(View v, ViewGroup.LayoutParams lp) {
        mDelegate.setContentView(v, lp);
    }


    public void addContentView(View v, ViewGroup.LayoutParams lp) {
        mDelegate.addContentView(v, lp);
    }

    public void setTitle(@Nullable CharSequence title) {
        mDelegate.setTitle(title);
    }


    public void invalidateOptionsMenu() {
        mDelegate.invalidateOptionsMenu();
    }


    public void onDestroy() {
        mDelegate.onDestroy();
        sDelegateMap.remove(mActivity);
    }


    public ActionBarDrawerToggle.Delegate getDrawerToggleDelegate() {
        return mDelegate.getDrawerToggleDelegate();
    }

    public boolean requestWindowFeature(int featureId) {
        return mDelegate.requestWindowFeature(featureId);
    }


    public boolean hasWindowFeature(int featureId) {
        return mDelegate.hasWindowFeature(featureId);
    }


    public ActionMode startSupportActionMode(@NonNull ActionMode.Callback callback) {
        return mDelegate.startSupportActionMode(callback);
    }


    public void installViewFactory() {
        // ignore
    }


    public View createView(@Nullable View parent, String name, @NonNull Context context, @NonNull AttributeSet attrs) {
        return mDelegate.createView(parent, name, context, attrs);
    }


    public void setHandleNativeActionModesEnabled(boolean enabled) {
        mDelegate.setHandleNativeActionModesEnabled(enabled);
    }


    public boolean isHandleNativeActionModesEnabled() {
        return mDelegate.isHandleNativeActionModesEnabled();
    }


    public void onSaveInstanceState(Bundle outState) {
        mDelegate.onSaveInstanceState(outState);
    }


    public boolean applyDayNight() {
        return mDelegate.applyDayNight();
    }


    public void setLocalNightMode(int mode) {
        mDelegate.setLocalNightMode(mode);
    }
}
