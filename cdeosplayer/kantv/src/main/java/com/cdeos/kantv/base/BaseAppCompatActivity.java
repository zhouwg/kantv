package com.cdeos.kantv.base;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.view.KeyEvent;
import android.view.WindowManager;

import androidx.annotation.ColorInt;
import androidx.annotation.DrawableRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.app.SkinAppCompatDelegateImpl;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.cdeos.kantv.BuildConfig;
import com.gyf.immersionbar.ImmersionBar;
import com.cdeos.kantv.R;
import com.cdeos.kantv.utils.CommonUtils;
import com.cdeos.kantv.utils.Settings;
import com.cdeos.kantv.utils.interf.IBaseView;


import butterknife.ButterKnife;
import butterknife.Unbinder;
import cdeos.media.player.CDEAssetLoader;
import skin.support.SkinCompatManager;
import skin.support.observe.SkinObservable;
import skin.support.observe.SkinObserver;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;


/**
 * onCreate()
 * onStart()
 * onResume()
 * onPause()
 * onStop()
 * onDestroy()
 */
public abstract class BaseAppCompatActivity extends AppCompatActivity implements IBaseView, SkinObserver {
    private final static String TAG = BaseAppCompatActivity.class.getName();
    private static final int MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 1;
    private static final int MY_PERMISSIONS_REQUEST_READ_PHONE_STATE = 2;
    private static final int MY_PERMISSIONS_REQUEST_WRITE_EXTERNAL_STORAGE = 3;
    private static final int SDK_PERMISSION_REQUEST = 4;
    private Settings mSettings;
    private Context mContext;
    private SharedPreferences mSharedPreferences;


    private Toolbar mActionBarToolbar;

    protected boolean isDestroyed = false;

    private Handler handler;

    private Unbinder unbind;


    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        initApp();

