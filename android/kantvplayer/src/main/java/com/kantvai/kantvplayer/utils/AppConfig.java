package com.kantvai.kantvplayer.utils;

import com.blankj.utilcode.util.SPUtils;

//TODO: merge code to Settings.java
public class AppConfig {
    private String lastPlayPath = null;

    private static class ShareHolder {
        private static AppConfig appConfig = new AppConfig();
    }

    private AppConfig() {

    }

    public static AppConfig getInstance() {
        return ShareHolder.appConfig;
    }


    public int getFolderSortType() {
        String type = SPUtils.getInstance().getString(Constants.Config.FOLDER_COLLECTIONS, Constants.FolderSort.NAME_ASC + "");
        return Integer.valueOf(type);
    }

    public void saveFolderSortType(int type) {
        SPUtils.getInstance().put(Constants.Config.FOLDER_COLLECTIONS, type + "");
    }


    public String getDownloadFolder() {
        return SPUtils.getInstance().getString(Constants.Config.LOCAL_DOWNLOAD_FOLDER, Constants.DefaultConfig.downloadPath);
    }

    public void setDownloadFolder(String path) {
        SPUtils.getInstance().put(Constants.Config.LOCAL_DOWNLOAD_FOLDER, path);
    }


    public boolean isOpenMediaCodeC() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.SHARE_MEDIA_CODE_C);
    }

    public void setOpenMediaCodeC(boolean isUse) {
        SPUtils.getInstance().put(Constants.PlayerConfig.SHARE_MEDIA_CODE_C, isUse);
    }


    public boolean isOpenSLES() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.SHARE_OPEN_SLES);
    }

    public void setOpenSLES(boolean isUse) {
        SPUtils.getInstance().put(Constants.PlayerConfig.SHARE_OPEN_SLES, isUse);
    }


    public boolean isSurfaceRenders() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.SHARE_SURFACE_RENDERS, true);
    }

    public void setSurfaceRenders(boolean isUse) {
        SPUtils.getInstance().put(Constants.PlayerConfig.SHARE_SURFACE_RENDERS, isUse);
    }

    public int getPlayerType() {
        return SPUtils.getInstance().getInt(Constants.PlayerConfig.SHARE_PLAYER_TYPE, com.kantvai.kantvplayer.player.common.utils.Constants.PLAYERENGINE__FFmpeg);
    }

    public void setPlayerType(int type) {
        SPUtils.getInstance().put(Constants.PlayerConfig.SHARE_PLAYER_TYPE, type);
    }

    public String getPixelFormat() {
        return SPUtils.getInstance().getString(Constants.PlayerConfig.SHARE_PIXEL_FORMAT, Constants.PlayerConfig.PIXEL_OPENGL_ES2);
    }

    public void setPixelFormat(String pixelFormat) {
        SPUtils.getInstance().put(Constants.PlayerConfig.SHARE_PIXEL_FORMAT, pixelFormat);
    }

    public boolean isUseNetWorkSubtitle() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.USE_NETWORK_SUBTITLE, true);
    }

    public void setUseNetWorkSubtitle(boolean isUse) {
        SPUtils.getInstance().put(Constants.PlayerConfig.USE_NETWORK_SUBTITLE, isUse);
    }

    public boolean isAutoLoadLocalSubtitle() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.AUTO_LOAD_LOCAL_SUBTITLE);
    }

    public void setAutoLoadLocalSubtitle(boolean auto) {
        SPUtils.getInstance().put(Constants.PlayerConfig.AUTO_LOAD_LOCAL_SUBTITLE, auto);
    }

    public boolean isAutoLoadNetworkSubtitle() {
        return SPUtils.getInstance().getBoolean(Constants.PlayerConfig.AUTO_LOAD_NETWORK_SUBTITLE);
    }

    public void setAutoLoadNetworkSubtitle(boolean auto) {
        SPUtils.getInstance().put(Constants.PlayerConfig.AUTO_LOAD_NETWORK_SUBTITLE, auto);
    }


    public String getLastPlayVideo() {
        if (lastPlayPath == null) {
            lastPlayPath = SPUtils.getInstance().getString(Constants.Config.LAST_PLAY_VIDEO_PATH, "");
        }
        return lastPlayPath;
    }

    public void setLastPlayVideo(String videoPath) {
        lastPlayPath = videoPath;
        SPUtils.getInstance().put(Constants.Config.LAST_PLAY_VIDEO_PATH, videoPath);
    }


    public boolean isCloseSplashPage() {
        return SPUtils.getInstance().getBoolean(Constants.Config.CLOSE_SPLASH_PAGE);
    }

    public void setCloseSplashPage(boolean isClose) {
        SPUtils.getInstance().put(Constants.Config.CLOSE_SPLASH_PAGE, isClose);
    }
}
