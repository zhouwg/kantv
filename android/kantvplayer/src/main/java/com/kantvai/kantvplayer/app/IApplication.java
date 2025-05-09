package com.kantvai.kantvplayer.app;

import android.annotation.TargetApi;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ResolveInfo;
import android.content.res.AssetManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;

import androidx.multidex.MultiDex;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;
import com.blankj.utilcode.util.Utils;

import com.kantvai.kantvplayer.BuildConfig;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.Constants;
import com.kantvai.kantvplayer.utils.Settings;
import com.tencent.bugly.Bugly;

import com.kantvai.kantvplayer.ui.activities.SplashActivity;
import com.kantvai.kantvplayer.ui.activities.personal.CrashActivity;
import com.kantvai.kantvplayer.ui.weight.material.MaterialViewInflater;
import com.kantvai.kantvplayer.utils.SoUtils;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.utils.net.okhttp.CookiesManager;
import com.kantvai.kantvplayer.player.common.utils.PlayerConfigShare;

import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import cat.ereza.customactivityoncrash.config.CaocConfig;
import kantvai.ai.KANTVAIUtils;
import kantvai.ai.ggmljava;
import kantvai.media.player.KANTVAssetLoader;
import kantvai.media.player.KANTVLibraryLoader;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVDRMManager;
import skin.support.SkinCompatManager;
import skin.support.app.SkinAppCompatViewInflater;
import skin.support.app.SkinCardViewInflater;
import skin.support.constraint.app.SkinConstraintViewInflater;
import skin.support.flycotablayout.app.SkinFlycoTabLayoutInflater;

public class IApplication extends Application {
    private final static String TAG = IApplication.class.getName();
    public static boolean startCorrectlyFlag = false;

    private static Handler mainHandler;
    private static ThreadPoolExecutor executor;
    private static ExecutorService sqlExecutor;
    private static Context _context;
    private static AssetManager _asset;
    private static CookiesManager cookiesManager;

