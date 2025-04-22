package com.kantvai.kantvplayer.base;

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

import com.kantvai.kantvplayer.BuildConfig;
import com.gyf.immersionbar.ImmersionBar;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.Settings;
import com.kantvai.kantvplayer.utils.interf.IBaseView;


import butterknife.ButterKnife;
import butterknife.Unbinder;
import kantvai.media.player.KANTVAssetLoader;
import skin.support.SkinCompatManager;
import skin.support.observe.SkinObservable;
import skin.support.observe.SkinObserver;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


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
        KANTVUtils.setPermissionGranted(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(this,
                        new String[]{
                                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                Manifest.permission.READ_EXTERNAL_STORAGE,
                                Manifest.permission.READ_PHONE_STATE,
                                Manifest.permission.CAMERA,
                                Manifest.permission.RECORD_AUDIO
                        },
                        SDK_PERMISSION_REQUEST);
            } else {
                KANTVUtils.setPermissionGranted(true);
            }

        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == SDK_PERMISSION_REQUEST) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                KANTVLog.j(TAG, "PERMISSIONS was granted");
                KANTVUtils.setPermissionGranted(true);
            } else {
                KANTVLog.j(TAG, "PERMISSIONS was denied");
                KANTVUtils.setPermissionGranted(true);
                //KANTVUtils.exitAPK(this);
            }
        }
    }


    public void initApp() {
        long startTime = System.currentTimeMillis();
        String buildTime = BuildConfig.BUILD_TIME;
        KANTVLog.j(TAG, "*************************enter initApp *********************************");
        KANTVLog.j(TAG, "buildTime: " + buildTime);
        mContext = this.getBaseContext();
        mSettings = new Settings(getApplicationContext());
        mSettings.updateUILang(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);

        KANTVUtils.dumpDeviceInfo();
        boolean isNetworkAvailable = KANTVUtils.isNetworkAvailable(this);
        KANTVLog.j(TAG, "isNetworkAvailable=" + isNetworkAvailable);
        KANTVUtils.setNetworkAvailable(isNetworkAvailable);
        if (KANTVUtils.isNetworkAvailable()) {
            KANTVLog.j(TAG, "network type:" + KANTVUtils.getNetworkTypeString());
        }
        KANTVLog.j(TAG, "wifi mac:" + KANTVUtils.getWifiMac());
        KANTVUtils.generateUniqueID(this);
        KANTVLog.j(TAG, "device unique id: " + KANTVUtils.getUniqueID());
        KANTVLog.d(TAG, "device unique id string: " + KANTVUtils.getUniqueIDString());
        try {
            WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi.getConnectionInfo();
            KANTVLog.j(TAG, "wifi ip:" + info.getIpAddress());
            KANTVLog.j(TAG, "wifi mac:" + info.getMacAddress());
            KANTVLog.j(TAG, "wifi ip:" + KANTVUtils.getNetworkIP());
            KANTVLog.j(TAG, "wifi mac:" + KANTVUtils.getWifiMac());
            KANTVLog.j(TAG, "wifi name:" + info.getSSID());
        } catch (Exception e) {
            KANTVLog.j(TAG, "can't get wifi info: " + e.toString());
        }

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        float density = dm.density;
        int densityDpi = dm.densityDpi;
        int screenWidth = (int) (width / density);
        int screenHeight = (int) (height / density);
        KANTVUtils.setScreenWidth(width);
        KANTVUtils.setScreenHeight(height);
        KANTVLog.j(TAG, "ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE=" + ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        KANTVLog.j(TAG, "ActivityInfo.SCREEN_ORIENTATION_PORTRAIT =" + ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        KANTVLog.j(TAG, "current orientation:                     "  + this.getRequestedOrientation());
        KANTVLog.j(TAG, "screen width x height（pixel)= : " + width + "x" + height);
        KANTVLog.j(TAG, "screen width x height (dp)   = : " + screenWidth + "x" + screenHeight);
        KANTVLog.j(TAG, "isRunningOnTV = " + KANTVUtils.isRunningOnTV());

        //for realtime subtitle demo with prebuilt android apk
        //String ggmlModelFileName = "ggml-tiny-q5_1.bin"; //31M
        String ggmlModelFileName = "ggml-tiny.en-q8_0.bin";//42M, ggml-tiny.en-q8_0.bin is preferred
        KANTVAssetLoader.copyAssetFile(mContext, ggmlModelFileName, KANTVUtils.getDataPath() + ggmlModelFileName);

        if (KANTVUtils.isRunningOnTV()) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            KANTVUtils.setScreenOrientation(KANTVUtils.SCREEN_ORIENTATION_LANDSCAPE);
        } else {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            KANTVUtils.setScreenOrientation(kantvai.media.player.KANTVUtils.SCREEN_ORIENTATION_PORTRAIT);
        }
        requestPermissions();

        long endTime   = System.currentTimeMillis();
        KANTVLog.j(TAG, "app launch cost time " + (endTime - startTime) + " milliseconds");
    }
}
