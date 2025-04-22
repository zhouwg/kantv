/*
 * Copyright (C) 2006 Bilibili
 * Copyright (C) 2006 The Android Open Source Project
 * Copyright (C) 2013 Zhang Rui <bbcallen@gmail.com>
 * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package kantvai.media.player;

import android.annotation.TargetApi;
import android.content.Context;
import android.media.DeniedByServerException;
import android.media.MediaDataSource;
import android.media.MediaDrm;
import android.media.MediaFormat;
import android.media.MediaPlayer;
import android.media.TimedText;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.text.DateFormat;
import java.util.Map;
import java.util.TimeZone;
import java.util.UUID;

import kantvai.media.player.bean.TrackInfoBean;
import kantvai.media.player.misc.AndroidTrackInfo;
import kantvai.media.player.misc.IMediaDataSource;
import kantvai.media.player.misc.ITrackInfo;

public class AndroidMediaPlayer extends AbstractMediaPlayer {
    private String TAG = AndroidMediaPlayer.class.getName();
    private final MediaPlayer mInternalMediaPlayer;
    private final AndroidMediaPlayerListenerHolder mInternalListenerAdapter;
    private String mDataSource;
    private MediaDataSource mMediaDataSource;

    private static final UUID CHINADRM_UUID_SOFT = new UUID(0x70c1db9f66ae4127L, 0xbfc0bb1981694b67L);

    private static final UUID CHINADRM_UUID_TEE = new UUID(0x70c1db9f66ae4127L, 0xbfc0bb1981694b66L);

    private final Object mInitLock = new Object();
    private boolean mIsReleased;

    private static IjkMediaInfo sMediaInfo;
    private boolean mFirstSeek   = true;
    private boolean mDRMEnabled  = false;

    private int mDecryptMode = KANTVUtils.DECRYPT_TEE;

    private boolean mDisableAudioTrack = false;
    private boolean mDisableVideoTrack = false;
    private boolean mEnableDumpVideoES      = false;
    private boolean mEnableDumpAudioES      = false;
    private int     mDurationDumpES         = 10; //default dumped duration is 10 seconds

    //just for pressure test & troubleshooting
    private long mBeginTime;
    private long mTotalPlayTime;
    private long mLastPauseTime;
    private long mTotalPauseTime;
    private boolean bIsPaused;

    private String mVideoTitle;

    public AndroidMediaPlayer() {
        synchronized (mInitLock) {
            KANTVLog.d(TAG, "create MediaPlayer");
            mInternalMediaPlayer = new MediaPlayer();
        }
        mInternalListenerAdapter = new AndroidMediaPlayerListenerHolder(this);
        attachInternalListeners();
        mBeginTime = System.currentTimeMillis();
    }

    public MediaPlayer getInternalMediaPlayer() {
        return mInternalMediaPlayer;
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {
        KANTVLog.d(TAG, "setDisplay");
        synchronized (mInitLock) {
            if (!mIsReleased) {
                mInternalMediaPlayer.setDisplay(sh);
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    @Override
    public void setSurface(Surface surface) {
        mInternalMediaPlayer.setSurface(surface);
    }

    @Override
    public void setDataSource(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        KANTVLog.d(TAG, "setDataSource: " + uri);
        mDataSource = uri.toString();
        mInternalMediaPlayer.setDataSource(context, uri);
    }

    @TargetApi(Build.VERSION_CODES.O)
    public void onPrepared() {
        MediaPlayer mp = mInternalMediaPlayer;
        KANTVLog.d(TAG, "onPrepared, duration: " + mp.getDuration());

        try {
            MediaPlayer.DrmInfo drmInfo = mInternalMediaPlayer.getDrmInfo();
            if (null != drmInfo) {
                mDRMEnabled = true;
                Map<UUID, byte[]> mapPssh = drmInfo.getPssh();
                if (mapPssh.containsKey(CHINADRM_UUID_SOFT)) {
                    mInternalMediaPlayer.prepareDrm(CHINADRM_UUID_SOFT);
                } else if (mapPssh.containsKey(CHINADRM_UUID_TEE)) {
                    mInternalMediaPlayer.prepareDrm(CHINADRM_UUID_TEE);
                }
            } else {
                KANTVLog.d(TAG, "no drm info found");
                start();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
    @Override
    public void setDataSource(Context context, Uri uri, Map<String, String> headers)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        KANTVLog.d(TAG, "setDataSource: " + uri);
        mDataSource = uri.toString();
        mInternalMediaPlayer.setDataSource(context, uri, headers);

        if (Build.BOARD.equals("p291_iptv")) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                new Thread(new Runnable() {
                    //@Override
                    public void run() {
                        {
                            mInternalMediaPlayer.setOnDrmConfigHelper(new MediaPlayer.OnDrmConfigHelper() {
                                @Override
                                public void onDrmConfig(MediaPlayer mediaPlayer) {
                                    if (null != mediaPlayer) {
                                        //nothing to do
                                    }
                                }
                            });


                            mInternalMediaPlayer.setOnDrmPreparedListener(new MediaPlayer.OnDrmPreparedListener() {
                                @Override
                                public void onDrmPrepared(MediaPlayer mediaPlayer, int i) {
                                    KANTVLog.d(TAG, "onDrmPrepared");
                                    MediaPlayer.DrmInfo drmInfo = mediaPlayer.getDrmInfo();
                                    Map<UUID, byte[]> mapPssh = null;

                                    String xKeyExtension = null;
                                    mapPssh = drmInfo.getPssh();
                                    byte[] psshForKANTVDRMSoft = mapPssh.get(CHINADRM_UUID_SOFT);
                                    byte[] psshForKANTVDRMTee = mapPssh.get(CHINADRM_UUID_TEE);
                                    if (null != psshForKANTVDRMSoft) {
                                        xKeyExtension = new String(psshForKANTVDRMSoft);
                                    } else if (null != psshForKANTVDRMTee) {
                                        xKeyExtension = new String(psshForKANTVDRMTee);
                                    }
                                    KANTVLog.d(TAG,"xKeyExtension: " + xKeyExtension);

                                    if (null != xKeyExtension) {
                                        MediaDrm.KeyRequest keyRequest = null;
                                        try {
                                            keyRequest = mediaPlayer.getKeyRequest(null, xKeyExtension.getBytes(), "hls", MediaDrm.KEY_TYPE_STREAMING, null);
                                            KANTVLog.d(TAG, "keyRequest: " + keyRequest.toString());
                                        } catch (MediaPlayer.NoDrmSchemeException e) {
                                            e.printStackTrace();
                                            KANTVLog.j(TAG, "error:" + e.toString());
                                        }

                                        if (keyRequest != null) {
                                            MediaDrm.KeyRequest finalKeyRequest = keyRequest;
                                            new Thread(new Runnable() {
                                                @Override
                                                public void run() {
                                                    executeKeyRequest(mediaPlayer, finalKeyRequest);
                                                    start();
                                                }
                                            }).start();

                                        }
                                    }

                                }
                            });
                        }
                    }
                }).start();
            }
        }
    }

    @Override
    public void setDataSource(FileDescriptor fd)
            throws IOException, IllegalArgumentException, IllegalStateException {
        mInternalMediaPlayer.setDataSource(fd);
    }

    @Override
    public void setDataSource(String path) throws IOException,
            IllegalArgumentException, SecurityException, IllegalStateException {
        mDataSource = path;
        KANTVLog.d(TAG, "setDataSource: " + path);
        Uri uri = Uri.parse(path);
        String scheme = uri.getScheme();
        if (!TextUtils.isEmpty(scheme) && scheme.equalsIgnoreCase("file")) {
            mInternalMediaPlayer.setDataSource(uri.getPath());
        } else {
            mInternalMediaPlayer.setDataSource(path);
        }
    }


    @TargetApi(Build.VERSION_CODES.O)
    private void executeKeyRequest(MediaPlayer mediaPlayer, MediaDrm.KeyRequest keyRequest) {
        try {
            KANTVLog.d(TAG, "executeKeyRequest");
            URL url = new URL(keyRequest.getDefaultUrl());
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setDoOutput(true);
            connection.setDoInput(true);
            connection.setRequestMethod("POST");
            connection.setUseCaches(false);
            connection.setInstanceFollowRedirects(true);
            connection.setRequestProperty("Content-Type", "application/json");
            connection.connect();
            DataOutputStream out = new
                    DataOutputStream(connection.getOutputStream());
            out.write(keyRequest.getData());
            out.flush();
            out.close();

            if (connection.getResponseCode() == 200) {
                BufferedReader reader = new BufferedReader(new
                        InputStreamReader(connection.getInputStream()));
                String lines;
                StringBuffer sb = new StringBuffer("");
                while ((lines = reader.readLine()) != null) {
                    sb.append(lines);
                }
                System.out.println(sb);
                reader.close();

                mInternalMediaPlayer.provideKeyResponse(null, sb.toString().getBytes());
            }
            connection.disconnect();
        } catch (MalformedURLException e) {
            e.printStackTrace();
            KANTVLog.j(TAG, "error: " + e.toString());
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
            KANTVLog.j(TAG, "error: " + e.toString());
        } catch (IOException e) {
            e.printStackTrace();
            KANTVLog.j(TAG, "error: " + e.toString());
        } catch (DeniedByServerException e) {
            e.printStackTrace();
            KANTVLog.j(TAG, "error: " + e.toString());
        } catch (MediaPlayer.NoDrmSchemeException e) {
            e.printStackTrace();
            KANTVLog.j(TAG, "error: " + e.toString());
        }
    }

    @TargetApi(Build.VERSION_CODES.M)
    @Override
    public void setDataSource(IMediaDataSource mediaDataSource) {
        releaseMediaDataSource();

        mMediaDataSource = new MediaDataSourceProxy(mediaDataSource);
        mInternalMediaPlayer.setDataSource(mMediaDataSource);
    }

    @TargetApi(Build.VERSION_CODES.M)
    private static class MediaDataSourceProxy extends MediaDataSource {
        private final IMediaDataSource mMediaDataSource;

        public MediaDataSourceProxy(IMediaDataSource mediaDataSource) {
            mMediaDataSource = mediaDataSource;
        }

        @Override
        public int readAt(long position, byte[] buffer, int offset, int size) throws IOException {
            return mMediaDataSource.readAt(position, buffer, offset, size);
        }

        @Override
        public long getSize() throws IOException {
            return mMediaDataSource.getSize();
        }

        @Override
        public void close() throws IOException {
            mMediaDataSource.close();
        }
    }

    @Override
    public String getDataSource() {
        return mDataSource;
    }

    private void releaseMediaDataSource() {
        if (mMediaDataSource != null) {
            try {
                mMediaDataSource.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            mMediaDataSource = null;
        }
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        KANTVLog.d(TAG, "prepareAsync");
        mInternalMediaPlayer.prepareAsync();
    }

    @Override
    public void start() throws IllegalStateException {
        KANTVLog.d(TAG, "start");
        if (bIsPaused) {
            mTotalPauseTime += System.currentTimeMillis() - mLastPauseTime;
            Log.i(TAG, "mTotalPauseTime " + (mTotalPauseTime / 1000) );
        }
        bIsPaused = false;

        if (mDecryptMode == KANTVUtils.DECRYPT_TEE) {
            KANTVLog.j(TAG, "using TEE");
        }
        KANTVLog.j(TAG, "disable video track: " + mDisableVideoTrack);
        KANTVLog.j(TAG, "disable audio track: " + mDisableAudioTrack);
        if ((mDisableAudioTrack) && (mDisableVideoTrack)) {
            KANTVLog.j(TAG, "audio and video couldn't be both disabled, pls check");
        } else {
            MediaPlayer.TrackInfo[] trackInfos = mInternalMediaPlayer.getTrackInfo();
            if (trackInfos != null) {
                KANTVLog.d(TAG, "track counts:" + trackInfos.length);
                if (0 == trackInfos.length) {
                    KANTVLog.j(TAG, "\n\n\n");
                    KANTVLog.j(TAG, "=======================================");
                    KANTVLog.j(TAG, "|");
                    KANTVLog.j(TAG, "|");
                    KANTVLog.j(TAG, "FIXME: why this happened?");
                    KANTVLog.j(TAG, "|");
                    KANTVLog.j(TAG, "|");
                    KANTVLog.j(TAG, "=======================================");
                    KANTVLog.j(TAG, "\n\n\n");
                }

                int index = -1;
                for (MediaPlayer.TrackInfo trackInfo : trackInfos) {
                    int trackType  = trackInfo.getTrackType();
                    index++;
                    KANTVLog.j(TAG, "trackType:" + trackType);
                    KANTVLog.j(TAG, "trackInfo:" + trackInfo.toString());

                    switch (trackType) {
                        case MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                            KANTVLog.d(TAG, "video index:" + index);
                            if (mDisableVideoTrack) {
                                KANTVLog.j(TAG, "disable default audio track not supported for AndroidMediaPlayer");
                            }
                            break;
                        case MediaPlayer.TrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                            KANTVLog.j(TAG, "audio index:" + index);
                            if (mDisableAudioTrack) {
                                KANTVLog.j(TAG, "disable default video track not supported for AndroidMediaPlayer");
                            }
                            break;
                        default:
                            break;
                    }
                    MediaFormat mediaFormat = trackInfo.getFormat();
                    if (mediaFormat != null) {
                        KANTVLog.j(TAG, "media format : " + mediaFormat.toString());
                    } else {
                        KANTVLog.j(TAG, "cant get media format for track " + trackInfo.toString());
                    }
                }
            } else {
                KANTVLog.j(TAG, "can't get track infos");
            }
        }
        mInternalMediaPlayer.start();
    }

    @Override
    public void stop() throws IllegalStateException {
        KANTVLog.d(TAG, "stop");
        mInternalMediaPlayer.stop();
    }

    @Override
    public void pause() throws IllegalStateException {
        assert(bIsPaused == false);
        if (!bIsPaused) {
            mLastPauseTime = System.currentTimeMillis();
        }
        bIsPaused = !bIsPaused;
        mInternalMediaPlayer.pause();
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        mInternalMediaPlayer.setScreenOnWhilePlaying(screenOn);
    }

    @Override
    public ITrackInfo[] getTrackInfo() {
        return AndroidTrackInfo.fromMediaPlayer(mInternalMediaPlayer);
    }

    @Override
    public int getVideoWidth() {
        return mInternalMediaPlayer.getVideoWidth();
    }

    @Override
    public int getVideoHeight() {
        return mInternalMediaPlayer.getVideoHeight();
    }

    @Override
    public int getVideoSarNum() {
        return 1;
    }

    @Override
    public int getVideoSarDen() {
        return 1;
    }

    @Override
    public boolean isPlaying() {
        try {
            return mInternalMediaPlayer.isPlaying();
        } catch (IllegalStateException e) {
            KANTVLog.j(TAG, e.toString());
            return false;
        }
    }

    @Override
    public void seekTo(long msec) throws IllegalStateException {
        KANTVLog.d(TAG, "seekTo: " + msec / 1000);
        if (mDRMEnabled && mFirstSeek) {
            mFirstSeek = false; //just a workaround
            return;
        }

        mInternalMediaPlayer.seekTo((int) msec);
    }

    @Override
    public long getCurrentPosition() {
        try {
            return mInternalMediaPlayer.getCurrentPosition();
        } catch (IllegalStateException e) {
            KANTVLog.g(TAG, e.toString());
            return 0;
        }
    }

    @Override
    public long getDuration() {
        try {
            return mInternalMediaPlayer.getDuration();
        } catch (IllegalStateException e) {
            KANTVLog.g(TAG, e.toString());
            return 0;
        }
    }

    @Override
    public void release() {
        KANTVLog.d(TAG, "release");
        mIsReleased = true;
        mInternalMediaPlayer.release();
        releaseMediaDataSource();
        resetListeners();
        attachInternalListeners();
    }

    @Override
    public void reset() {
        KANTVLog.d(TAG, "reset");
        try {
            mInternalMediaPlayer.reset();
        } catch (IllegalStateException e) {
            KANTVLog.g(TAG, e.toString());
        }
        releaseMediaDataSource();
        resetListeners();
        attachInternalListeners();
    }

    @Override
    public void setLooping(boolean looping) {
        mInternalMediaPlayer.setLooping(looping);
    }

    @Override
    public boolean isLooping() {
        return mInternalMediaPlayer.isLooping();
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        mInternalMediaPlayer.setVolume(leftVolume, rightVolume);
    }

    @Override
    public int getAudioSessionId() {
        return mInternalMediaPlayer.getAudioSessionId();
    }

    @Override
    public IjkMediaInfo getMediaInfo() {
        if (sMediaInfo == null) {
            IjkMediaInfo module = new IjkMediaInfo();

            module.mVideoDecoder = "android";
            module.mVideoDecoderImpl = "HW";

            module.mAudioDecoder = "android";
            module.mAudioDecoderImpl = "HW";

            sMediaInfo = module;
        }

        return sMediaInfo;
    }

    @Override
    public void setLogEnabled(boolean enable) {
    }

    @Override
    public boolean isPlayable() {
        return true;
    }


    @Override
    public boolean isClearContent() {
        if (mDRMEnabled)
            return false;

        return true;
    }




    //The following functions not implemented and just as stub function

    //TODO
    @Override
    public void setDecryptMode(int decryptMode) {
         mDecryptMode = decryptMode;
    }

    @Override
    public void setDisableAudioTrack(boolean bDisable) {
        mDisableAudioTrack = bDisable;
    }

    @Override
    public void setDisableVideoTrack(boolean bDisable) {
        mDisableVideoTrack = bDisable;
    }

    //TODO
    @Override
    public void setEnableDumpVideoES(boolean bEnable) {
        mEnableDumpVideoES = bEnable;
    }

    //TODO
    @Override
    public void setEnableDumpAudioES(boolean bEnable) {
        mEnableDumpAudioES = bEnable;
    }

    //TODO
    @Override
    public void setEnableRecord(boolean bEnableRecord) {
    }

    //TODO
    @Override
    public void setEnableASR(boolean bEnableRecord) {
    }

    //TODO
    @Override
    public void updateRecordingStatus(boolean bEnableRecord) {
    }

    //TODO
    @Override
    public boolean getEnableRecord() {
        return false;
    }

    //TODO
    @Override
    public boolean getEnableASR() {
        return false;
    }

    //TODO
    @Override
    public void setDurationDumpES(int durationDumpES) {
        mDurationDumpES = durationDumpES;
    }

    //TODO: not implemented
    @Override
    public String getDrmInfo() {
        return "ChinaDRM";
    }

    //TODO: not implemented
    @Override
    public long getBitRate() {
        return 0L;
    }

    //TODO: not implemented
    @Override
    public void selectTrack(TrackInfoBean trackInfo, boolean isAudio) { }


    @Override
    public void setVideoTitle(String title) { mVideoTitle = title;}

    @Override
    public String getVideoTitle() {
        return  mVideoTitle;
    }


    @Override
    public String getVideoInfo() {
        return "unknown";
    }

    @Override
    public String getAudioInfo() {
        return "unknown";
    }

    @Override
    public String getVideoDecoderName() {
        return "MediaCodec";
    }

    @Override
    public String getAudioDecoderName() {
        return "unknown";
    }





    public long getBeginTime() {
        return mBeginTime;
    }

    public long getPauseTime() {
        long mCurrentPauseTime = 0;
        if (bIsPaused) {
            mCurrentPauseTime = System.currentTimeMillis() - mLastPauseTime;
        }
        return (mTotalPauseTime + mCurrentPauseTime) / 1000;
    }

    public long getPlayTime() {
        DateFormat dateFormatterChina = DateFormat.getDateTimeInstance(DateFormat.MEDIUM, DateFormat.MEDIUM);
        TimeZone timeZoneChina = TimeZone.getTimeZone("Asia/Shanghai");
        dateFormatterChina.setTimeZone(timeZoneChina);

        if (isPlaying()) {
            long curTime = System.currentTimeMillis();
            mTotalPlayTime = ((curTime - mBeginTime - mTotalPauseTime) / 1000) ;

            return mTotalPlayTime;
        }
        return mTotalPlayTime;
    }


    @Override
    public void setWakeMode(Context context, int mode) {
        mInternalMediaPlayer.setWakeMode(context, mode);
    }

    @Override
    public void setAudioStreamType(int streamtype) {
        mInternalMediaPlayer.setAudioStreamType(streamtype);
    }

    @Override
    public void setKeepInBackground(boolean keepInBackground) {
    }

    private void attachInternalListeners() {
        mInternalMediaPlayer.setOnPreparedListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnBufferingUpdateListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnCompletionListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnSeekCompleteListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnVideoSizeChangedListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnErrorListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnInfoListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnTimedTextListener(mInternalListenerAdapter);
    }

    private class AndroidMediaPlayerListenerHolder implements
            MediaPlayer.OnPreparedListener, MediaPlayer.OnCompletionListener,
            MediaPlayer.OnBufferingUpdateListener,
            MediaPlayer.OnSeekCompleteListener,
            MediaPlayer.OnVideoSizeChangedListener,
            MediaPlayer.OnErrorListener, MediaPlayer.OnInfoListener,
            MediaPlayer.OnTimedTextListener {
        public final WeakReference<AndroidMediaPlayer> mWeakMediaPlayer;

        public AndroidMediaPlayerListenerHolder(AndroidMediaPlayer mp) {
            mWeakMediaPlayer = new WeakReference<AndroidMediaPlayer>(mp);
        }

        @Override
        public boolean onInfo(MediaPlayer mp, int what, int extra) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            KANTVLog.j(TAG, "what:" + what + ",extra:" + extra);
            return self != null && notifyOnInfo(what, extra);

        }

        @Override
        public boolean onError(MediaPlayer mp, int what, int extra) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            return self != null && notifyOnError(what, extra);

        }

        @Override
        public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnVideoSizeChanged(width, height, 1, 1);
        }

        @Override
        public void onSeekComplete(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnSeekComplete();
        }

        @Override
        public void onBufferingUpdate(MediaPlayer mp, int percent) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            KANTVLog.d(TAG, "buffering update, percent: " + percent);
            notifyOnBufferingUpdate(percent);
        }

        @Override
        public void onCompletion(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnCompletion();
        }

        @Override
        public void onPrepared(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnPrepared();
        }

        @Override
        public void onTimedText(MediaPlayer mp, TimedText text) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            IjkTimedText cdeText = null;

            if (text != null) {
                cdeText = new IjkTimedText(text.getBounds(), text.getText());
            }

            notifyOnTimedText(cdeText);
        }
    }

    public static native int kantv_anti_remove_rename_this_file();
}
