package com.cdeos.kantv.app;

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

import com.cdeos.kantv.BuildConfig;
import com.cdeos.kantv.R;
import com.cdeos.kantv.utils.Constants;
import com.cdeos.kantv.utils.Settings;
import com.tencent.bugly.Bugly;

import com.cdeos.kantv.ui.activities.SplashActivity;
import com.cdeos.kantv.ui.activities.personal.CrashActivity;
import com.cdeos.kantv.ui.weight.material.MaterialViewInflater;
import com.cdeos.kantv.utils.SoUtils;
import com.cdeos.kantv.utils.database.DataBaseManager;
import com.cdeos.kantv.utils.net.okhttp.CookiesManager;
import com.cdeos.kantv.player.common.utils.PlayerConfigShare;

import org.ggml.ggmljava;

import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import cat.ereza.customactivityoncrash.config.CaocConfig;
import cdeos.media.player.CDEAssetLoader;
import cdeos.media.player.CDELibraryLoader;
import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;
import cdeos.media.player.KANTVDRM;
import cdeos.media.player.KANTVDRMManager;
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
        CDEUtils.setReleaseMode(true);
        CDELog.j(TAG, "*************************enter initGlobal *********************************");
        CDELog.j(TAG, "buildTime: " + buildTime);
        CDELog.j(TAG, "init app");

        //step-1
        CDEUtils.dumpDeviceInfo();
        String uid = CDEUtils.getAndroidID(getBaseContext());
        KANTVDRMManager.setUid(uid);
        CDELog.j(TAG, "android id : " + uid);

        String wifimac = CDEUtils.getWifiMac();
        CDEUtils.setJavalayerUniqueID("mac:" + wifimac + "androidid:" + uid);
        CDELog.j(TAG, "phone wifi mac:" + wifimac);

        CDELog.j(TAG, "device clear id:" + KANTVDRM.getInstance().ANDROID_JNI_GetDeviceClearIDString());
        CDELog.j(TAG, "device  id:" + KANTVDRM.getInstance().ANDROID_JNI_GetDeviceIDString());

        if (!CDEUtils.getReleaseMode())
            CDELog.j(TAG, "device unique id: " + CDEUtils.getUniqueID());

        CDEUtils.getDataPath();
        Constants.DefaultConfig.initDataPath();

        mContext = this.getBaseContext();
        mSettings = new Settings(getApplicationContext());
        //mSettings.updateUILang(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);


        try {
            WifiManager wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi.getConnectionInfo();
            CDELog.j(TAG, "wifi ip:" + info.getIpAddress());
            CDELog.j(TAG, "wifi mac:" + info.getMacAddress());
            CDELog.j(TAG, "wifi name:" + info.getSSID());
        } catch (Exception e) {
            CDELog.j(TAG, "can't get wifi info: " + e.toString());
        }

        //step-2
        Intent intent = new Intent(Intent.ACTION_MAIN, null);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        List<ResolveInfo> packageInfos = getPackageManager().queryIntentActivities(intent, 0);
        int indexKanTV = 0;
        for (int i = packageInfos.size() - 1; i >= 0; i--) {
            String launcherActivityName = packageInfos.get(i).activityInfo.name;
            String packageName = packageInfos.get(i).activityInfo.packageName;
            CDELog.i(TAG, i + " -- launcherActivityName: " + launcherActivityName);
            CDELog.e(TAG, i + " -- packageName: " + packageName);
            if (packageName.equals("com.cdeos.kantv")) {
                indexKanTV = i;
                break;
            }
        }
        String launcherName = packageInfos.get(indexKanTV).activityInfo.name;
        CDELog.d(TAG, "packageName: " + packageInfos.get(indexKanTV).activityInfo.packageName + " ,launcherActivityName: " + launcherName);


        //step-3: asset files
        KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
        mKANTVDRM.ANDROID_JNI_Init(mContext, CDEUtils.getDataPath(mContext));
        mKANTVDRM.ANDROID_JNI_SetLocalEMS(CDEUtils.getLocalEMS());
        CDEAssetLoader.copyAssetFile(mContext, "config.json", CDEAssetLoader.getDataPath(mContext) + "config.json");
        CDEAssetLoader.copyAssetFile(mContext, "ggml-hexagon.cfg", CDEAssetLoader.getDataPath(mContext) + "ggml-hexagon.cfg");
        CDEAssetLoader.copyAssetFile(mContext, "libggmlop_skel.so", CDEAssetLoader.getDataPath(mContext) + "libggmlop_skel.so");

        CDEUtils.copyAssetFile(mContext, "res/apple.png", CDEUtils.getDataPath(mContext) + "apple.png");
        CDEUtils.copyAssetFile(mContext, "res/colorkey.png", CDEUtils.getDataPath(mContext) + "colorkey.png");
        CDEUtils.copyAssetFile(mContext, "res/line.png", CDEUtils.getDataPath(mContext) + "line.png");
        CDEUtils.copyAssetFile(mContext, "res/logo.png", CDEUtils.getDataPath(mContext) + "logo.png");
        CDEUtils.copyAssetFile(mContext, "res/png1.png", CDEUtils.getDataPath(mContext) + "png1.png");
        CDEUtils.copyAssetFile(mContext, "res/png2.png", CDEUtils.getDataPath(mContext) + "png2.png");
        CDEUtils.copyAssetFile(mContext, "res/simhei.ttf", CDEUtils.getDataPath(mContext) + "simhei.ttf");

        //copy asset files to /sdcard/kantv/, this file is needed for ASR benchmark
        String ggmlSampleFileName = "jfk.wav";
        CDEAssetLoader.copyAssetFile(mContext, ggmlSampleFileName, CDEUtils.getDataPath() + ggmlSampleFileName);

        CDEAssetLoader.copyAssetFile(mContext, "mnist-5.png", CDEUtils.getDataPath() + "mnist-5.png");
        CDEAssetLoader.copyAssetFile(mContext, "mnist-7.png", CDEUtils.getDataPath() + "mnist-7.png");


        //for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
        //TODO: fix following issue and then modify from
        //      CDEAssetLoader.copyAssetDir(mContext, "qnnlib", CDEUtils.getDataPath(mContext) + "qnnlib");
        //      to
        //      CDEAssetLoader.copyAssetDir(mContext, "qnnlib", CDEUtils.getDataPath() + "qnnlib");
        //      or
        //      move assets/qnnlib to /sdcard/kantv/qnnlib manually for purpose of reduce size of APK because size of prebuilt qnnlibs is about 49M
        //   QNN issue:
        //   can not open QNN library /sdcard/kantv/qnnlib/libQnnSystem.so,
        //   error: dlopen failed: library "/sdcard/kantv/qnnlib/libQnnSystem.so"
        //   needed or dlopened by "/data/app/~~clbTlTogBUHAPF5Da52Cfw==/com.cdeos.kantv-k2X0NpXfzg9uT10HNFGVDQ==/base.apk!/lib/arm64-v8a/libggml-jni.so" is not accessible for the namespace "clns-4"
        CDEAssetLoader.copyAssetDir(mContext, "qnnlib", CDEUtils.getDataPath(mContext) + "qnnlib");
        CDEAssetLoader.copyAssetDir(mContext, "qnnlib", "/data/local/tmp/");
        CDELog.j(TAG, "qnn lib path:" + CDEUtils.getDataPath(mContext) + "qnnlib");

        //note: move assets/models to /sdcard/kantv/models manually
        //     for purpose of reduce size of APK, the APK size would be smaller significantly
        CDEAssetLoader.copyAssetDir(mContext, "models", CDEUtils.getDataPath() + "/models");
        CDEAssetLoader.copyAssetFile(mContext, "/models/ggml-tiny.en-q8_0.bin", CDEUtils.getSDCardDataPath() + "ggml-tiny.en-q8_0.bin");

        //step-4:
        String configString = CDEAssetLoader.readTextFromFile(CDEAssetLoader.getDataPath(mContext) + "config.json");
        JSONObject jsonObject = JSON.parseObject(configString);
        CDELog.j(TAG, "Config: kantvServer: " + jsonObject.getString("kantvServer"));
        CDELog.j(TAG, "Config: rtmpServer: " + jsonObject.getString("rtmpServerUrl"));
        CDELog.j(TAG, "Config: provision URL: " + jsonObject.getString("provisionUrl"));
        CDELog.d(TAG, "Config: usingTEE: " + jsonObject.getString("usingTEE"));
        CDELog.d(TAG, "Config: troubleshootingMode: " + jsonObject.getString("troubleshootingMode"));
        CDELog.j(TAG, "Config: releaseMode: " + jsonObject.getString("releaseMode"));
        CDELog.j(TAG, "Config: apkVersion: " + jsonObject.getString("apkVersion"));
        CDELog.j(TAG, "Config: apkForTV: " + jsonObject.getString("apkForTV"));


        SharedPreferences.Editor editor = mSharedPreferences.edit();
        String key = mContext.getString(R.string.pref_key_version);
        String apkVersionString = jsonObject.getString("apkVersion");
        if (apkVersionString != null) {
            CDEUtils.setKANTVAPKVersion(apkVersionString);
            if (key != null) {
                CDELog.j(TAG, "current version: " + key);
                key = mContext.getString(R.string.pref_key_version);
                editor.putString(key, apkVersionString);
                editor.commit();
                CDELog.j(TAG, "app version:" + apkVersionString);
            } else {
                CDELog.j(TAG, "no app version info in pref store");
            }
        } else {
            CDELog.j(TAG, "can't find app version info in config file, it should not happen, pls check why?\n");
        }

        String apkForTVString = jsonObject.getString("apkForTV");
        if (apkForTVString != null) {
            int apkForTV = Integer.valueOf(apkForTVString);
            CDELog.j(TAG, "apk for TV: " + apkForTV);
            CDEUtils.setAPKForTV((1 == apkForTV) ? true : false);
        } else {
            CDEUtils.setAPKForTV(false);
        }

        // releaseMode:disable/enable log
        // troubleshootingMode: internal development mode
        // expertMode:whether user is IT worker or expert
        String releaseModeString = jsonObject.getString("releaseMode");
        if (releaseModeString != null) {
            int releaseMode = Integer.valueOf(releaseModeString);
            CDELog.j(TAG, "releaseMode: " + releaseMode);
            CDEUtils.setReleaseMode((1 == releaseMode) ? true : false);
        } else {
            CDEUtils.setReleaseMode(true);
        }
        CDELog.j(TAG, "qnn lib path        : " + CDEUtils.getDataPath(mContext) + "qnnlib");
        CDELog.j(TAG, "dev  mode           : " + mSettings.getDevMode());
        CDEUtils.setExpertMode(mSettings.getDevMode());


        CDEUtils.setTroubleshootingMode(CDEUtils.TROUBLESHOOTING_MODE_DISABLE);
        int troubleshootingMode = CDEUtils.TROUBLESHOOTING_MODE_DISABLE;
        if (jsonObject.getString("troubleshootingMode") != null) {
            troubleshootingMode = Integer.valueOf(jsonObject.getString("troubleshootingMode"));
        }

        CDEUtils.setTroubleshootingMode(CDEUtils.TROUBLESHOOTING_MODE_DISABLE);

        if (true) {
            key = mContext.getString(R.string.pref_key_using_tee);
            editor.putBoolean(key, false);
            editor.commit();
            CDEUtils.setDecryptMode(CDEUtils.DECRYPT_SOFT);
        }

        CDEUtils.setUsingFFmpegCodec(mSettings.getUsingFFmpegCodec());
        CDELog.j(TAG, "using ffmpeg codec for audio:" + CDEUtils.getUsingFFmpegCodec());

        String startPlayPosString = mSettings.getStartPlaypos();
        CDELog.d(TAG, "start play pos: " + startPlayPosString + " minutes");
        int settingStartPlayPos = 0;

        if (startPlayPosString != null)
            settingStartPlayPos = Integer.valueOf(startPlayPosString);
        CDELog.d(TAG, "start play pos: " + settingStartPlayPos + " minutes");

        String tmpString = jsonObject.getString("startPlayPos");
        int configStartPlayPos = 0;
        if (tmpString != null)
            configStartPlayPos = Integer.valueOf(tmpString);

        if (0 == configStartPlayPos) {
            CDEUtils.setStartPlayPos(settingStartPlayPos);
        } else {
            CDEUtils.setStartPlayPos(configStartPlayPos);
        }
        CDELog.j(TAG, "startPlayPos: " + CDEUtils.getStartPlayPos() + " minutes");
        String localEMS = jsonObject.getString("localEMS");
        if (localEMS != null)
            CDEUtils.setLocalEMS(localEMS);
        CDEUtils.setDisableAudioTrack(mSettings.getAudioDisabled());
        CDEUtils.setDisableVideoTrack(mSettings.getVideoDisabled());
        CDEUtils.setEnableDumpVideoES(mSettings.getEnableDumpVideoES());
        CDEUtils.setEnableDumpAudioES(mSettings.getEnableDumpAudioES());


        CDELog.j(TAG, "disable audio: " + CDEUtils.getDisableAudioTrack());
        CDELog.j(TAG, "disable video: " + CDEUtils.getDisableVideoTrack());
        CDELog.j(TAG, "enable dump video es: " + CDEUtils.getEnableDumpVideoES());
        CDELog.j(TAG, "enable dump audio es: " + CDEUtils.getEnableDumpAudioES());
        CDELog.j(TAG, "continued playback  : " + mSettings.getContinuedPlayback());

        int recordMode = mSettings.getRecordMode();  //default is both video & audio
        int recordFormat = mSettings.getRecordFormat();//default is mp4
        int recordCodec = mSettings.getRecordCodec(); //default is h264

        CDEUtils.setRecordConfig(CDEUtils.getDataPath(), recordMode, recordFormat, recordCodec, mSettings.getRecordDuration(), mSettings.getRecordSize());
        CDEUtils.setTVRecording(false);

        int asrMode = mSettings.getASRMode();  //default is normal transcription
        int asrThreadCounts = mSettings.getASRThreadCounts(); //default is 4
        CDELog.j(TAG, "GGML mode: " + mSettings.getGGMLMode());
        CDELog.j(TAG, "GGML mode name: " + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()));
        String modelPath = CDEUtils.getDataPath() + "/models/" + "ggml-" + CDEUtils.getGGMLModeString(mSettings.getGGMLMode()) + ".bin";
        CDELog.j(TAG, "modelPath:" + modelPath);

        //preload GGML model and initialize asr_subsystem as early as possible for purpose of ASR real-time performance
        try {
            int result = 0;
            CDELibraryLoader.load("ggml-jni");
            CDELog.d(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
            CDELog.j(TAG, "asr mode: " + mSettings.getASRMode());
            if ((CDEUtils.ASR_MODE_NORMAL == mSettings.getASRMode()) || (CDEUtils.ASR_MODE_TRANSCRIPTION_RECORD == mSettings.getASRMode())) {
                result = ggmljava.asr_init(modelPath, mSettings.getASRThreadCounts(), CDEUtils.ASR_MODE_NORMAL, CDEUtils.HEXAGON_BACKEND_GGML);
            } else {
                result = ggmljava.asr_init(modelPath, mSettings.getASRThreadCounts(), CDEUtils.ASR_MODE_PRESURETEST, CDEUtils.HEXAGON_BACKEND_GGML);
            }
            CDEUtils.setASRConfig("whispercpp", modelPath, asrThreadCounts + 1, asrMode);
            CDEUtils.setTVASR(false);
            if (0 == result) {
                CDEUtils.setASRSubsystemInit(true);
            } else {
                CDELog.j(TAG, "********************************************\n");
                CDELog.j(TAG, " pls check why failed to initialize ggml jni\n");
                CDELog.j(TAG, "********************************************\n");
            }
        } catch (Exception e) {
            CDELog.j(TAG, "********************************************\n");
            CDELog.j(TAG, " pls check why failed to initialize ggml jni: " + e.toString() + "\n");
            CDELog.j(TAG, "********************************************\n");
        }

        int thresoldsize = Integer.valueOf(mSettings.getThresholddisksize());
        CDELog.j(TAG, "threshold disk size:" + thresoldsize + "MBytes");
        CDEUtils.setDiskThresholdFreeSize(thresoldsize);

        tmpString = jsonObject.getString("kantvServer");
        if (tmpString != null) {
            CDELog.j(TAG, "kantv server in config.json is:" + tmpString);
            CDELog.j(TAG, "kantv server in settings:" + mSettings.getKANTVServer());

            if (tmpString.equals(mSettings.getKANTVServer())) {
                CDEUtils.updateKANTVServer(tmpString);
                CDELog.j(TAG, "kantv server:" + CDEUtils.getKANTVServer());
                CDELog.j(TAG, "kantv update apk url:" + CDEUtils.getKANTVUpdateAPKUrl());
                CDELog.j(TAG, "kantv update apk version url:" + CDEUtils.getKANTVUpdateVersionUrl());
                CDELog.j(TAG, "kantv epg um server:" + CDEUtils.getKanTVUMServer());
            } else {
                CDEUtils.updateKANTVServer(mSettings.getKANTVServer());
                CDELog.j(TAG, "kantv server:" + CDEUtils.getKANTVServer());
                CDELog.j(TAG, "kantv update apk url:" + CDEUtils.getKANTVUpdateAPKUrl());
                CDELog.j(TAG, "kantv update apk version url:" + CDEUtils.getKANTVUpdateVersionUrl());
                CDELog.j(TAG, "kantv epg um server:" + CDEUtils.getKanTVUMServer());
            }
        } else {
            CDELog.j(TAG, "it shouldn't happen, pls check config.json");
            key = mContext.getString(R.string.pref_key_kantvserver);
            editor.putString(key, "www.cdeos.com");
            editor.commit();
            CDEUtils.updateKANTVServer("www.cdeos.com"); //use default value
            CDELog.j(TAG, "kantv server:" + CDEUtils.getKANTVServer());
            CDELog.j(TAG, "kantv update apk url:" + CDEUtils.getKANTVUpdateAPKUrl());
            CDELog.j(TAG, "kantv update apk version url:" + CDEUtils.getKANTVUpdateVersionUrl());
            CDELog.j(TAG, "kantv epg um server:" + CDEUtils.getKanTVUMServer());
        }


        String rtmpServerUrl = jsonObject.getString("rtmpServerUrl");
        if (rtmpServerUrl != null) {
            CDELog.j(TAG, "rtmpServerUrl: " + rtmpServerUrl);
            key = mContext.getString(R.string.pref_key_rtmpserver);
            editor.putString(key, rtmpServerUrl);
            editor.commit();
        }


        CDELog.j(TAG, "dump mode           : " + mSettings.getDumpMode());
        CDEUtils.setPlayEngine(mSettings.getPlayerEngine());
        CDELog.j(TAG, "play engine         : " + CDEUtils.getPlayEngineName(mSettings.getPlayerEngine()));
        CDELog.j(TAG, "dump duration       : " + mSettings.getDumpDuration());

        CDEUtils.setDumpDuration(Integer.valueOf(mSettings.getDumpDuration()));
        CDEUtils.setDumpSize(Integer.valueOf(mSettings.getDumpSize()));
        CDEUtils.setDumpCounts(Integer.valueOf(mSettings.getDumpCounts()));
        CDELog.j(TAG, "dump duration       : " + CDEUtils.getDumpDuration());
        CDELog.j(TAG, "dump size           : " + CDEUtils.getDumpSize());
        CDELog.j(TAG, "dump counts         : " + CDEUtils.getDumpCounts());
        if (mSettings.getEnableWisePlay()) {
            CDELog.j(TAG, "drm scheme          : " + "wiseplay");
            CDEUtils.setDrmScheme(CDEUtils.DRM_SCHEME_WISEPLAY);
        } else {
            CDELog.j(TAG, "drm scheme          : " + "chinadrm");
            CDEUtils.setDrmScheme(CDEUtils.DRM_SCHEME_CHINADRM);
        }
        CDEUtils.setApiGatewayServerUrl(mSettings.getApiGatewayServerUrl());
        CDELog.j(TAG, "API gateway          : " + CDEUtils.getApiGatewayServerUrl());
        CDEUtils.setNginxServerUrl(mSettings.getNginxServerUrl());
        CDELog.j(TAG, "nginx               : " + CDEUtils.getNginxServerUrl());
        CDEUtils.setEnableMultiDRM(mSettings.getEnableWisePlay());
        CDEUtils.setEnableWisePlay(mSettings.getEnableWisePlay());
        CDELog.j(TAG, "enable WisePlay         : " + CDEUtils.getEnableWisePlay());
        CDEUtils.setDevMode(CDEUtils.getTEEConfigFilePath(mContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), CDEUtils.getDrmScheme(), CDEUtils.getDumpDuration(), CDEUtils.getDumpSize(), CDEUtils.getDumpCounts());


        if (CDEUtils.DUMP_MODE_PERF_DECRYPT_TERMINAL == mSettings.getDumpMode()) {
            CDEUtils.createPerfFile("/tmp/kantv_perf_teedecrypt.dat");
        }

        if (CDEUtils.DUMP_MODE_DATA_DECRYPT_FILE == mSettings.getDumpMode()) {
            String filename = " ";
            String filenameInput = " ";
            if (CDEUtils.PV_PLAYERENGINE__Exoplayer == mSettings.getPlayerEngine()) {
                filename = "/tmp/kantv_decrypt_exoplayer.dat";
                filenameInput = "/tmp/kantv_input_exoplayer.dat";
            } else if (CDEUtils.PV_PLAYERENGINE__FFmpeg == mSettings.getPlayerEngine()) {
                filename = "/tmp/kantv_decrypt_ffmpeg.dat";
                filenameInput = "/tmp/kantv_input_ffmpeg.dat";
            } else {
                filename = "/tmp/kantv_decrypt_androimediaplayer.dat";
                filenameInput = "/tmp/kantv_input_androimediaplayer.dat";
            }
            CDEUtils.createPerfFile(filename);
            CDEUtils.createPerfFile(filenameInput);
        }


        //step-5
        CDEUtils.umGetRecordInfo();

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
        CDEUtils.umLauchApp();

        long endTime = System.currentTimeMillis();
        CDELog.j(TAG, "init app cost time " + (endTime - startTime) + " milliseconds\n\n\n\n");
    }

    public static native int kantv_anti_remove_rename_this_file();
}
