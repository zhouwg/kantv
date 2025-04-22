package com.kantvai.kantvplayer.utils;

import android.os.Build;
import android.os.Environment;

import com.kantvai.kantvplayer.mvp.impl.MainPresenterImpl;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import kantvai.media.player.KANTVLog;

//TODO: merge code to KANTVUtils.java
public class Constants {

    public static class PlayerConfig {

        static final String SHARE_PLAYER_TYPE = "player_type";

        static final String SHARE_PIXEL_FORMAT = "pixel_format";

        static final String SHARE_MEDIA_CODE_C = "media_code_c";

        static final String SHARE_MEDIA_CODE_C_H265 = "media_code_c_h265";

        static final String SHARE_OPEN_SLES = "open_sles";

        static final String SHARE_SURFACE_RENDERS = "surface_renders";

        static final String USE_NETWORK_SUBTITLE = "use_network_subtitle";

        static final String AUTO_LOAD_LOCAL_SUBTITLE = "auto_load_local_subtitle";

        static final String AUTO_LOAD_NETWORK_SUBTITLE = "auto_load_network_subtitle";

        public static final String PIXEL_AUTO = "";
        public static final String PIXEL_RGB565 = "fcc-rv16";
        public static final String PIXEL_RGB888 = "fcc-rv24";
        public static final String PIXEL_RGBX8888 = "fcc-rv32";
        public static final String PIXEL_YV12 = "fcc-yv12";
        public static final String PIXEL_OPENGL_ES2 = "fcc-_es2";
    }


    public static class Config {

        static final String LOCAL_DOWNLOAD_FOLDER = "local_download_folder";

        static final String FOLDER_COLLECTIONS = "folder_collection";

        static final String LAST_PLAY_VIDEO_PATH = "last_play_video_path";

        static final String CLOSE_SPLASH_PAGE = "close_splash_page";
    }


    public static class DefaultConfig {
        private static final String TAG = DefaultConfig.class.getName();

        public static final String SYSTEM_VIDEO_PATH = "System Video";

        public static String downloadPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/kantv";

        public static String cacheFolderPath = downloadPath + "/.cache";

        public static String configPath = downloadPath + "/_config/config.txt";

        public static final String subtitleFolder = "/_zimu";

        public static String imageFolder = downloadPath + "/_image";


        public static void initDataPath() {
            KANTVLog.j(TAG, "ANDROID SDK:" + String.valueOf(android.os.Build.VERSION.SDK_INT));
            KANTVLog.j(TAG, "OS Version:" + android.os.Build.VERSION.RELEASE);
            String state = Environment.getExternalStorageState();
            KANTVLog.j(TAG, "Environment.getExternalStorageState() state: " + state);
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
                KANTVLog.j(TAG, "Android version > API22：Android 5.1 (Android L)Lollipop_MR1");
                if (!state.equals(Environment.MEDIA_MOUNTED)) {
                    KANTVLog.d(TAG, "can't read/write extern device");
                    return;
                }
            } else {
                KANTVLog.j(TAG, "Android version <= API22：Android 5.1 (Android L)Lollipop");
            }

            File sdcardPath = Environment.getExternalStorageDirectory();
            KANTVLog.j(TAG, "sdcardPath name:" + sdcardPath.getName() + ",sdcardPath:" + sdcardPath.getPath());
            String dataDirectoryPath = null;
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
                dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "kantv";
            } else {
                dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
                        File.separator + "Android" + File.separator + "data" + File.separator + "com.kantvai.kantvplayer" + File.separator + "files" + File.separator + "kantv";
            }
            KANTVLog.j(TAG, "dataDirectoryPath: " + dataDirectoryPath);

            downloadPath = dataDirectoryPath;
            cacheFolderPath = downloadPath + "/.cache";
            configPath = downloadPath + "/_config/config.txt";
            imageFolder = downloadPath + "/_image";
            KANTVLog.j(TAG, "downloadPath:" + downloadPath);
            KANTVLog.j(TAG, "cacheFolderPath:" + cacheFolderPath);
            KANTVLog.j(TAG, "configPath:" + configPath);
            KANTVLog.j(TAG, "imageFolder:" + imageFolder);
        }
    }


    public static class FolderSort {
        public static final int NAME_ASC = 1;
        public static final int NAME_DESC = 2;
        public static final int DURATION_ASC = 3;
        public static final int DURATION_DESC = 4;
        public static final int GENERATETIME_ASC = 5;
        public static final int GENERATETIME_DESC = 6;
    }


    public static class ScanType {
        public static final int BLOCK = 0;
        public static final int SCAN = 1;
    }
}
