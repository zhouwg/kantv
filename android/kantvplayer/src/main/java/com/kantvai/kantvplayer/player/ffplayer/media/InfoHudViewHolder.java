/*
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
 * Copyright (c) Project KanTV. 2021-2023
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
package com.kantvai.kantvplayer.player.ffplayer.media;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.Debug;
import android.os.Handler;
import android.os.Message;
import android.util.SparseArray;
import android.view.View;
import android.widget.TableLayout;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.Settings;

import java.util.Date;
import java.util.Locale;
import java.text.SimpleDateFormat;

import kantvai.media.player.AndroidMediaPlayer;
import kantvai.media.player.IMediaPlayer;
import kantvai.media.player.KANTVDRM;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.FFmpegMediaPlayer;
import kantvai.media.player.MediaPlayerProxy;


import kantvai.media.player.KANTVUtils;
import kantvai.media.player.misc.IMediaFormat;
import kantvai.media.player.misc.ITrackInfo;
import kantvai.media.player.misc.IjkMediaFormat;

public class InfoHudViewHolder {
    private final static String TAG = InfoHudViewHolder.class.getName();
    private TableLayoutBinder mTableLayoutBinder;
    private SparseArray<View> mRowMap = new SparseArray<View>();
    private IMediaPlayer mMediaPlayer;
    private long mLoadCost = 0;
    private long mSeekCost = 0;
    private String mBoardInfo;
    private String mVideoTitle;
    private AppCompatActivity mActivity;
    private KANTVVideoView mVideoView;
    private Settings mSettings;

    public InfoHudViewHolder(Context context, TableLayout tableLayout, AppCompatActivity activity) {
        mSettings = new Settings(context.getApplicationContext());
        mTableLayoutBinder = new TableLayoutBinder(context, tableLayout);
        StringBuilder  boardInfo = new StringBuilder();
        boardInfo.append("Device:" + Build.DEVICE + " ABI:" + Build.CPU_ABI + " Product:" + Build.PRODUCT);
        mBoardInfo = boardInfo.toString();
        mBoardInfo = KANTVUtils.getDeviceName();
        mSettings.updateUILang(activity);
        KANTVLog.v(TAG,"board info: " + mBoardInfo);
    }

    private void appendSection(int nameId) {
        mTableLayoutBinder.appendSection(nameId);
    }

    private void appendRow(int nameId) {
        View rowView = mTableLayoutBinder.appendRow2(nameId, null);
        mRowMap.put(nameId, rowView);
    }

    private void setRowValue(int id, String value) {
        View rowView = mRowMap.get(id);
        if (rowView == null) {
            rowView = mTableLayoutBinder.appendRow2(id, value);
            mRowMap.put(id, rowView);
        } else {
            mTableLayoutBinder.setValueText(rowView, value);
        }
    }

    public void setVideoActivity(AppCompatActivity videoActivity) {
        mActivity = videoActivity;
    }

    public void setActivity(AppCompatActivity activity) {
        mActivity = activity;
    }
    public void setVideoView(KANTVVideoView videoView) {
        mVideoView = videoView;
    }

    public void setVideoTitle(String videoTitle) {
        mVideoTitle = videoTitle;
    }

    public void setMediaPlayer(IMediaPlayer mp) {
        mMediaPlayer = mp;
        if (mMediaPlayer != null) {
            mHandler.sendEmptyMessageDelayed(MSG_UPDATE_HUD, 500);
        } else {
            mHandler.removeMessages(MSG_UPDATE_HUD);
        }
    }


    public void updateLoadCost(long time)  {
        mLoadCost = time;
    }

    public void updateSeekCost(long time)  {
        mSeekCost = time;
    }

    public void setVisibility (int visibility) {
        mTableLayoutBinder.setVisibility(visibility);
    }

    public void setBackgroundColor (int color) {
        mTableLayoutBinder.setBackgroundColor(color);
    }

    private static String formatedSize(long bytes) {
        if (bytes >= 100 * 1000) {
            return String.format(Locale.US, "%.2f MB", ((float)bytes) / 1000 / 1000);
        } else if (bytes >= 100) {
            return String.format(Locale.US, "%.1f KB", ((float)bytes) / 1000);
        } else {
            return String.format(Locale.US, "%d B", bytes);
        }
    }

    private static final int MSG_UPDATE_HUD = 1;
    //private Handler mHandler = new Handler() {
    private Handler mHandler = new Handler(new Handler.Callback() {
        @RequiresApi(api = Build.VERSION_CODES.KITKAT)
        @Override
        public boolean handleMessage(@NonNull Message msg) {
            switch (msg.what) {
                case MSG_UPDATE_HUD: {
                    InfoHudViewHolder holder = InfoHudViewHolder.this;
                    FFmpegMediaPlayer mp = null;
                    mSettings.updateUILang(mActivity);
                    if (mMediaPlayer == null)
                        break;
                    if (mMediaPlayer instanceof FFmpegMediaPlayer) {
                        mp = (FFmpegMediaPlayer) mMediaPlayer;
                    } else if (mMediaPlayer instanceof MediaPlayerProxy) {
                        MediaPlayerProxy proxy = (MediaPlayerProxy) mMediaPlayer;
                        IMediaPlayer internal = proxy.getInternalMediaPlayer();
                        if (internal != null && internal instanceof FFmpegMediaPlayer)
                            mp = (FFmpegMediaPlayer) internal;
                    }
                    if (mp == null) {
                        //break;
                    }
                    String url = mMediaPlayer.getDataSource();
                    String tmpString = KANTVUtils.convertLongString(url, 30);
                    //setRowValue(R.string.development_mode, "");
                    if (!KANTVUtils.getReleaseMode()) {
                        //don't display real url in both debug & release mode
                        //setRowValue(R.string.url, tmpString);
                    }
                    long playTime            = mMediaPlayer.getPlayTime();
                    long pauseTime           = mMediaPlayer.getPauseTime();

                    setRowValue(R.string.name, mVideoTitle);
                    //setRowValue(R.string.mi_resolution, KANTVUtils.buildResolution(mMediaPlayer.getVideoWidth(), mMediaPlayer.getVideoHeight(), mMediaPlayer.getVideoSarNum(), mMediaPlayer.getVideoSarDen()));
                    tmpString = KANTVUtils.buildResolution(mMediaPlayer.getVideoWidth(), mMediaPlayer.getVideoHeight(), mMediaPlayer.getVideoSarNum(), mMediaPlayer.getVideoSarDen());
                    if (!(mMediaPlayer instanceof AndroidMediaPlayer)) {
                        if (!KANTVUtils.getReleaseMode())
                        {
                            tmpString += "( " + mActivity.getBaseContext().getString(R.string.disable_audiotrack) + ":" + (KANTVUtils.getDisableAudioTrack() ? "YES" : "NO");
                            tmpString += "  " + mActivity.getBaseContext().getString(R.string.disable_videotrack) + ":" + (KANTVUtils.getDisableVideoTrack() ? "YES" : "NO") + " )";

                        }
                    }
                    tmpString = KANTVUtils.convertLongString(tmpString, 30);
                    setRowValue(R.string.mi_resolution, tmpString);

                    if (mMediaPlayer instanceof FFmpegMediaPlayer) {
                        int vdec = mp.getVideoDecoder();
                        switch (vdec) {
                            case FFmpegMediaPlayer.FFP_PROPV_DECODER_AVCODEC:
                                //setRowValue(R.string.vdec, "avcodec");
                                tmpString = "FFmpeg";
                                break;
                            case FFmpegMediaPlayer.FFP_PROPV_DECODER_MEDIACODEC:
                                //setRowValue(R.string.vdec, "MediaCodec");
                                tmpString = "MediaCodec";
                                break;
                            default:
                                //setRowValue(R.string.vdec, "unknown");
                                tmpString = "unknown";
                                break;
                        }
                        //setRowValue(R.string.playengine, "FFmpeg");
                        tmpString = "SDK:" + KANTVDRM.getInstance().ANDROID_JNI_GetSDKVersion() +
                                " "  + mMediaPlayer.getVideoDecoderName() +
                                " "  + mActivity.getBaseContext().getString(R.string.playengine) +
                                ": " + "FFmpeg-" + ((FFmpegMediaPlayer) mMediaPlayer).getFFmpegVersion();
                        tmpString = KANTVUtils.convertLongString(tmpString, 30);
                        setRowValue(R.string.vdec, tmpString);

                        float fpsOutput = mp.getVideoOutputFramesPerSecond();
                        float fpsDecode = mp.getVideoDecodeFramesPerSecond();
                        String fpsString = String.format(Locale.US, "%.2f / %.2f", fpsDecode, fpsOutput);
                        //setRowValue(R.string.fps, fpsString);

                        String bitrateString = null;

                        {
                            long videoCachedDuration = mp.getVideoCachedDuration();
                            long audioCachedDuration = mp.getAudioCachedDuration();
                            long videoCachedBytes = mp.getVideoCachedBytes();
                            long audioCachedBytes = mp.getAudioCachedBytes();
                            long totalBytes = mp.getTrafficStatisticByteCount();
                            long tcpSpeed = mp.getTcpSpeed();
                            long bitRate = mp.getBitRate();
                            float playbackRate = mp.getPlaybackSpeed();
                            KANTVLog.d(TAG, "bitrate: " + bitRate);
                            KANTVLog.d(TAG, "playback rate: " + playbackRate);
                            long seekLoadDuration = mp.getSeekLoadDuration();
                            setRowValue(R.string.v_cache, String.format(Locale.US, "%s, %s", KANTVUtils.formatedDurationMilli(videoCachedDuration), formatedSize(videoCachedBytes)));
                            setRowValue(R.string.a_cache, String.format(Locale.US, "%s, %s", KANTVUtils.formatedDurationMilli(audioCachedDuration), formatedSize(audioCachedBytes)));
                            //TODO
                            //setRowValue(R.string.tcp_speed, String.format(Locale.US, "%s", formatedSpeed(tcpSpeed, 1000)));
                            if (mMediaPlayer.isClearContent()) {
                                if (bitRate > 0)
                                    //setRowValue(R.string.bit_rate, String.format(Locale.US, "%.2f Kb/s", bitRate / 1000f));
                                    bitrateString = String.format(Locale.US, "%.2f Kb/s", bitRate / 1000f);
                                else
                                    //setRowValue(R.string.bit_rate, String.format(Locale.US, "%s", KANTVUtils.formatedBitRate(totalBytes, (playTime + pauseTime) * 1000)));
                                    bitrateString = String.format(Locale.US, "%s", KANTVUtils.formatedBitRate(totalBytes, (playTime + pauseTime) * 1000));
                            } else {
                                //setRowValue(R.string.bit_rate, String.format(Locale.US, "%s", KANTVUtils.formatedBitRate(totalBytes, (playTime + pauseTime) * 1000)));
                                bitrateString = String.format(Locale.US, "%s", KANTVUtils.formatedBitRate(totalBytes, (playTime + pauseTime) * 1000));
                            }
                            //setRowValue(R.string.seek_load_cost, String.format(Locale.US, "%d ms", seekLoadDuration));

                            setRowValue(R.string.fps, fpsString + "      \t" + mActivity.getString(R.string.bit_rate) + "\t" +  bitrateString);
                        }

                        String videoProfileLevel = " ";
                        ITrackInfo trackInfos[] = mMediaPlayer.getTrackInfo();
                        if (trackInfos != null) {
                            int index = -1;
                            for (ITrackInfo trackInfo : trackInfos) {
                                index++;
                                int trackType = trackInfo.getTrackType();
                                IMediaFormat mediaFormat = trackInfo.getFormat();
                                if (mediaFormat == null) {
                                } else if (mediaFormat instanceof IjkMediaFormat) {
                                    switch (trackType) {
                                        case ITrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                                            //setRowValue(R.string.mi_video_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_NAME_UI));
                                            //setRowValue(R.string.mi_profile_level, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI));
                                            videoProfileLevel = " (" + mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI) + ") ";
                                            //setRowValue(R.string.mi_pixel_format, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PIXEL_FORMAT_UI));
                                            //setRowValue(R.string.mi_resolution, mediaFormat.getString(IjkMediaFormat.KEY_IJK_RESOLUTION_UI));
                                            break;
                                        case ITrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                                            //setRowValue(R.string.mi_audio_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_NAME_UI));
                                            //setRowValue(R.string.mi_sample_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_SAMPLE_RATE_UI));
                                            //setRowValue(R.string.mi_channels, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CHANNEL_UI));
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                        }

                        {
                            String videoInfo = mp.getVideoInfo();
                            String audioInfo = mp.getAudioInfo();
                            if (videoInfo != null) {
                                //tmpString = KANTVUtils.convertLongString(videoInfo.substring(6, videoInfo.indexOf(",")) + videoProfileLevel + videoInfo.substring(videoInfo.indexOf(",")), 80);
                                tmpString = videoInfo.substring(6, videoInfo.indexOf(",")) + videoProfileLevel + videoInfo.substring(videoInfo.indexOf(","));
                                setRowValue(R.string.videoinfo, tmpString);
                            }

                            if (audioInfo != null) {
                                //tmpString = KANTVUtils.convertLongString(audioInfo.substring(6), 80);
                                tmpString = audioInfo.substring(6);
                                setRowValue(R.string.audioinfo, tmpString);
                            }
                        }
                    }



                    //setRowValue(R.string.clear_content, (mMediaPlayer.isClearContent() ? "clear content" : "encrypted content"));
                    //if (!mMediaPlayer.isClearContent()) {
                    //    setRowValue(R.string.drminfo, mMediaPlayer.getDrmInfo());
                    //    setRowValue(R.string.decrypt, KANTVUtils.getDecryptMode());
                    //}
                    if (!KANTVUtils.getReleaseMode())
                    {
                        if (!mMediaPlayer.isClearContent()) {
                            String drmInfo = KANTVUtils.getDrminfoString(mActivity, mMediaPlayer, R.string.drminfo, R.string.encryptinfo, R.string.decrypt);
                            tmpString = KANTVUtils.convertLongString(drmInfo, 30);
                            setRowValue(R.string.clear_content, tmpString);
                        } else {
                            setRowValue(R.string.clear_content, "clear content");
                        }
                    }

                    if (mMediaPlayer instanceof FFmpegMediaPlayer) {


                    } else {
                        String videoInfo = mMediaPlayer.getVideoInfo();
                        String audioInfo = mMediaPlayer.getAudioInfo();
                        if (videoInfo != null) {
                            tmpString = KANTVUtils.convertLongString(videoInfo, 30);
                            setRowValue(R.string.videoinfo, tmpString);
                        }

                        if (audioInfo != null) {
                            tmpString = KANTVUtils.convertLongString(audioInfo, 30);
                            setRowValue(R.string.audioinfo, tmpString);
                        }
                    }

                    //long playTime            = mMediaPlayer.getPlayTime();
                    //long pauseTime           = mMediaPlayer.getPauseTime();
                    //setRowValue(R.string.load_cost, String.format(Locale.US, "%d ms", mLoadCost));
                    //setRowValue(R.string.seek_cost, String.format(Locale.US, "%d ms", mSeekCost));

                    SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    long startTime = mMediaPlayer.getBeginTime();
                    setRowValue(R.string.start_time, formatter.format(new Date(startTime)));
                    Date curTime =  new Date(System.currentTimeMillis());

                    //setRowValue(R.string.play_time, String.valueOf(playTime) + "  secs");
                    //setRowValue(R.string.pause_time, String.valueOf(pauseTime) + "  secs");
                    tmpString = String.valueOf(playTime) + "  secs   " + mActivity.getBaseContext().getString(R.string.pause_time) + ": "  + String.valueOf(pauseTime) + "  secs";
                    tmpString = KANTVUtils.convertLongString(tmpString, 30);


                    if (mMediaPlayer instanceof FFmpegMediaPlayer) {
                        setRowValue(R.string.current_time, formatter.format(curTime) + "       \t" + mActivity.getString(R.string.play_time) + "\t" + tmpString);
                    } else {
                        setRowValue(R.string.current_time, formatter.format(curTime));
                        setRowValue(R.string.play_time, tmpString);
                    }

                    if (!KANTVUtils.getReleaseMode()) {
                        tmpString = "media duration:" + KANTVUtils.getVideoDuration(mMediaPlayer) + "   ," + mActivity.getBaseContext().getString(R.string.position_time)
                                + ":" + KANTVUtils.getVideoPositiontime(mMediaPlayer);
                        tmpString = KANTVUtils.convertLongString(tmpString, 30);
                        setRowValue(R.string.content_time, tmpString);
                    }

                    if (KANTVUtils.getTroubleshootingMode() == KANTVUtils.TROUBLESHOOTING_MODE_INTERNAL) {
                        if (!KANTVUtils.getReleaseMode()) {
                            tmpString = mActivity.getString(R.string.buffering_counts) + ":" + String.valueOf(mVideoView.getBufferingCounts()) + "  ";
                            setRowValue(R.string.stuck_info, tmpString);
                        }
                    } else {
                        if (!KANTVUtils.getReleaseMode())
                        {
                            tmpString = mActivity.getString(R.string.buffering_counts) + ":" + String.valueOf(mVideoView.getBufferingCounts()) + "  ";
                            //TODO
                            //tmpString += mVideoActivity.getString(R.string.buffering_duration) + ":" + "min:" + "N/A" + ",max:" + "N/A" + ",avg:" + "N/A" + "  ";
                            //TODO
                            //tmpString += mVideoActivity.getString(R.string.buffering_network_bandwidth) + ":" + "min:" + "N/A" + ",max:" + "N/A" + ",avg:" + "N/A" + "  ";
                            tmpString = KANTVUtils.convertLongString(tmpString, 30);
                            setRowValue(R.string.stuck_info, tmpString);
                        }
                    }

                    tmpString = KANTVUtils.convertLongString(mBoardInfo, 30);
                    setRowValue(R.string.device_info, tmpString);

                    tmpString = mActivity.getBaseContext().getString(R.string.network_type) + ":" + KANTVUtils.getNetworkTypeString();
                    if ((KANTVUtils.getNetworkIP() != null) && (!KANTVUtils.getNetworkIP().isEmpty())) {
                        tmpString +=  "  " + mActivity.getBaseContext().getString(R.string.network_ip) + ":" + KANTVUtils.getNetworkIP();
                    }
                    tmpString = KANTVUtils.convertLongString(tmpString, 30);
                    setRowValue(R.string.network_info, tmpString);

                    if ((!KANTVUtils.getReleaseMode()) || (mMediaPlayer instanceof FFmpegMediaPlayer)){
                        ActivityManager am = (ActivityManager) mActivity.getSystemService(Context.ACTIVITY_SERVICE);
                        ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
                        am.getMemoryInfo(memoryInfo);
                        long totalMem = memoryInfo.totalMem;
                        long availMem = memoryInfo.availMem;
                        boolean isLowMemory = memoryInfo.lowMemory;
                        long threshold = memoryInfo.threshold;

                        Debug.MemoryInfo debugInfo = new Debug.MemoryInfo();
                        Debug.getMemoryInfo(debugInfo);
                        //https://stackoverflow.com/questions/13022759/difference-between-total-private-dirty-total-pss-total-shared-dirty-in-android
                        //https://stackoverflow.com/questions/2298208/how-do-i-discover-memory-usage-of-my-application-in-android/2299813#2299813
                        //PrivateDirty, which is basically the amount of RAM inside the process that can not be paged to disk
                        //(it is not backed by the same data on disk), and is not shared with any other processes.
                        //Another way to look at this is the RAM that will become available to the system when that
                        //process goes away (and probably quickly subsumed into caches and other uses of it).
                        //int privateDirty = debugInfo.getTotalPrivateDirty();
                        //int totalPss = debugInfo.getTotalPss();//PSS (Proportional Set Size)
                        //int sharedDirty = debugInfo.getTotalSharedDirty();
                        // dalvikPrivateClean + nativePrivateClean + otherPrivateClean;
                        int totalPrivateClean = debugInfo.getTotalPrivateClean();
                        // dalvikPrivateDirty + nativePrivateDirty + otherPrivateDirty;
                        int totalPrivateDirty = debugInfo.getTotalPrivateDirty();
                        // dalvikPss + nativePss + otherPss;
                        int totalPss = debugInfo.getTotalPss();//PSS (Proportional Set Size)
                        // dalvikSharedClean + nativeSharedClean + otherSharedClean;
                        int totalSharedClean = debugInfo.getTotalSharedClean();
                        // dalvikSharedDirty + nativeSharedDirty + otherSharedDirty;
                        int totalSharedDirty = debugInfo.getTotalSharedDirty();
                        // dalvikSwappablePss + nativeSwappablePss + otherSwappablePss;
                        int totalSwappablePss = debugInfo.getTotalSwappablePss();

                        int totalUsageMemory = totalPrivateClean + totalPrivateDirty + totalPss + totalSharedClean + totalSharedDirty + totalSwappablePss;
                        int Bytes2MBytes = (1 << 20);

                        //VSS - Virtual Set Size
                        //RSS - Resident Set Size
                        //PSS - Proportional Set Size
                        //USS - Unique Set Size
                        //getNativeHeapSize()
                        //getNativeHeapAllocatedSize()
                        //getNativeHeapFreeSize()
                        String memoryInfoString = null;
                        if (mSettings.getUILang() == Settings.KANTV_UILANG_CN) {
                            memoryInfoString = "total mem                              ：" + (totalMem >> 20) + "MB" + "  "
                                    + "threshold of low mem：" + threshold / Bytes2MBytes + "MB" + "  "
                                    + "available mem：" + availMem / Bytes2MBytes + "MB" + "\r\n"
                                    + "NativeHeapSize：" + (Debug.getNativeHeapSize() >> 20) + "MB" + "  "
                                    + "NativeHeapAllocatedSize：" + (Debug.getNativeHeapAllocatedSize() >> 20) + "MB" + " "
                                    + "NativeHeapFreeSize：" + (Debug.getNativeHeapFreeSize() >> 20) + "MB " + "\r\n"
                                    + "total private dirty memory：" + totalPrivateDirty / 1024 + "MB" + "\r\n"
                                    + "total shared  dirty memory：" + totalSharedDirty / 1024 + "MB" + "\r\n"
                                    + "total PSS memory               ：" + totalPss / 1024 + "MB" + "\r\n"
                                    + "total swappable memory  ：" + totalSwappablePss / 1024 + "MB" + "\r\n"
                                    + "total usage memory           ：" + totalUsageMemory / 1024 + "MB" + "\r\n";
                        } else {
                            memoryInfoString = "total mem                              ：" + (totalMem >> 20) + "MB" + "  "
                                    + "threshold of low mem：" + threshold / Bytes2MBytes + "MB" + "  "
                                    + "available mem：" + availMem / Bytes2MBytes + "MB" + "\r\n"
                                    + "NativeHeapSize：" + (Debug.getNativeHeapSize() >> 20) + "MB" + "  "
                                    + "NativeHeapAllocatedSize：" + (Debug.getNativeHeapAllocatedSize() >> 20) + "MB" + " "
                                    + "NativeHeapFreeSize：" + (Debug.getNativeHeapFreeSize() >> 20) + "MB " + "\r\n"
                                    + "total private dirty memory：" + totalPrivateDirty / 1024 + "MB" + "\r\n"
                                    + "total shared  dirty memory：" + totalSharedDirty / 1024 + "MB" + "\r\n"
                                    + "total PSS memory               ：" + totalPss / 1024 + "MB" + "\r\n"
                                    + "total swappable memory  ：" + totalSwappablePss / 1024 + "MB" + "\r\n"
                                    + "total usage memory           ：" + totalUsageMemory / 1024 + "MB" + "\r\n";
                        }


                        if (KANTVUtils.isRunningOnAmlogicBox()) {
                            memoryInfoString += KANTVUtils.getAmlogicTVPEnableInfo();
                            memoryInfoString += KANTVUtils.getAmlogicCodecMMInfo();
                            if (mMediaPlayer instanceof FFmpegMediaPlayer) {
                                memoryInfoString += KANTVUtils.getAmlogicKernelCFGContentWithLines("/sys/class/codec_mm/codec_mm_dump", 10);
                            } else {
                                memoryInfoString += KANTVUtils.getAmlogicKernelCFGContentWithLines("/sys/class/codec_mm/codec_mm_dump", 20);
                            }
                        }
                        setRowValue(R.string.memory_info, memoryInfoString);
                    }

                    mHandler.removeMessages(MSG_UPDATE_HUD);
                    mHandler.sendEmptyMessageDelayed(MSG_UPDATE_HUD, 500);
                }
            }
            return false;
        }
    });

    public static native int kantv_anti_remove_rename_this_file();
}
