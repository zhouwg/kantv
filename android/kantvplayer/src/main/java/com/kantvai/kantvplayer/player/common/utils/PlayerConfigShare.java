package com.kantvai.kantvplayer.player.common.utils;

import android.content.Context;


public class PlayerConfigShare {
    private static Context _context;

    public static void initPlayerConfigShare(Context context){
        _context = context;
    }

    private static class ShareHolder{
        private static PlayerConfigShare playerConfigShare = new PlayerConfigShare();
    }

    private PlayerConfigShare(){

    }

    public static PlayerConfigShare getInstance(){
        return ShareHolder.playerConfigShare;
    }

    private SharedPreferencesUtil getShare() {
        return SharedPreferencesUtil.getInstance(_context, Constants.PLAYER_CONFIG);
    }

    public void setSubtitleTextSize(int textSize){
        getShare().saveSharedPreferences(Constants.SUBTITLE_SIZE, textSize);
    }

    public int getSubtitleTextSize(){
        int size = getShare().loadIntSharedPreference(Constants.SUBTITLE_SIZE);
        return size == 0 ? 50 : size;
    }

    public boolean isAllowOrientationChange(){
        return getShare().load(Constants.ORIENTATION_CHANGE, true);
    }

    public void setAllowOrientationChange(boolean isAllow){
        getShare().save(Constants.ORIENTATION_CHANGE, isAllow);
    }
}
