/*
 * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
 *
 * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
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

package kantvai.media.exo2;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.exoplayer2.ExoPlayer;
import com.google.android.exoplayer2.util.Util;

import java.io.FileDescriptor;
import java.text.DateFormat;
import java.util.Map;
import java.util.TimeZone;

import kantvai.media.player.AbstractMediaPlayer;
import kantvai.media.player.IMediaPlayer;
import kantvai.media.player.IjkMediaInfo;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.bean.TrackInfoBean;
import kantvai.media.player.misc.IjkTrackInfo;


public class KANTVExo2MediaPlayer extends AbstractMediaPlayer {
    private final static String TAG = KANTVExo2MediaPlayer.class.getName();
    private Context mAppContext;
    private AppCompatActivity mActivity;

    private DemoPlayer mInternalPlayer;
    private DemoPlayerEvent mEventLogger;
    private DemoPlayerListener mDemoListener;

    private String mDataSource;
    private String mVideoTitle;
    private int mVideoWidth;
    private int mVideoHeight;
    private Surface mSurface;
    private boolean mPlaying = false;
    private long mStartPlayPostion = 0;
    private long mBeginTime;
    private long mTotalPlayTime;
    private long mLastPauseTime;
    private long mTotalPauseTime;
    private boolean bIsPaused;
    private int mDecryptMode = KANTVUtils.DECRYPT_SOFT;
    private boolean mDisableAudioTrack = false;
    private boolean mDisableVideoTrack = false;
    private boolean mEnableDumpVideoES = false;
    private boolean mEnableDumpAudioES = false;
    private int mDurationDumpES = 10; //default dumped duration is 10 seconds
    private long mBitrate = 0;

    public KANTVExo2MediaPlayer(AppCompatActivity activity, Context context) {
        KANTVLog.d(TAG, "enter KANTVExo2MediaPlayer");
        mActivity = activity;
        mAppContext = context;
        mEventLogger = new DemoPlayerEvent();
        mDemoListener = new DemoPlayerListener();
        mEventLogger.startSession();
        mBeginTime = System.currentTimeMillis();
        KANTVUtils.setExoplayerInstance(this);//2023-12-15,for recording feature development by exoplayer engine
        KANTVLog.d(TAG, "leave KANTVExo2MediaPlayer");
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {
        if (sh == null)
            setSurface(null);
        else
            setSurface(sh.getSurface());
    }

    @Override
    public void setSurface(Surface surface) {
        KANTVLog.d(TAG, "enter setSurface");
        mSurface = surface;
        if (mInternalPlayer != null)
            mInternalPlayer.setSurface(surface);
        if (mInternalPlayer != null) {
            mInternalPlayer.setDisableAudioTrack(mDisableAudioTrack);
            mInternalPlayer.setDisableVideoTrack(mDisableVideoTrack);
            if ((mDecryptMode == KANTVUtils.DECRYPT_TEE)
                    || (mDecryptMode == KANTVUtils.DECRYPT_TEE_NONSVP)
                    || (mDecryptMode == KANTVUtils.DECRYPT_TEE_SVP)) {
                mInternalPlayer.setUsingTEE(true);
            }
        }

        KANTVLog.d(TAG, "leave setSurfacer");
    }

    @Override
    public void setDataSource(Context context, Uri uri) {
        KANTVLog.d(TAG, "enter setDataSource uri: " + uri.toString());
        mDataSource = uri.toString();
    }

    @Override
    public void setDataSource(Context context, Uri uri, Map<String, String> headers) {
        // TODO: handle headers
        KANTVLog.d(TAG, "enter setDataSource uri: " + uri.toString());
        setDataSource(context, uri);
    }

    @Override
    public void setDataSource(String path) {
        KANTVLog.d(TAG, "enter setDataSource path: " + path);
        setDataSource(mAppContext, Uri.parse(path));
    }

    @Override
    public void setDataSource(FileDescriptor fd) {
        // TODO: no support
        throw new UnsupportedOperationException("no support");
    }

    @Override
    public String getDataSource() {
        return mDataSource;
    }


    @Override
    public void setVideoTitle(String title) {
        mVideoTitle = title;
    }

    @Override
    public String getVideoTitle() {
        return mVideoTitle;
    }


    @Override
    public void prepareAsync() throws IllegalStateException {
        KANTVLog.d(TAG, "enter prepareAsync");
        boolean bUsingTEE = false;
        if (mInternalPlayer != null)
            throw new IllegalStateException("can't prepare a prepared player");

        mInternalPlayer = new DemoPlayer(mActivity, mAppContext);
        mInternalPlayer.addListener(mDemoListener);
        mInternalPlayer.addListener(mEventLogger);
        mInternalPlayer.setInfoListener(mEventLogger);
        mInternalPlayer.setInternalErrorListener(mEventLogger);
        mInternalPlayer.setDisableAudioTrack(mDisableAudioTrack);
        mInternalPlayer.setDisableVideoTrack(mDisableVideoTrack);
        if ((mDecryptMode == KANTVUtils.DECRYPT_TEE)
                || (mDecryptMode == KANTVUtils.DECRYPT_TEE_NONSVP)
                || (mDecryptMode == KANTVUtils.DECRYPT_TEE_SVP)) {
            mInternalPlayer.setUsingTEE(true);
        }

        if (mSurface != null)
            mInternalPlayer.setSurface(mSurface);

        if (mDataSource != null)
            mInternalPlayer.setDataSource(mDataSource);

        if (mVideoTitle != null)
            mInternalPlayer.setVideoTitle(mVideoTitle);

        mInternalPlayer.prepare();
        mInternalPlayer.seekTo(mStartPlayPostion);
        mInternalPlayer.setPlayWhenReady(true);
        mPlaying = true;
        if (bIsPaused) {
            mTotalPauseTime += System.currentTimeMillis() - mLastPauseTime;
            Log.i(TAG, "mTotalPauseTime " + (mTotalPauseTime / 1000));
        }
        bIsPaused = false;
        KANTVLog.d(TAG, "leave prepareAsync");
    }

    @Override
    public void start() throws IllegalStateException {
        KANTVLog.d(TAG, "enter start");
        if (mInternalPlayer == null) {
            KANTVLog.d(TAG, "internal player is null");
            return;
        }
        mInternalPlayer.setPlayWhenReady(true);
        mInternalPlayer.play();
        mPlaying = true;
        if (bIsPaused) {
            mTotalPauseTime += System.currentTimeMillis() - mLastPauseTime;
            Log.i(TAG, "mTotalPauseTime " + (mTotalPauseTime / 1000));
        }
        bIsPaused = false;
        KANTVLog.d(TAG, "leave start");
    }

    @Override
    public void stop() throws IllegalStateException {
        KANTVLog.d(TAG, "enter stop");
        if (mInternalPlayer == null)
            return;
        mPlaying = false;
        mInternalPlayer.release();
        mInternalPlayer = null;
        KANTVLog.d(TAG, "leave stop");
    }

    @Override
    public void pause() throws IllegalStateException {
        KANTVLog.d(TAG, "enter pause");
        if (mInternalPlayer == null)
            return;
        mPlaying = false;
        mInternalPlayer.setPlayWhenReady(false);

        assert (bIsPaused == false);
        if (!bIsPaused) {
            mLastPauseTime = System.currentTimeMillis();
        }
        bIsPaused = !bIsPaused;

        KANTVLog.d(TAG, "leave pause");
    }

    @Override
    public void setWakeMode(Context context, int mode) {
        // FIXME: implement
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        // TODO: do nothing
    }

    @Override
    public IjkTrackInfo[] getTrackInfo() {
        // TODO: implement
        return null;
    }

    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public boolean isPlaying() {
        KANTVLog.d(TAG, "enter isPlaying");
        if (mInternalPlayer == null)
            return false;
        KANTVLog.d(TAG, "leave isPlaying");
        return mPlaying;
    }


    @Override
    public void seekTo(long msec) throws IllegalStateException {
        KANTVLog.d(TAG, "enter seekTo " + msec);
        if (mInternalPlayer == null) {
            KANTVLog.d(TAG, "set start play postion to " + msec);
            mStartPlayPostion = msec;
            return;
        }
        mInternalPlayer.seekTo(msec);
    }

    @Override
    public long getCurrentPosition() {
        KANTVLog.d(TAG, "enter getCurrentPosition");
        if (mInternalPlayer == null)
            return 0;
        return mInternalPlayer.getCurrentPosition();
    }

    @Override
    public long getDuration() {
        KANTVLog.d(TAG, "enter getDuration");
        if (mInternalPlayer == null)
            return 0;
        return mInternalPlayer.getDuration();
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
    public void reset() {
        KANTVLog.d(TAG, "enter reset");

        if (mInternalPlayer != null) {
            mInternalPlayer.release();
            mInternalPlayer.removeListener(mDemoListener);
            mInternalPlayer.removeListener(mEventLogger);
            mInternalPlayer.setInfoListener(null);
            mInternalPlayer.setInternalErrorListener(null);
            mInternalPlayer = null;
        }

        mSurface = null;
        mDataSource = null;
        mVideoWidth = 0;
        mVideoHeight = 0;
        KANTVLog.d(TAG, "leave reset");
    }

    @Override
    public void setLooping(boolean looping) {
        // TODO: no support
        throw new UnsupportedOperationException("no support");
    }

    @Override
    public boolean isLooping() {
        // TODO: no support
        return false;
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        // TODO: no support
    }


    @Override
    public int getAudioSessionId() {
        // TODO: no support
        return 0;
    }

    @Override
    public IjkMediaInfo getMediaInfo() {
        // TODO: no support
        return null;
    }

    @Override
    public void setLogEnabled(boolean enable) {
        // do nothing
    }

    @Override
    public boolean isPlayable() {
        return true;
    }

    @Override
    public void setAudioStreamType(int streamtype) {
        // do nothing
    }

    @Override
    public void setKeepInBackground(boolean keepInBackground) {
        // do nothing
    }

    @Override
    public void release() {
        KANTVLog.d(TAG, "enter release");
        if (mInternalPlayer != null) {
            reset();

            mDemoListener = null;
            mEventLogger.endSession();
            mEventLogger = null;
        }
        KANTVLog.d(TAG, "leave release");
    }


    public int getBufferedPercentage() {
        if (mInternalPlayer == null)
            return 0;

        return mInternalPlayer.getBufferedPercentage();
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
            mTotalPlayTime = ((curTime - mBeginTime - mTotalPauseTime) / 1000);

            return mTotalPlayTime;
        }
        return mTotalPlayTime;
    }


    /**
     * Makes a best guess to infer the type from a media {@link Uri}
     *
     * @param uri The {@link Uri} of the media.
     * @return The inferred type.
     */
    private static int inferContentType(Uri uri) {
        String lastPathSegment = uri.getLastPathSegment();
        return Util.inferContentType(lastPathSegment);
    }


    //TODO
    @Override
    public boolean isClearContent() {
        int isClearContent = KANTVUtils.getKANTVDRMInstance().ANDROID_DRM_IsClearContent();

        if (0 == isClearContent)
            return false;

        if (mDecryptMode == KANTVUtils.DECRYPT_TEE) {
            if (mInternalPlayer.isDRMEnabled()) {
                // FIMXE
                return false;
            } else {
                // FIMXE
                return true;
            }
        }

        return true;
    }

    @Override
    public long getBitRate() {
        if (isPlaying()) {
            return mInternalPlayer.getBitRate();
        }

        return mBitrate;
    }

    @Override
    public void setDisableAudioTrack(boolean bDisable) {
        mDisableAudioTrack = bDisable;
    }

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
        KANTVLog.j(TAG, "setEnableRecording:" + bEnableRecord);
        KANTVUtils.setTVRecording(bEnableRecord);
    }

    //TODO
    @Override
    public void setEnableASR(boolean bEnableASR) {
        KANTVUtils.setTVASR(bEnableASR);
    }

    //FIXME
    @Override
    public void updateRecordingStatus(boolean bEnableRecord) {
        KANTVLog.j(TAG, "updateRecordingStatus:" + bEnableRecord);
        //Thread modelDownLoadThread = new Thread(new Runnable() {
           // @Override
           // public void run() {
                if (bEnableRecord)
                    notifyOnInfo(1983, 0);
                else
                    notifyOnInfo(1984, 0);
           // }
       // });
       // modelDownLoadThread.start();

    }

    //TODO
    @Override
    public boolean getEnableRecord() {
        return KANTVUtils.getTVRecording();
    }

    //TODO
    @Override
    public boolean getEnableASR() {
        return KANTVUtils.getTVASR();
    }


    @Override
    public void setDurationDumpES(int durationDumpES) {
        //default duration is 10 seconds
        mDurationDumpES = durationDumpES;
    }

    @Override
    public void setDecryptMode(int decryptMode) {
        mDecryptMode = decryptMode;
        //internal player might be not initialized at the moment
        if (mInternalPlayer != null) {
            if ((mDecryptMode == KANTVUtils.DECRYPT_TEE)
                    || (mDecryptMode == KANTVUtils.DECRYPT_TEE_NONSVP)
                    || (mDecryptMode == KANTVUtils.DECRYPT_TEE_SVP)) {
                mInternalPlayer.setUsingTEE(true);
            }
        }
    }

    //TODO: not implemented
    @Override
    public String getDrmInfo() {
        return "unknown-drm";
    }


    @Override
    public void selectTrack(TrackInfoBean trackInfo, boolean isAudio) {
        if (mInternalPlayer != null)
            mInternalPlayer.selectTrack(trackInfo, isAudio);
        else
            KANTVLog.j(TAG, "internal player is null");
    }


    @Override
    public String getVideoInfo() {
        if (mInternalPlayer != null)
            return mInternalPlayer.getVideoInfo();
        else
            return "unknown";
    }

    @Override
    public String getAudioInfo() {
        if (mInternalPlayer != null)
            return mInternalPlayer.getAudioInfo();
        else
            return "unknown";
    }

    @Override
    public String getVideoDecoderName() {
        if (mInternalPlayer != null)
            return mInternalPlayer.getVideoDecoderName();
        else
            return "MediaCodec";
    }

    @Override
    public String getAudioDecoderName() {
        if (mInternalPlayer != null)
            return mInternalPlayer.getAudioDecoderName();
        else
            return "unknown";
    }

    private class DemoPlayerListener implements DemoPlayer.Listener {
        private boolean mIsPrepareing = false;
        private boolean mDidPrepare = false;
        private boolean mIsBuffering = false;

        public void onStateChanged(boolean playWhenReady, int playbackState) {
            KANTVLog.d(TAG, "enter onStateChanged, playbackState: " + playbackState + "(" + DemoPlayer.getStateString(playbackState) + ")");
            if (mIsBuffering) {
                switch (playbackState) {
                    case ExoPlayer.STATE_ENDED:
                    case ExoPlayer.STATE_READY:
                        KANTVLog.d(TAG, "STATE_READY");
                        notifyOnInfo(IMediaPlayer.MEDIA_INFO_BUFFERING_END, mInternalPlayer.getBufferedPercentage());
                        mIsBuffering = false;
                        break;
                }
            }

            if (mIsPrepareing) {
                switch (playbackState) {
                    case ExoPlayer.STATE_READY:
                        KANTVLog.d(TAG, "STATE_READY");
                        notifyOnPrepared();
                        mIsPrepareing = false;
                        mDidPrepare = false;
                        break;
                }
            }

            switch (playbackState) {
                case ExoPlayer.STATE_IDLE:
                    KANTVLog.d(TAG, "STATE_IDLE");
                    if (KANTVUtils.getHLSPlaylistType() == KANTVUtils.PLAYLIST_TYPE_LIVE) {
                        //do nothing with live content
                    } else {
                        notifyOnCompletion();
                    }
                    break;

                case ExoPlayer.STATE_BUFFERING:
                    KANTVLog.d(TAG, "STATE_BUFFERING");
                    mIsBuffering = true;
                    notifyOnInfo(IMediaPlayer.MEDIA_INFO_BUFFERING_START, mInternalPlayer.getBufferedPercentage());
                    break;
                case ExoPlayer.STATE_READY:
                    KANTVLog.d(TAG, "STATE_READY");
                    mIsBuffering = false;
                    notifyOnPrepared();
                    //notifyOnInfo(IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START, ExoPlayer.STATE_READY);
                    notifyOnInfo(IMediaPlayer.MEDIA_INFO_AUDIO_RENDERING_START, ExoPlayer.STATE_READY);
                    break;
                case ExoPlayer.STATE_ENDED:
                    KANTVLog.d(TAG, "STATE_ENDED");
                    notifyOnCompletion();
                    break;
                default:
                    break;
            }
            KANTVLog.d(TAG, "leave onStateChanged");
        }

        @Override
        public void onRenderedFirstFrame() {
            KANTVLog.d(TAG, "onRenderedFirstFrame");
            notifyOnInfo(IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START, ExoPlayer.STATE_READY);
        }

        public void onError(Exception e) {
            KANTVLog.j(TAG, "enter onError");
            notifyOnError(IMediaPlayer.MEDIA_ERROR_UNKNOWN, IMediaPlayer.MEDIA_ERROR_UNKNOWN);
            KANTVLog.d(TAG, "leave onError");
        }

        public void onVideoSizeChanged(int width, int height, int unappliedRotationDegrees,
                                       float pixelWidthHeightRatio) {
            KANTVLog.d(TAG, "enter onVideoSizeChanged");
            mVideoWidth = width;
            mVideoHeight = height;
            notifyOnVideoSizeChanged(width, height, 1, 1);
            if (unappliedRotationDegrees > 0)
                notifyOnInfo(IMediaPlayer.MEDIA_INFO_VIDEO_ROTATION_CHANGED, unappliedRotationDegrees);
            KANTVLog.d(TAG, "leave onVideoSizeChanged");
        }


        public void onTracksChanged(Object trackSelector, Object trackGroups, Object trackSelections) {
            KANTVLog.j(TAG, "onTracksChanged");
            notifyOnTrackChange(trackSelector, trackGroups, trackSelections);
        }
    }

    public static native int kantv_anti_remove_rename_this_file();
}