    private Settings mSettings;
    private Context mContext;
    private SharedPreferences mSharedPreferences;

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        MultiDex.install(this);
    }

    @TargetApi(Build.VERSION_CODES.GINGERBREAD)
    @Override
    public void onCreate() {
        super.onCreate();
        _context = this.getApplicationContext();
        _asset = _context.getAssets();

        Utils.init(this);

        SkinCompatManager.withoutActivity(this)
                .addInflater(new MaterialViewInflater())
                .addInflater(new SkinConstraintViewInflater())
                .addInflater(new SkinCardViewInflater())
                .addInflater(new SkinFlycoTabLayoutInflater())
                .addInflater(new SkinAppCompatViewInflater())
                .setSkinStatusBarColorEnable(true)
                .setSkinWindowBackgroundEnable(true)
                .setSkinAllActivityEnable(true)
                .loadSkin();


        CaocConfig.Builder.create()
                .backgroundMode(CaocConfig.BACKGROUND_MODE_SHOW_CUSTOM)
                .enabled(true)
                .trackActivities(true)
                .minTimeBetweenCrashesMs(2000)
                .restartActivity(SplashActivity.class)
                .errorActivity(CrashActivity.class)
                .apply();


        Bugly.init(getApplicationContext(), SoUtils.getInstance().getBuglyAppId(), false);


        DataBaseManager.init(this);

        PlayerConfigShare.initPlayerConfigShare(this);

        startCorrectlyFlag = true;

        initGlobal();
    }


    public static Handler getMainHandler() {
        if (mainHandler == null) {
            mainHandler = new Handler(Looper.getMainLooper());
        }
        return mainHandler;
    }


    public static ThreadPoolExecutor getExecutor() {
        if (executor == null) {
            executor = new ThreadPoolExecutor(3, 10, 200, TimeUnit.MILLISECONDS, new ArrayBlockingQueue<>(20));
        }
        return executor;
    }


    public static ExecutorService getSqlThreadPool() {
        if (sqlExecutor == null) {
            sqlExecutor = Executors.newSingleThreadExecutor();
        }
        return sqlExecutor;
    }


    public static CookiesManager getCookiesManager() {
        if (cookiesManager == null) {
            cookiesManager = new CookiesManager(get_context());
        }
        return cookiesManager;
    }


    public static Context get_context() {
        return _context;
    }

    public static AssetManager get_asset() {
        return _asset;
    }

    public static boolean isDebug() {
        return false;
    }

    public void initGlobal() {
        long startTime = System.currentTimeMillis();
        String buildTime = BuildConfig.BUILD_TIME;
        KANTVUtils.setReleaseMode(true);
        KANTVLog.j(TAG, "*************************enter initGlobal *********************************");
        KANTVLog.j(TAG, "buildTime: " + buildTime);
        KANTVLog.j(TAG, "init app");

        //step-1
        KANTVUtils.dumpDeviceInfo();
        String uid = KANTVUtils.getAndroidID(getBaseContext());
        KANTVDRMManager.setUid(uid);
        KANTVLog.j(TAG, "android id : " + uid);

        String wifimac = KANTVUtils.getWifiMac();
        KANTVUtils.setJavalayerUniqueID("mac:" + wifimac + "androidid:" + uid);
        KANTVLog.j(TAG, "phone wifi mac:" + wifimac);

        KANTVLog.j(TAG, "device clear id:" + KANTVDRM.getInstance().ANDROID_JNI_GetDeviceClearIDString());
        KANTVLog.j(TAG, "device  id:" + KANTVDRM.getInstance().ANDROID_JNI_GetDeviceIDString());

        if (!KANTVUtils.getReleaseMode())
            KANTVLog.j(TAG, "device unique id: " + KANTVUtils.getUniqueID());

        KANTVUtils.getDataPath();
        Constants.DefaultConfig.initDataPath();

        mContext = this.getBaseContext();
        mSettings = new Settings(getApplicationContext());
        //mSettings.updateUILang(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);


        try {
            WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi.getConnectionInfo();
            KANTVLog.j(TAG, "wifi ip:" + info.getIpAddress());
            KANTVLog.j(TAG, "wifi mac:" + info.getMacAddress());
            KANTVLog.j(TAG, "wifi name:" + info.getSSID());
        } catch (Exception e) {
            KANTVLog.j(TAG, "can't get wifi info: " + e.toString());
        }

        //step-2
        /* reduce permission requirement with APK, don't need to read Application List on phone
        Intent intent = new Intent(Intent.ACTION_MAIN, null);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> packageInfos = getPackageManager().queryIntentActivities(intent, 0);
        int indexKanTV = 0;
        for (int i = packageInfos.size() - 1; i >= 0; i--) {
            String launcherActivityName = packageInfos.get(i).activityInfo.name;
            String packageName = packageInfos.get(i).activityInfo.packageName;
            KANTVLog.i(TAG, i + " -- launcherActivityName: " + launcherActivityName);
            KANTVLog.e(TAG, i + " -- packageName: " + packageName);
            if (packageName.equals("com.kantvai.kantvplayer")) {
                indexKanTV = i;
                break;
            }
        }
        String launcherName = packageInfos.get(indexKanTV).activityInfo.name;
        KANTVLog.d(TAG, "packageName: " + packageInfos.get(indexKanTV).activityInfo.packageName + " ,launcherActivityName: " + launcherName);
        */

        //step-3: asset files
        KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
        mKANTVDRM.ANDROID_JNI_Init(mContext, KANTVUtils.getDataPath(mContext));
        mKANTVDRM.ANDROID_JNI_SetLocalEMS(KANTVUtils.getLocalEMS());
        KANTVAssetLoader.copyAssetFile(mContext, "config.json", KANTVAssetLoader.getDataPath(mContext) + "config.json");

        KANTVAssetLoader.copyAssetFile(mContext, "models/ggml-hexagon.cfg", KANTVAssetLoader.getDataPath(mContext) + "ggml-hexagon.cfg");
        KANTVAssetLoader.copyAssetFile(mContext, "models/libggmlop-skel.so", KANTVAssetLoader.getDataPath(mContext) + "libggmlop-skel.so");
        //copy asset files to /sdcard/kantv/, this file is needed for ASR benchmark
        KANTVAssetLoader.copyAssetFile(mContext, "models/jfk.wav", KANTVUtils.getDataPath() + "jfk.wav");
        KANTVAssetLoader.copyAssetFile(mContext, "models/jfk.wav", KANTVUtils.getDataPath(mContext) + "jfk.wav");
        KANTVAssetLoader.copyAssetFile(mContext, "models/ggml-tiny.en-q8_0.bin", KANTVAssetLoader.getDataPath(mContext) + "ggml-tiny.en-q8_0.bin");
        KANTVAssetLoader.copyAssetFile(mContext, "models/ggml-tiny.en-q8_0.bin", KANTVUtils.getDataPath() + "ggml-tiny.en-q8_0.bin");

        KANTVUtils.copyAssetFile(mContext, "res/apple.png", KANTVUtils.getDataPath(mContext) + "apple.png");
        KANTVUtils.copyAssetFile(mContext, "res/colorkey.png", KANTVUtils.getDataPath(mContext) + "colorkey.png");
        KANTVUtils.copyAssetFile(mContext, "res/line.png", KANTVUtils.getDataPath(mContext) + "line.png");
        KANTVUtils.copyAssetFile(mContext, "res/logo.png", KANTVUtils.getDataPath(mContext) + "logo.png");
        KANTVUtils.copyAssetFile(mContext, "res/png1.png", KANTVUtils.getDataPath(mContext) + "png1.png");
        KANTVUtils.copyAssetFile(mContext, "res/png2.png", KANTVUtils.getDataPath(mContext) + "png2.png");
        KANTVUtils.copyAssetFile(mContext, "res/simhei.ttf", KANTVUtils.getDataPath(mContext) + "simhei.ttf");
        KANTVAssetLoader.copyAssetFile(mContext, "res/vision-test.jpg", KANTVUtils.getDataPath() + "vision-test.jpg");
        KANTVAssetLoader.copyAssetFile(mContext, "res/vision-test.jpg", KANTVUtils.getSDCardDataPath() + "vision-test.jpg");

        KANTVUtils.copyAssetFile(mContext, "tv.xml", KANTVUtils.getDataPath(mContext) + "tv.xml"); //backuped EPG / L2 EPG
        KANTVAssetLoader.copyAssetFile(mContext, "tv.xml", KANTVUtils.getSDCardDataPath() + "tv.xml"); //preferred EPG / L1 EPG

        //for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
        //   QNN issue:
        //   can not open QNN library /sdcard/kantv/qnnlib/libQnnSystem.so,
        //   error: dlopen failed: library "/sdcard/kantv/qnnlib/libQnnSystem.so"
        //   needed or dlopened by "/data/app/~~clbTlTogBUHAPF5Da52Cfw==/com.kantvai.kantvplayer-k2X0NpXfzg9uT10HNFGVDQ==/base.apk!/lib/arm64-v8a/libggml-jni.so" is not accessible for the namespace "clns-4"
        KANTVAssetLoader.copyAssetDir(mContext, "qnnlib", KANTVUtils.getDataPath(mContext) + "qnnlib");
        KANTVLog.j(TAG, "qnn lib path:" + KANTVUtils.getDataPath(mContext) + "qnnlib");

        //step-4:
        String configString = KANTVAssetLoader.readTextFromFile(KANTVAssetLoader.getDataPath(mContext) + "config.json");
        JSONObject jsonObject = JSON.parseObject(configString);
        KANTVLog.j(TAG, "Config: kantvServer: " + jsonObject.getString("kantvServer"));
        KANTVLog.j(TAG, "Config: rtmpServer: " + jsonObject.getString("rtmpServerUrl"));
        KANTVLog.j(TAG, "Config: provision URL: " + jsonObject.getString("provisionUrl"));
        KANTVLog.d(TAG, "Config: usingTEE: " + jsonObject.getString("usingTEE"));
        KANTVLog.d(TAG, "Config: troubleshootingMode: " + jsonObject.getString("troubleshootingMode"));
        KANTVLog.j(TAG, "Config: releaseMode: " + jsonObject.getString("releaseMode"));
        KANTVLog.j(TAG, "Config: apkVersion: " + jsonObject.getString("apkVersion"));
        KANTVLog.j(TAG, "Config: apkForTV: " + jsonObject.getString("apkForTV"));


        SharedPreferences.Editor editor = mSharedPreferences.edit();
        String key = mContext.getString(R.string.pref_key_version);
        String apkVersionString = jsonObject.getString("apkVersion");
        if (apkVersionString != null) {
            KANTVUtils.setKANTVAPKVersion(apkVersionString);
            if (key != null) {
                KANTVLog.j(TAG, "current version: " + key);
                key = mContext.getString(R.string.pref_key_version);
                editor.putString(key, apkVersionString);
                editor.commit();
                KANTVLog.j(TAG, "app version:" + apkVersionString);
            } else {
                KANTVLog.j(TAG, "no app version info in pref store");
            }
        } else {
            KANTVLog.j(TAG, "can't find app version info in config file, it should not happen, pls check why?\n");
        }

        String apkForTVString = jsonObject.getString("apkForTV");
        if (apkForTVString != null) {
            int apkForTV = Integer.valueOf(apkForTVString);
            KANTVLog.j(TAG, "apk for TV: " + apkForTV);
            KANTVUtils.setAPKForTV((1 == apkForTV) ? true : false);
        } else {
            KANTVUtils.setAPKForTV(false);
        }

        // releaseMode:disable/enable log
        // troubleshootingMode: internal development mode
        // expertMode:whether user is IT worker or expert
        String releaseModeString = jsonObject.getString("releaseMode");
        if (releaseModeString != null) {
            int releaseMode = Integer.valueOf(releaseModeString);
            KANTVLog.j(TAG, "releaseMode: " + releaseMode);
            KANTVUtils.setReleaseMode(1 == releaseMode);
        } else {
            KANTVUtils.setReleaseMode(true);
        }
        KANTVLog.j(TAG, "qnn lib path        : " + KANTVUtils.getDataPath(mContext) + "qnnlib");
        KANTVLog.j(TAG, "runtime lib path    : " + KANTVUtils.getDataPath(mContext));
        KANTVLog.j(TAG, "dev  mode           : " + mSettings.getDevMode());
        KANTVUtils.setExpertMode(mSettings.getDevMode());


        KANTVUtils.setTroubleshootingMode(KANTVUtils.TROUBLESHOOTING_MODE_DISABLE);
        int troubleshootingMode = KANTVUtils.TROUBLESHOOTING_MODE_DISABLE;
        if (jsonObject.getString("troubleshootingMode") != null) {
            troubleshootingMode = Integer.valueOf(jsonObject.getString("troubleshootingMode"));
        }

        KANTVUtils.setTroubleshootingMode(KANTVUtils.TROUBLESHOOTING_MODE_DISABLE);

        if (true) {
            key = mContext.getString(R.string.pref_key_using_tee);
            editor.putBoolean(key, false);
            editor.commit();
            KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
        }

        KANTVUtils.setUsingFFmpegCodec(mSettings.getUsingFFmpegCodec());
        KANTVLog.j(TAG, "using ffmpeg codec for audio:" + KANTVUtils.getUsingFFmpegCodec());

        String startPlayPosString = mSettings.getStartPlaypos();
        KANTVLog.d(TAG, "start play pos: " + startPlayPosString + " minutes");
        int settingStartPlayPos = 0;

        if (startPlayPosString != null)
            settingStartPlayPos = Integer.valueOf(startPlayPosString);
        KANTVLog.d(TAG, "start play pos: " + settingStartPlayPos + " minutes");

        String tmpString = jsonObject.getString("startPlayPos");
        int configStartPlayPos = 0;
        if (tmpString != null)
            configStartPlayPos = Integer.valueOf(tmpString);

        if (0 == configStartPlayPos) {
            KANTVUtils.setStartPlayPos(settingStartPlayPos);
        } else {
            KANTVUtils.setStartPlayPos(configStartPlayPos);
        }
        KANTVLog.j(TAG, "startPlayPos: " + KANTVUtils.getStartPlayPos() + " minutes");
        String localEMS = jsonObject.getString("localEMS");
        if (localEMS != null)
            KANTVUtils.setLocalEMS(localEMS);
        KANTVUtils.setDisableAudioTrack(mSettings.getAudioDisabled());
        KANTVUtils.setDisableVideoTrack(mSettings.getVideoDisabled());
        KANTVUtils.setEnableDumpVideoES(mSettings.getEnableDumpVideoES());
        KANTVUtils.setEnableDumpAudioES(mSettings.getEnableDumpAudioES());


        KANTVLog.j(TAG, "disable audio: " + KANTVUtils.getDisableAudioTrack());
        KANTVLog.j(TAG, "disable video: " + KANTVUtils.getDisableVideoTrack());
        KANTVLog.j(TAG, "enable dump video es: " + KANTVUtils.getEnableDumpVideoES());
        KANTVLog.j(TAG, "enable dump audio es: " + KANTVUtils.getEnableDumpAudioES());
        KANTVLog.j(TAG, "continued playback  : " + mSettings.getContinuedPlayback());

        int recordMode = mSettings.getRecordMode();  //default is both video & audio
        int recordFormat = mSettings.getRecordFormat();//default is mp4
        int recordCodec = mSettings.getRecordCodec(); //default is h264

        KANTVUtils.setRecordConfig(KANTVUtils.getDataPath(), recordMode, recordFormat, recordCodec, mSettings.getRecordDuration(), mSettings.getRecordSize());
        KANTVUtils.setTVRecording(false);

        int asrMode = mSettings.getASRMode();  //default is normal transcription
        int asrThreadCounts = mSettings.getASRThreadCounts(); //default is 4
        KANTVLog.j(TAG, "ASR model: " + mSettings.getASRModel());
        KANTVLog.j(TAG, "ASR model name: " + KANTVAIUtils.getASRModelString(mSettings.getASRModel()));
        String modelPath = KANTVUtils.getDataPath(mContext) + "ggml-" + KANTVAIUtils.getASRModelString(mSettings.getASRModel()) + ".bin";
        KANTVLog.j(TAG, "modelPath:" + modelPath);

        //preload GGML model and initialize asr_subsystem as early as possible for purpose of ASR real-time performance
        try {
            int result = 0;
            KANTVLibraryLoader.load("ggml-jni");
            KANTVLog.d(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
            KANTVLog.j(TAG, "asr mode: " + mSettings.getASRMode());
            KANTVLog.g(TAG, "asr mode string: " + KANTVAIUtils.getASRModeString(mSettings.getASRMode()));
            if ((KANTVAIUtils.ASR_MODE_NORMAL == mSettings.getASRMode()) || (KANTVAIUtils.ASR_MODE_TRANSCRIPTION_RECORD == mSettings.getASRMode())) {
                result = ggmljava.asr_init(modelPath, mSettings.getASRThreadCounts(), KANTVAIUtils.ASR_MODE_NORMAL, ggmljava.HEXAGON_BACKEND_GGML);
            } else {
                result = ggmljava.asr_init(modelPath, mSettings.getASRThreadCounts(), KANTVAIUtils.ASR_MODE_PRESURETEST, ggmljava.HEXAGON_BACKEND_GGML);
            }
            KANTVUtils.setASRConfig("whispercpp", modelPath, asrThreadCounts + 1, asrMode);
            KANTVUtils.setTVASR(false);
            if (0 == result) {
                KANTVAIUtils.setASRSubsystemInit(true);
            } else {
                KANTVLog.j(TAG, "********************************************\n");
                KANTVLog.j(TAG, " pls check why failed to initialize ggml jni\n");
                KANTVLog.j(TAG, "********************************************\n");
            }
        } catch (Exception e) {
            KANTVLog.j(TAG, "********************************************\n");
            KANTVLog.j(TAG, " pls check why failed to initialize ggml jni: " + e.toString() + "\n");
            KANTVLog.j(TAG, "********************************************\n");
        }

        int thresoldsize = Integer.valueOf(mSettings.getThresholddisksize());
        KANTVLog.j(TAG, "threshold disk size:" + thresoldsize + "MBytes");
        KANTVUtils.setDiskThresholdFreeSize(thresoldsize);

        tmpString = jsonObject.getString("kantvServer");
        if (tmpString != null) {
            KANTVLog.j(TAG, "kantv server in config.json is:" + tmpString);
            KANTVLog.j(TAG, "kantv server in settings:" + mSettings.getKANTVServer());

            if (tmpString.equals(mSettings.getKANTVServer())) {
                KANTVUtils.updateKANTVServer(tmpString);
                KANTVLog.j(TAG, "kantv server:" + KANTVUtils.getKANTVServer());
                KANTVLog.j(TAG, "kantv update apk url:" + KANTVUtils.getKANTVUpdateAPKUrl());
                KANTVLog.j(TAG, "kantv update apk version url:" + KANTVUtils.getKANTVUpdateVersionUrl());
                KANTVLog.j(TAG, "kantv epg um server:" + KANTVUtils.getKanTVUMServer());
            } else {
                KANTVUtils.updateKANTVServer(mSettings.getKANTVServer());
                KANTVLog.j(TAG, "kantv server:" + KANTVUtils.getKANTVServer());
                KANTVLog.j(TAG, "kantv update apk url:" + KANTVUtils.getKANTVUpdateAPKUrl());
                KANTVLog.j(TAG, "kantv update apk version url:" + KANTVUtils.getKANTVUpdateVersionUrl());
                KANTVLog.j(TAG, "kantv epg um server:" + KANTVUtils.getKanTVUMServer());
            }
        } else {
            KANTVLog.j(TAG, "it shouldn't happen, pls check config.json");
            key = mContext.getString(R.string.pref_key_kantvserver);
            editor.putString(key, "www.kantvai.com");
            editor.commit();
            KANTVUtils.updateKANTVServer("www.kantvai.com"); //use default value
            KANTVLog.j(TAG, "kantv server:" + KANTVUtils.getKANTVServer());
            KANTVLog.j(TAG, "kantv update apk url:" + KANTVUtils.getKANTVUpdateAPKUrl());
            KANTVLog.j(TAG, "kantv update apk version url:" + KANTVUtils.getKANTVUpdateVersionUrl());
            KANTVLog.j(TAG, "kantv epg um server:" + KANTVUtils.getKanTVUMServer());
        }


        String rtmpServerUrl = jsonObject.getString("rtmpServerUrl");
        if (rtmpServerUrl != null) {
            KANTVLog.j(TAG, "rtmpServerUrl: " + rtmpServerUrl);
            key = mContext.getString(R.string.pref_key_rtmpserver);
            editor.putString(key, rtmpServerUrl);
            editor.commit();
        }


        KANTVLog.j(TAG, "dump mode           : " + mSettings.getDumpMode());
        KANTVUtils.setPlayEngine(mSettings.getPlayerEngine());
        KANTVLog.j(TAG, "play engine         : " + KANTVUtils.getPlayEngineName(mSettings.getPlayerEngine()));
        KANTVLog.j(TAG, "dump duration       : " + mSettings.getDumpDuration());

        KANTVUtils.setDumpDuration(Integer.valueOf(mSettings.getDumpDuration()));
        KANTVUtils.setDumpSize(Integer.valueOf(mSettings.getDumpSize()));
        KANTVUtils.setDumpCounts(Integer.valueOf(mSettings.getDumpCounts()));
        KANTVLog.j(TAG, "dump duration       : " + KANTVUtils.getDumpDuration());
        KANTVLog.j(TAG, "dump size           : " + KANTVUtils.getDumpSize());
        KANTVLog.j(TAG, "dump counts         : " + KANTVUtils.getDumpCounts());
        if (mSettings.getEnableWisePlay()) {
            KANTVLog.j(TAG, "drm scheme          : " + "wiseplay");
            KANTVUtils.setDrmScheme(KANTVUtils.DRM_SCHEME_WISEPLAY);
        } else {
            KANTVLog.j(TAG, "drm scheme          : " + "chinadrm");
            KANTVUtils.setDrmScheme(KANTVUtils.DRM_SCHEME_CHINADRM);
        }
        KANTVUtils.setApiGatewayServerUrl(mSettings.getApiGatewayServerUrl());
        KANTVLog.j(TAG, "API gateway          : " + KANTVUtils.getApiGatewayServerUrl());
        KANTVUtils.setNginxServerUrl(mSettings.getNginxServerUrl());
        KANTVLog.j(TAG, "nginx               : " + KANTVUtils.getNginxServerUrl());
        KANTVUtils.setEnableMultiDRM(mSettings.getEnableWisePlay());
        KANTVUtils.setEnableWisePlay(mSettings.getEnableWisePlay());
        KANTVLog.j(TAG, "enable WisePlay         : " + KANTVUtils.getEnableWisePlay());
        KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDrmScheme(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());


        if (KANTVUtils.DUMP_MODE_PERF_DECRYPT_TERMINAL == mSettings.getDumpMode()) {
            KANTVUtils.createPerfFile("/tmp/kantv_perf_teedecrypt.dat");
        }

        if (KANTVUtils.DUMP_MODE_DATA_DECRYPT_FILE == mSettings.getDumpMode()) {
            String filename = " ";
            String filenameInput = " ";
            if (KANTVUtils.PV_PLAYERENGINE__Exoplayer == mSettings.getPlayerEngine()) {
                filename = "/tmp/kantv_decrypt_exoplayer.dat";
                filenameInput = "/tmp/kantv_input_exoplayer.dat";
            } else if (KANTVUtils.PV_PLAYERENGINE__FFmpeg == mSettings.getPlayerEngine()) {
                filename = "/tmp/kantv_decrypt_ffmpeg.dat";
                filenameInput = "/tmp/kantv_input_ffmpeg.dat";
            } else {
                filename = "/tmp/kantv_decrypt_androimediaplayer.dat";
                filenameInput = "/tmp/kantv_input_androimediaplayer.dat";
            }
            KANTVUtils.createPerfFile(filename);
            KANTVUtils.createPerfFile(filenameInput);
        }


        //step-5
        KANTVUtils.umGetRecordInfo();

        //step-6
        {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    KANTVDRMManager.start(getBaseContext());
                }
            }).start();
        }

        //step-7
        KANTVUtils.umLauchApp();

        long endTime = System.currentTimeMillis();
        KANTVLog.j(TAG, "init app cost time " + (endTime - startTime) + " milliseconds\n\n\n\n");
    }

    public static native int kantv_anti_remove_rename_this_file();
}
