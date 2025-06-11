package com.kantvai.kantvplayer.player.common.utils;

//TODO: remove this file and merge them to KANTVUtils.java
public final class Constants {
    public static final int INTENT_OPEN_SUBTITLE = 1001;

    public static final int INTENT_QUERY_SUBTITLE = 1002;

    public static final int INTENT_SELECT_SUBTITLE = 1003;

    public static final int INTENT_AUTO_SUBTITLE = 1004;

    public static final int INTENT_SAVE_CURRENT = 1005;

    public static final int INTENT_PLAY_FAILED = 1007;

    public static final int INTENT_PLAY_END = 1008;

    public static final int INTENT_PLAY_COMPLETE = 1009;

    static final String PLAYER_CONFIG = "player_config";

    public static final int PLAYERENGINE__FFmpeg             = 0;
    public static final int PLAYERENGINE__Exoplayer          = 1;
    public static final int PLAYERENGINE__AndroidMediaPlayer = 2;
    public static final int PLAYERENGINE__GSTREAMER          = 3;

    static final String SUBTITLE_SIZE = "subtitle_size";

    static final String ORIENTATION_CHANGE = "orientation_change";
}