        onBeforeSetContentLayout();
        setContentView(initPageLayoutID());
        unbind = ButterKnife.bind(this);
        initActionBar();
        setStatusBar();
        init();
        initPageView();
        initPageViewListener();
        process(savedInstanceState);
        SkinCompatManager.getInstance().addObserver(this);
    }

    @NonNull
    @Override
    public AppCompatDelegate getDelegate() {
        return SkinAppCompatDelegateImpl.get(this, this);
    }


    @LayoutRes
    protected abstract int initPageLayoutID();


    @SuppressLint("PrivateResource")
    protected void initActionBar() {
        if (getActionBarToolbar() == null) {
            return;
        }
        mActionBarToolbar.setBackgroundColor(getToolbarColor());
        if (hasBackActionbar() && getSupportActionBar() != null) {
            ActionBar actionBar = getSupportActionBar();
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowTitleEnabled(false);
            setSupportActionBar(mActionBarToolbar);
            mActionBarToolbar.setNavigationOnClickListener(v -> finishPage());
        }
    }


    protected Toolbar getActionBarToolbar() {
        if (mActionBarToolbar == null) {
            mActionBarToolbar = findViewById(R.id.toolbar);
            if (mActionBarToolbar != null) {
                setSupportActionBar(mActionBarToolbar);
            }
        }
        return mActionBarToolbar;
    }


    protected void setStatusBar() {
        ImmersionBar.with(this)
                .statusBarColorInt(CommonUtils.getResColor(R.color.colorPrimaryDark))
                .fitsSystemWindows(true)
                .init();
    }


    protected void init() {
    }


    protected void process(Bundle savedInstanceState) {
    }

    @Override
    protected void onDestroy() {
        isDestroyed = true;
        super.onDestroy();
        unbind.unbind();
        SkinCompatManager.getInstance().deleteObserver(this);
    }


    protected boolean hasBackActionbar() {
        return true;
    }


    @DrawableRes
    protected int setBackIcon() {
        return 0;
    }


    @ColorInt
    protected int getToolbarColor() {
        return CommonUtils.getResColor(R.color.colorPrimaryDark);
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBeforeFinish();
        finish();
        return super.onSupportNavigateUp();
    }


    public void finishPage() {
        onBeforeFinish();
        finish();
    }


    protected void onBeforeSetContentLayout() {
    }


    protected void onBeforeFinish() {

    }


    public void launchActivity(Class<? extends Activity> cls) {
        launchActivity(cls, null);
    }


    public void launchActivity(Class<? extends Activity> cls, @Nullable Bundle bundle) {
        Intent intent = new Intent(this, cls);
        if (bundle != null) {
            intent.putExtras(bundle);
        }
        startActivity(intent);
    }


    public void launchActivity(Class<? extends Activity> cls, @Nullable Bundle bundle, int flags) {
        Intent intent = new Intent(this, cls);
        intent.setFlags(flags);
        if (bundle != null) intent.putExtras(bundle);
        startActivity(intent);
    }


    public void launchActivityForResult(Class<? extends Activity> cls, int requestCode, @Nullable Bundle bundle) {
        Intent intent = new Intent(this, cls);
        if (bundle != null) {
            intent.putExtras(bundle);
        }
        startActivityForResult(intent, requestCode);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            finishPage();
        }
        return false;
    }

    @Override
    public void initData() {

    }

    @Override
    public void updateSkin(SkinObservable observable, Object o) {
        setStatusBar();
    }


    public boolean isDestroyed() {
        return super.isDestroyed() || isDestroyed;
    }

    public Handler getHandler() {
        synchronized (this) {
            if (handler == null) {
                handler = new Handler(Looper.getMainLooper());
            }
        }
        return handler;
    }


    public void backgroundAlpha(Float bgAlpha) {
        WindowManager.LayoutParams lp = this.getWindow().getAttributes();
        lp.alpha = bgAlpha;
        this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        this.getWindow().setAttributes(lp);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }


    @TargetApi(23)
    private void requestPermissions() {
        CDEUtils.setPermissionGranted(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(this,
                        new String[]{
                                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                Manifest.permission.READ_EXTERNAL_STORAGE,
                                Manifest.permission.READ_PHONE_STATE
                        },
                        SDK_PERMISSION_REQUEST);
            } else {
                CDEUtils.setPermissionGranted(true);
            }

        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == SDK_PERMISSION_REQUEST) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                CDELog.j(TAG, "PERMISSIONS was granted");
                CDEUtils.setPermissionGranted(true);
            } else {
                CDELog.j(TAG, "PERMISSIONS was denied");
                CDEUtils.setPermissionGranted(true);
                //CDEUtils.exitAPK(this);
            }
        }
    }


    public void initApp() {
        long startTime = System.currentTimeMillis();
        String buildTime = BuildConfig.BUILD_TIME;
        CDELog.j(TAG, "*************************enter initApp *********************************");
        CDELog.j(TAG, "buildTime: " + buildTime);
        mContext = this.getBaseContext();
        mSettings = new Settings(getApplicationContext());
        mSettings.updateUILang(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);

        CDEUtils.dumpDeviceInfo();
        boolean isNetworkAvailable = CDEUtils.isNetworkAvailable(this);
        CDELog.j(TAG, "isNetworkAvailable=" + isNetworkAvailable);
        CDEUtils.setNetworkAvailable(isNetworkAvailable);
        if (CDEUtils.isNetworkAvailable()) {
            CDELog.j(TAG, "network type:" + CDEUtils.getNetworkTypeString());
        }
        CDELog.j(TAG, "wifi mac:" + CDEUtils.getWifiMac());
        CDEUtils.generateUniqueID(this);
        CDELog.j(TAG, "device unique id: " + CDEUtils.getUniqueID());
        CDELog.j(TAG, "device unique id string: " + CDEUtils.getUniqueIDString());
        try {
            WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi.getConnectionInfo();
            CDELog.j(TAG, "wifi ip:" + info.getIpAddress());
            CDELog.j(TAG, "wifi mac:" + info.getMacAddress());
            CDELog.j(TAG, "wifi ip:" + CDEUtils.getNetworkIP());
            CDELog.j(TAG, "wifi mac:" + CDEUtils.getWifiMac());
            CDELog.j(TAG, "wifi name:" + info.getSSID());
        } catch (Exception e) {
            CDELog.j(TAG, "can't get wifi info: " + e.toString());
        }

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        float density = dm.density;
        int densityDpi = dm.densityDpi;
        int screenWidth = (int) (width / density);
        int screenHeight = (int) (height / density);
        CDEUtils.setScreenWidth(width);
        CDEUtils.setScreenHeight(height);
        CDELog.j(TAG, "ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE=" + ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        CDELog.j(TAG, "ActivityInfo.SCREEN_ORIENTATION_PORTRAIT =" + ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        CDELog.j(TAG, "current orientation:                     "  + this.getRequestedOrientation());
        CDELog.j(TAG, "screen width x height（pixel)= : " + width + "x" + height);
        CDELog.j(TAG, "screen width x height (dp)   = : " + screenWidth + "x" + screenHeight);
        CDELog.j(TAG, "isRunningOnTV = " + CDEUtils.isRunningOnTV());

        //for realtime subtitle demo with prebuilt android apk
        String ggmlModelFileName = "ggml-tiny-q5_1.bin"; //31M
        CDEAssetLoader.copyAssetFile(mContext, ggmlModelFileName, CDEUtils.getDataPath() + ggmlModelFileName);

        if (CDEUtils.isRunningOnTV()) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            CDEUtils.setScreenOrientation(CDEUtils.SCREEN_ORIENTATION_LANDSCAPE);
        } else {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            CDEUtils.setScreenOrientation(cdeos.media.player.CDEUtils.SCREEN_ORIENTATION_PORTRAIT);
        }
        requestPermissions();

        long endTime   = System.currentTimeMillis();
        CDELog.j(TAG, "app launch cost time " + (endTime - startTime) + " milliseconds");
    }
}
