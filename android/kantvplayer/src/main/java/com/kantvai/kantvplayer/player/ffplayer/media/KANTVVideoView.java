/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
 * Copyright (c) Project KanTV. 2021-2023
 */

package com.kantvai.kantvplayer.player.ffplayer.media;

import android.animation.ValueAnimator;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.net.Uri;
import android.os.Build;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.MediaController;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.Settings;

import java.io.File;
import java.io.IOException;
import java.util.Locale;
import java.util.Map;

import kantvai.ai.ggmljava;
import kantvai.media.exoplayer2.kantvexo2.KANTVExo2MediaPlayer;
import kantvai.media.player.AndroidMediaPlayer;
import kantvai.media.player.KANTVLibraryLoader;
import kantvai.media.player.KANTVDRM;
import kantvai.media.player.IMediaPlayer;
import kantvai.media.player.KANTVDataEntity;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.FFmpegMediaPlayer;
import kantvai.media.player.IjkTimedText;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.MediaPlayerService;
import kantvai.media.player.KANTVDRMManager;
import kantvai.media.player.TextureMediaPlayer;
import kantvai.media.player.misc.IMediaDataSource;
import kantvai.media.player.misc.ITrackInfo;

import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.DialogInterface;
import android.content.res.Resources;

import androidx.appcompat.app.AlertDialog;

import android.widget.TableLayout;
import android.widget.TextClock;
import android.widget.TextView;
import android.widget.Toast;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONObject;


public class KANTVVideoView extends FrameLayout implements MediaController.MediaPlayerControl {
    private static final String TAG = KANTVVideoView.class.getName();
    private Uri mUri;
    private Map<String, String> mHeaders;

    private boolean bShowInfo = false;
    // all possible internal states
    private static final int STATE_ERROR = -1;
    private static final int STATE_IDLE = 0;
    private static final int STATE_PREPARING = 1;
    private static final int STATE_PREPARED = 2;
    private static final int STATE_PLAYING = 3;
    private static final int STATE_PAUSED = 4;
    private static final int STATE_PLAYBACK_COMPLETED = 5;

    // mCurrentState is a VideoView object's current state.
    // mTargetState is the state that a method caller intends to reach.
    // For instance, regardless the VideoView object's current state,
    // calling pause() intends to bring the object to a target state
    // of STATE_PAUSED.
    private int mCurrentState = STATE_IDLE;
    private int mTargetState = STATE_IDLE;

    // All the stuff we need for playing and showing a video
    private IRenderView.ISurfaceHolder mSurfaceHolder = null;
    private IMediaPlayer mMediaPlayer = null;

    private int mVideoWidth;
    private int mVideoHeight;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mVideoRotationDegree;
    private int mVideoTargetRotationDegree;
    private Matrix mOriginalMatrix;

    private IMediaController mMediaController;
    private IMediaPlayer.OnCompletionListener mOnCompletionListener;
    private IMediaPlayer.OnPreparedListener mOnPreparedListener;
    private int mCurrentBufferPercentage;
    private IMediaPlayer.OnErrorListener mOnErrorListener;
    private IMediaPlayer.OnInfoListener mOnInfoListener;
    private IMediaPlayer.OnTimedTextListener mOnTimedTextListener;
    private IMediaPlayer.OnTrackChangeListener mOnTrackChangeListener;
    private int mSeekWhenPrepared;  // recording the seek position while preparing
    private boolean mCanPause = true;
    private boolean mCanSeekBack = true;
    private boolean mCanSeekForward = true;

    /** Subtitle rendering widget overlaid on top of the video. */
    // private RenderingWidget mSubtitleWidget;

    /**
     * Listener for changes to subtitle data, used to redraw when needed.
     */
    // private RenderingWidget.OnChangedListener mSubtitlesChangedListener;
    private AppCompatActivity mActivity;
    private Context mAppContext;
    private Settings mSettings;
    private SharedPreferences mSharedPreferences;
    private IRenderView mRenderView;
    private int mVideoSarNum;
    private int mVideoSarDen;


    private float mVideoScale = 1.0f;
    private boolean mIsNormalScreen = true;
    private int mScreenOrWidth;
    private int mScreenOrHeight;
    private Matrix mSaveMatrix;

    private TableLayout mHudView;
    private InfoHudViewHolder mHudViewHolder;

    private long mPrepareStartTime = 0;
    private long mPrepareEndTime = 0;

    private long mSeekStartTime = 0;
    private long mSeekEndTime = 0;
    private String mVideoTitle;
    private String mMediaType;
    private String mVideoPath;
    private long mPlayPos = 0;
    private String mDrmScheme;
    private String mDrmLicenseURL;
    private boolean mIsLive = false;
    private boolean mEnableTFLite = false;
    private boolean mEnableLandscape = true;
    private boolean mEnableFullscreen = false;
    private boolean mIsExoplayer = false;

    private TextView subtitleDisplay;
    private ProgressDialog mProgressDialog;
    private VoisePlayingIcon mAudioAnimatonView;
    private TextClock mTextClock;
    private TextView mTextInfo;
    private long mBufferingCounts = 0;


    private int mKeyFSM[] = new int[4];

    public KANTVVideoView(Context context) {
        super(context);
        initVideoView(context);
    }

    public KANTVVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initVideoView(context);
    }

    public KANTVVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initVideoView(context);
    }


    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public KANTVVideoView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initVideoView(context);
    }


    private void initVideoView(Context context) {
        mAppContext = context.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

        initBackground();
        initRenders();

        clearKeyFSM();

        mVideoWidth = 0;
        mVideoHeight = 0;

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();

        mCurrentState = STATE_IDLE;
        mTargetState = STATE_IDLE;

        subtitleDisplay = new TextView(context);
        subtitleDisplay.setTextSize(24);
        subtitleDisplay.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams layoutParams_txt = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        addView(subtitleDisplay, layoutParams_txt);

        _notifyMediaStatus();
    }


    public void setRenderView(IRenderView renderView) {
        if (mRenderView != null) {
            if (mMediaPlayer != null)
                mMediaPlayer.setDisplay(null);

            View renderUIView = mRenderView.getView();
            mRenderView.removeRenderCallback(mSHCallback);
            mRenderView = null;
            removeView(renderUIView);
        }

        if (renderView == null)
            return;

        mRenderView = renderView;
        renderView.setAspectRatio(mCurrentAspectRatio);
        if (mVideoWidth > 0 && mVideoHeight > 0)
            renderView.setVideoSize(mVideoWidth, mVideoHeight);
        if (mVideoSarNum > 0 && mVideoSarDen > 0)
            renderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);

        View renderUIView = mRenderView.getView();
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER);
        renderUIView.setLayoutParams(lp);
        addView(renderUIView);

        mRenderView.addRenderCallback(mSHCallback);
        mRenderView.setVideoRotation(mVideoRotationDegree);
    }


    public void setRender(int render) {
        switch (render) {
            case RENDER_NONE:
                setRenderView(null);
                break;
            case RENDER_TEXTURE_VIEW: {
                TextureRenderView renderView = new TextureRenderView(getContext());
                if (mMediaPlayer != null) {
                    renderView.getSurfaceHolder().bindToMediaPlayer(mMediaPlayer);
                    renderView.setVideoSize(mMediaPlayer.getVideoWidth(), mMediaPlayer.getVideoHeight());
                    renderView.setVideoSampleAspectRatio(mMediaPlayer.getVideoSarNum(), mMediaPlayer.getVideoSarDen());
                    renderView.setAspectRatio(mCurrentAspectRatio);
                }
                setRenderView(renderView);
                break;
            }
            case RENDER_SURFACE_VIEW: {
                SurfaceRenderView renderView = new SurfaceRenderView(getContext());
                setRenderView(renderView);
                break;
            }
            default:
                Log.e(TAG, String.format(Locale.getDefault(), "invalid render %d\n", render));
                break;
        }
    }


    public void setVideoRotation(int degree) {
        mVideoTargetRotationDegree = mVideoRotationDegree + degree;
        mRenderView.setVideoRotation(mVideoTargetRotationDegree);
    }


    public Matrix getVideoTransform() {
        if (mOriginalMatrix == null) {
            mOriginalMatrix = mRenderView.getTransform();
        }
        return mRenderView.getTransform();
    }


    public void setVideoTransform(Matrix transform) {
        mRenderView.setTransform(transform);
    }


    public boolean adjustVideoView(float scale) {
        mVideoScale *= scale;
        final int degree = (mVideoTargetRotationDegree + 360) % 360;
        if (mVideoScale == 1.0f && degree == 0) {
            return false;
        }

        if (degree > 315 || degree <= 45) {
            mVideoRotationDegree = 0;
        } else if (degree > 45 && degree <= 135) {
            mVideoRotationDegree = 90;
        } else if (degree > 135 && degree <= 225) {
            mVideoRotationDegree = 180;
        } else if (degree > 225 && degree <= 315) {
            mVideoRotationDegree = 270;
        } else {
            mVideoRotationDegree = 0;
        }

        final int deltaDegree = mVideoRotationDegree - mVideoTargetRotationDegree;
        mVideoTargetRotationDegree = mVideoRotationDegree;

        final Matrix matrix = getVideoTransform();
        if (mScreenOrWidth == 0 || mScreenOrHeight == 0) {
            mScreenOrWidth = mRenderView.getView().getWidth();
            mScreenOrHeight = mRenderView.getView().getHeight();
        }
        if (!mIsNormalScreen) {
            matrix.preScale(mVideoScale, mVideoScale);
            matrix.postTranslate(mScreenOrWidth * (1 - mVideoScale) / 2,
                    mScreenOrHeight * (1 - mVideoScale) / 2);
            mRenderView.setTransform(matrix);
            mIsNormalScreen = true;
        } else {
            float[] points = new float[2];
            matrix.mapPoints(points);
            final float deltaX = mScreenOrWidth * (1 - mVideoScale) / 2 - points[0];
            final float deltaY = mScreenOrHeight * (1 - mVideoScale) / 2 - points[1];
            if (mSaveMatrix == null) {
                mSaveMatrix = new Matrix();
            }
            ValueAnimator animator = ValueAnimator.ofFloat(0, 1.0f).setDuration(300);
            animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                @Override
                public void onAnimationUpdate(ValueAnimator valueAnimator) {
                    float percent = (float) valueAnimator.getAnimatedValue();
                    mSaveMatrix.set(matrix);
                    mSaveMatrix.postTranslate(deltaX * percent, deltaY * percent);
                    mRenderView.setTransform(mSaveMatrix);
                    mRenderView.setVideoRotation((int) (mVideoRotationDegree - deltaDegree * (1 - percent)));
                }
            });
            animator.start();
        }
        return true;
    }


    public void resetVideoView(boolean isForever) {
        mIsNormalScreen = isForever;
        mVideoRotationDegree = 0;
        if (isForever) {
            mVideoTargetRotationDegree = 0;
            mVideoScale = 1.0f;
        }
        mRenderView.setTransform(mOriginalMatrix);
        mRenderView.setVideoRotation(mVideoRotationDegree);
    }


    public void setHudView(AppCompatActivity activity, TableLayout tableLayout) {
        mHudView = tableLayout;
        mHudViewHolder = new InfoHudViewHolder(getContext(), tableLayout, activity);
    }

    public void setAudioView(VoisePlayingIcon audioView) {
        mAudioAnimatonView = audioView;
    }


    public void setActivity(AppCompatActivity activity) {
        mActivity = activity;
    }

    public void setTextClock(TextClock txtCock) {
        mTextClock = txtCock;
    }

    public void setTextInfo(TextView txtInfo) {
        mTextInfo = txtInfo;
    }


    public void setVideoPath(String path) {
        mVideoPath = path;
        setVideoURI(Uri.parse(path));
    }

    public void setVideoTitle(String title) {
        mVideoTitle = title;
    }

    public void setDrmScheme(String drmScheme) {
        mDrmScheme = drmScheme;
    }

    public void setDrmLicenseURL(String drmLicenseURL) {
        mDrmLicenseURL = drmLicenseURL;
    }

    public void setIsLive(boolean isLive) {
        mIsLive = isLive;
    }

    public void setMediaType(String mediaType) {
        mMediaType = mediaType;
    }

    public void setPlayPos(long playPos) {
        mPlayPos = playPos;
    }


    public void setVideoURI(Uri uri) {
        setVideoURI(uri, null);
    }

    public Uri getUri() {
        return mUri;
    }

    private void setVideoURI(Uri uri, Map<String, String> headers) {
        KANTVLog.d(TAG, "setVideoURI:" + uri.toString());
        mUri = uri;
        mHeaders = headers;
        mSeekWhenPrepared = 0;
        openVideo();
        requestLayout();
        invalidate();
    }


    public Bitmap getScreenshot() {
        if (mRenderView != null) {
            return mRenderView.getVideoScreenshot();
        }
        return null;
    }


    public void stopPlayback() {
        if (mMediaPlayer != null) {
            if (mEnableLandscape) {
                //mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            }

            if (!mSettings.getDevMode()) {
                if (!TextUtils.isEmpty(mVideoPath)) {
                    String regEx = "[`~!@#$%^&*()+=|{}:;\\\\[\\\\].<>/?~！@（）——+|{}【】‘；：”“’。，、？']";
                    String videoTitle = Pattern.compile(regEx).matcher(mVideoTitle).replaceAll("").trim();
                    long playedTime = getPlayedTime();
                    long currentPos = mMediaPlayer.getCurrentPosition();
                    KANTVLog.d(TAG, "playedTime: " + playedTime + " secs, playPos: " + mPlayPos + " secs");
                    KANTVLog.d(TAG, "currentPos: " + currentPos / 1000 + " secs");
                    String currentPosString = null;
                    String playedTimeString = null;
                    if (currentPos > 0) {
                        playedTimeString = String.valueOf(playedTime + mPlayPos);
                        currentPosString = String.valueOf(currentPos / 1000);
                        //use "_KANTV_" as delimiter here
                        //String syncString = mVideoPath + "_KANTV_" + mMediaType + "_KANTV_" + videoTitle + "_KANTV_" + playedTimeString;
                        String syncString = mVideoPath + "_KANTV_" + mMediaType + "_KANTV_" + videoTitle + "_KANTV_" + currentPosString;
                        //new RecentMediaStorage(mActivity).saveUrlAsync(syncString);
                    } else {
                        KANTVLog.e(TAG, "can't get current postion");
                    }

                    KANTVUtils.umStopPlayback(videoTitle, mVideoPath);
                }
            }

            ggmljava.asr_stop();
            //ggmljava.asr_finalize();
            setEnableASR(false);
            KANTVUtils.setTVASR(false);


            KANTVUtils.setTVRecording(false);
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;

            if (mHudViewHolder != null)
                mHudViewHolder.setMediaPlayer(null);

            if (mAudioAnimatonView != null)
                stopAudioAnimation();

            mCurrentState = STATE_IDLE;
            mTargetState = STATE_IDLE;
            AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            am.abandonAudioFocus(null);
        }

        if ((mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__FFmpeg)
                || (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__AndroidMediaPlayer)) {
            KANTVDRM mKANTVDRM = KANTVDRM.getInstance();
            if (mKANTVDRM != null) {
                mKANTVDRM.ANDROID_JNI_Finalize();
            }
        }

        if (mSettings.getPlayerEngine() == KANTVUtils.PV_PLAYERENGINE__Exoplayer) {
            KANTVDRMManager.closePlaybackService();
        }
        KANTVLog.d(TAG, "end playback");
    }

    @TargetApi(Build.VERSION_CODES.M)
    private void openVideo() {
        if (mUri == null || mSurfaceHolder == null) {
            // not ready for playback just yet, will try again later
            return;
        }

        release(false);

        AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
        am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);

        try {
            KANTVLog.g(TAG, "play engine " + mSettings.getPlayerEngine());
            mMediaPlayer = createPlayer(mSettings.getPlayerEngine());
            if (mMediaPlayer instanceof KANTVExo2MediaPlayer) {
                mIsExoplayer = true;
            }

            mMediaPlayer.setOnPreparedListener(mPreparedListener);
            mMediaPlayer.setOnVideoSizeChangedListener(mSizeChangedListener);
            mMediaPlayer.setOnCompletionListener(mCompletionListener);
            mMediaPlayer.setOnErrorListener(mErrorListener);
            mMediaPlayer.setOnInfoListener(mInfoListener);
            mMediaPlayer.setOnBufferingUpdateListener(mBufferingUpdateListener);
            mMediaPlayer.setOnSeekCompleteListener(mSeekCompleteListener);
            mMediaPlayer.setOnTimedTextListener(mOnTimedTextListener);
            mCurrentBufferPercentage = 0;
            String scheme = mUri.getScheme();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M &&
                    mSettings.getUsingMediaDataSource() &&
                    (TextUtils.isEmpty(scheme) || scheme.equalsIgnoreCase("file"))) {
                IMediaDataSource dataSource = new FileMediaDataSource(new File(mUri.toString()));
                mMediaPlayer.setDataSource(dataSource);
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                KANTVLog.d(TAG, "set url: " + mUri);
                if (mSeekWhenPrepared != 0) {
                }
                mMediaPlayer.setDataSource(mAppContext, mUri, mHeaders);
            } else {
                mMediaPlayer.setDataSource(mUri.toString());
            }
            mMediaPlayer.setVideoTitle(mVideoTitle);
            bindSurfaceHolder(mMediaPlayer, mSurfaceHolder);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setScreenOnWhilePlaying(true);
            mPrepareStartTime = System.currentTimeMillis();
            KANTVLog.d(TAG, "mMediaPlayer.prepareAsync()");
            mMediaPlayer.prepareAsync();

            if (mHudViewHolder != null) {
                mHudViewHolder.setVideoActivity(mActivity);
                mHudViewHolder.setVideoView(this);
                mHudViewHolder.setVideoTitle(mVideoTitle);
                mHudViewHolder.setMediaPlayer(mMediaPlayer);

                /*
                <color name="white">#FFFFFF</color>
                <color name="blue">#FF0000FF</color>
                <color name="black">#FF000000</color>
                <color name="cyan">#FF00FFFF</color>
                <color name="green">#FF00FF00</color>
                <color name="red">#FFFF0000</color>
                <color name="yellow">#FFFFFF00</color>
                <color name="dkgray">#80808FF0</color>
                <color name="ijk_transparent_dark">#77000000</color>
                 */
                mHudViewHolder.setBackgroundColor(Color.parseColor("#77808FF0"));
                mHudViewHolder.setVisibility(View.INVISIBLE);
            }

            mCurrentState = STATE_PREPARING;
            attachMediaController();
        } catch (IOException ex) {
            KANTVLog.w(TAG, "Unable to open content: " + mUri + ex.getMessage());
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
        } catch (IllegalArgumentException ex) {
            KANTVLog.w(TAG, "Unable to open content: " + mUri + ex.getMessage());
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
        } finally {
            _notifyMediaStatus();
        }
    }

    public void setHudViewHolderVisibility(int visibility) {
        if (mHudViewHolder != null) {
            KANTVLog.d(TAG, "dev_mode: " + mSettings.getDevMode());
            KANTVLog.d(TAG, "release_mode: " + KANTVUtils.getReleaseMode());
            mSettings.updateUILang(mActivity);
            if (visibility == View.VISIBLE) {
                mHudViewHolder.setVisibility(View.VISIBLE);
            } else if (visibility == View.INVISIBLE) {
                mHudViewHolder.setVisibility(View.INVISIBLE);
            } else {
                KANTVLog.d(TAG, "invalid visibility mode : " + visibility);
            }

            if (KANTVUtils.getDisableVideoTrack()) {
                this.setBackgroundColor(0xFF00FF00);
            }
        }
    }


    public void setEnableRecord(boolean bEnableRecord) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setEnableRecord(bEnableRecord);
        } else {
            KANTVLog.d(TAG, "player not initialized");
        }
    }

    public boolean getEnableRecord() {
        if (mMediaPlayer != null)
            return mMediaPlayer.getEnableRecord();
        else
            return false;
    }

    public void setEnableASR(boolean bEnableASR) {
        if (mMediaPlayer != null) {
            mMediaPlayer.setEnableASR(bEnableASR);
        } else {
            KANTVLog.d(TAG, "player not initialized");
        }
    }

    public boolean getEnableASR() {
        if (mMediaPlayer != null)
            return mMediaPlayer.getEnableASR();
        else
            return false;
    }

    public void setMediaController(IMediaController controller) {
        KANTVLog.d(TAG, "setMediaController");
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = controller;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaPlayer != null && mMediaController != null) {
            mMediaController.setMediaPlayer(this);
            View anchorView = this.getParent() instanceof View ?
                    (View) this.getParent() : this;
            mMediaController.setAnchorView(anchorView);
            mMediaController.setEnabled(isInPlaybackState());
        }
    }

    IMediaPlayer.OnVideoSizeChangedListener mSizeChangedListener =
            new IMediaPlayer.OnVideoSizeChangedListener() {
                public void onVideoSizeChanged(IMediaPlayer mp, int width, int height, int sarNum, int sarDen) {
                    mVideoWidth = mp.getVideoWidth();
                    mVideoHeight = mp.getVideoHeight();
                    mVideoSarNum = mp.getVideoSarNum();
                    mVideoSarDen = mp.getVideoSarDen();
                    if (mVideoWidth != 0 && mVideoHeight != 0) {
                        if (mRenderView != null) {
                            mRenderView.setVideoSize(mVideoWidth, mVideoHeight);
                            mRenderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);
                        }

                        if (mTextClock != null) {
                            //mTextClock.setVisibility(View.VISIBLE);
                        }

                        if (mTextInfo != null) {
                            mTextInfo.setVisibility(View.VISIBLE);
                        }

                        if (!KANTVUtils.getReleaseMode() && mSettings.getDevMode()) {
                            setHudViewHolderVisibility(View.VISIBLE);
                        }

                        if ((mActivity.getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE) && (!mSettings.getDevMode())) {
                            if (mActivity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
                                mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                                KANTVUtils.setScreenOrientation(KANTVUtils.SCREEN_ORIENTATION_LANDSCAPE);
                            }
                        }

                        if (KANTVUtils.isRunningOnPhone()) {
                            mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                            KANTVUtils.setScreenOrientation(KANTVUtils.SCREEN_ORIENTATION_LANDSCAPE);
                        }


                        if ((mMediaPlayer != null) && (0 == mMediaPlayer.getDuration()) && (!mSettings.getDevMode())) {
                            //enable pause during online TV playback so APK's user could capture TV screenshot conveniently
                            //mMediaController.setEnabled(false);
                        }
                        requestLayout();
                    }
                }
            };

    IMediaPlayer.OnPreparedListener mPreparedListener = new IMediaPlayer.OnPreparedListener() {
        public void onPrepared(IMediaPlayer mp) {
            KANTVLog.d(TAG, "onPrepared");
            mPrepareEndTime = System.currentTimeMillis();

            if (mHudViewHolder != null) {
                mHudViewHolder.updateLoadCost(mPrepareEndTime - mPrepareStartTime);
            }

            mCurrentState = STATE_PREPARED;
            _notifyMediaStatus();

            if (mMediaPlayer instanceof AndroidMediaPlayer) {
                AndroidMediaPlayer amp = (AndroidMediaPlayer) mMediaPlayer;
                amp.onPrepared();
            }

            if (mOnPreparedListener != null) {
                mOnPreparedListener.onPrepared(mMediaPlayer);
            }
            if (mMediaController != null) {
                mMediaController.setEnabled(true);
            }
            mVideoWidth = mp.getVideoWidth();
            mVideoHeight = mp.getVideoHeight();

            int seekToPosition = mSeekWhenPrepared;  // mSeekWhenPrepared may be changed after seekTo() call
            if (seekToPosition != 0) {
                seekTo(seekToPosition);
            }
            if (mVideoWidth != 0 && mVideoHeight != 0) {
                if (mRenderView != null) {
                    mRenderView.setVideoSize(mVideoWidth, mVideoHeight);
                    mRenderView.setVideoSampleAspectRatio(mVideoSarNum, mVideoSarDen);
                    if (!mRenderView.shouldWaitForResize() || mSurfaceWidth == mVideoWidth && mSurfaceHeight == mVideoHeight) {
                        // We didn't actually change the size (it was already at the size
                        // we need), so we won't get a "surface changed" callback, so
                        // start the video here instead of in the callback.
                        if (mTargetState == STATE_PLAYING) {
                            KANTVLog.d(TAG, "calling start");
                            start();
                            if (mMediaController != null) {
                                mMediaController.show();
                            }
                        } else if (!isPlaying() &&
                                (seekToPosition != 0 || getCurrentPosition() > 0)) {
                            if (mMediaController != null) {
                                // Show the media controls when we're paused into a video and make 'em stick.
                                mMediaController.show(0);
                            }
                        }
                    }
                }
            } else {
                // We don't know the video size yet, but should start anyway.
                // The video size might be reported to us later.
                if (mTargetState == STATE_PLAYING) {
                    KANTVLog.d(TAG, "calling start");
                    start();
                }
            }
        }
    };

    private IMediaPlayer.OnCompletionListener mCompletionListener =
            new IMediaPlayer.OnCompletionListener() {
                public void onCompletion(IMediaPlayer mp) {
                    mCurrentState = STATE_PLAYBACK_COMPLETED;
                    mTargetState = STATE_PLAYBACK_COMPLETED;
                    _notifyMediaStatus();
                    if (mMediaController != null) {
                        mMediaController.hide();
                    }
                    if (mOnCompletionListener != null) {
                        mOnCompletionListener.onCompletion(mMediaPlayer);
                    }
                    stopPlayback();
                    mActivity.finish();
                }
            };

    private void _notifyMediaStatus() {
        if (mOnInfoListener != null) {
            mOnInfoListener.onInfo(mMediaPlayer, mCurrentState, -1);
        }
    }

    private IMediaPlayer.OnInfoListener mInfoListener =
            new IMediaPlayer.OnInfoListener() {
                public boolean onInfo(IMediaPlayer mp, int arg1, int arg2) {
                    KANTVLog.d(TAG, "arg1:" + arg1 + ",arg2:" + arg2);
                    if (mOnInfoListener != null) {
                        mOnInfoListener.onInfo(mp, arg1, arg2);
                    }

                    switch (arg1) {
                        case IMediaPlayer.MEDIA_INFO_VIDEO_TRACK_LAGGING:
                            KANTVLog.d(TAG, "MEDIA_INFO_VIDEO_TRACK_LAGGING:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START:
                            KANTVLog.d(TAG, "MEDIA_INFO_VIDEO_RENDERING_START");
                            KANTVUtils.perfGetRenderedTimeOfFirstVideoFrame();
                            if (mSettings.getPlayMode() != KANTVUtils.PLAY_MODE_NORMAL) {
                                KANTVLog.d(TAG, "loop play elapse time:" + KANTVUtils.perfGetLoopPlayElapseTime() / 1000 + " secs");
                                KANTVLog.d(TAG, "loop play elapse time:" + KANTVUtils.perfGetLoopPlayElapseTimeString());
                                KANTVLog.d(TAG, "loop play counts:" + KANTVUtils.perfGetLoopPlayCounts());
                                KANTVLog.d(TAG, "loop play error counts:" + KANTVUtils.perfGetLoopPlayErrorCounts());
                            }

                            KANTVLog.j(TAG, "duration of first video frame was rendered:" + (KANTVUtils.perfGetFirstVideoFrameTime()) + " millisecs");
                            stopUIBuffering();

                            if (!KANTVUtils.getReleaseMode() && mSettings.getDevMode()) {
                                setHudViewHolderVisibility(View.VISIBLE);
                            }

                            break;
                        case IMediaPlayer.MEDIA_INFO_BUFFERING_START:
                            KANTVLog.d(TAG, "MEDIA_INFO_BUFFERING_START");
                            String status = getResources().getString(R.string.buffering);
                            startUIBuffering(status);
                            break;
                        case IMediaPlayer.MEDIA_INFO_BUFFERING_END:
                            KANTVLog.d(TAG, "MEDIA_INFO_BUFFERING_END");
                            stopUIBuffering();
                            break;
                        case IMediaPlayer.MEDIA_INFO_NETWORK_BANDWIDTH:
                            KANTVLog.d(TAG, "MEDIA_INFO_NETWORK_BANDWIDTH: " + arg2);
                            break;
                        case IMediaPlayer.MEDIA_INFO_BAD_INTERLEAVING:
                            KANTVLog.d(TAG, "MEDIA_INFO_BAD_INTERLEAVING:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_NOT_SEEKABLE:
                            KANTVLog.d(TAG, "MEDIA_INFO_NOT_SEEKABLE:");
                            if (mMediaType.equals("1_Online_RADIO")) {
                                Log.d(TAG, "audio content");
                                stopUIBuffering();

                                if (mMediaPlayer.isPlaying()) {
                                    if (mMediaController != null) {
                                        mMediaController.show();
                                    }
                                }
                                startAudioAnimation();
                                if (mMediaController != null) {
                                    mMediaController.show();
                                }
                            }
                            break;
                        case IMediaPlayer.MEDIA_INFO_METADATA_UPDATE:
                            KANTVLog.d(TAG, "MEDIA_INFO_METADATA_UPDATE:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_UNSUPPORTED_SUBTITLE:
                            KANTVLog.d(TAG, "MEDIA_INFO_UNSUPPORTED_SUBTITLE:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_SUBTITLE_TIMED_OUT:
                            KANTVLog.d(TAG, "MEDIA_INFO_SUBTITLE_TIMED_OUT:");
                            break;
                        case IMediaPlayer.MEDIA_INFO_VIDEO_ROTATION_CHANGED:
                            mVideoRotationDegree = arg2;
                            KANTVLog.d(TAG, "MEDIA_INFO_VIDEO_ROTATION_CHANGED: " + arg2);
                            if (mRenderView != null)
                                mRenderView.setVideoRotation(arg2);
                            break;
                        case IMediaPlayer.MEDIA_INFO_AUDIO_RENDERING_START:
                            KANTVLog.d(TAG, "MEDIA_INFO_AUDIO_RENDERING_START:");
                            stopUIBuffering();

                            if (mMediaPlayer.isPlaying()) {
                                if (mMediaController != null) {
                                    mMediaController.show();
                                }
                            }
                            startAudioAnimation();
                            if (KANTVUtils.getDisableVideoTrack()) {
                                setHudViewHolderVisibility(View.VISIBLE);
                            }

                            break;
                    }
                    return true;
                }
            };

    private IMediaPlayer.OnErrorListener mErrorListener =
            new IMediaPlayer.OnErrorListener() {
                public boolean onError(IMediaPlayer mp, int framework_err, int impl_err) {
                    KANTVLog.d(TAG, "play mode: " + mSettings.getPlayMode());
                    if (mSettings.getPlayMode() != KANTVUtils.PLAY_MODE_LOOP_LIST) {
                        exitLoopPlayMode();
                    } else {
                        KANTVUtils.perfUpdateLoopPlayErrorCounts();
                    }

                    KANTVLog.d(TAG, "Error: " + framework_err + "," + impl_err);
                    if ((KANTVUtils.isRunningOnAmlogicBox() && (framework_err == -38) && (0 == impl_err))) {
                        KANTVLog.d(TAG, "\n\n\n");
                        KANTVLog.d(TAG, "=======================================");
                        KANTVLog.d(TAG, "|");
                        KANTVLog.d(TAG, "|");
                        KANTVLog.d(TAG, "workaround with this issue");
                        KANTVLog.d(TAG, "|");
                        KANTVLog.d(TAG, "|");
                        KANTVLog.d(TAG, "=======================================");
                        KANTVLog.d(TAG, "\n\n\n");
                        return true;
                    }
                    mCurrentState = STATE_ERROR;
                    mTargetState = STATE_ERROR;
                    if (mMediaController != null) {
                        mMediaController.hide();
                    }

                    /* If an error handler has been supplied, use it and finish. */
                    if (mOnErrorListener != null) {
                        if (mOnErrorListener.onError(mMediaPlayer, framework_err, impl_err)) {
                            return true;
                        }
                    }

                    /* Otherwise, pop up an error dialog so the user knows that
                     * something bad has happened. Only try and pop up the dialog
                     * if we're attached to a window. When we're going away and no
                     * longer have a window, don't bother showing the user an error.
                     */
                    if (getWindowToken() != null) {
                        Resources r = mAppContext.getResources();
                        int messageId;

                        if (framework_err == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
                            messageId = R.string.VideoView_error_text_invalid_progressive_playback;
                        } else {
                            messageId = R.string.VideoView_error_text_unknown;
                        }

                        new AlertDialog.Builder(getContext())
                                .setMessage(messageId)
                                .setPositiveButton(R.string.VideoView_error_button,
                                        new DialogInterface.OnClickListener() {
                                            public void onClick(DialogInterface dialog, int whichButton) {
                                                /* If we get here, there is no onError listener, so
                                                 * at least inform them that the video is over.
                                                 */
                                                if (mOnCompletionListener != null) {
                                                    mOnCompletionListener.onCompletion(mMediaPlayer);
                                                }

                                                if (mProgressDialog != null) {
                                                    mProgressDialog.dismiss();
                                                    mProgressDialog = null;
                                                }
                                                stopPlayback();
                                                mActivity.finish();
                                            }
                                        })
                                .setCancelable(false)
                                .show();
                    }
                    return true;
                }
            };

    private IMediaPlayer.OnBufferingUpdateListener mBufferingUpdateListener =
            new IMediaPlayer.OnBufferingUpdateListener() {
                public void onBufferingUpdate(IMediaPlayer mp, int percent) {
                    mCurrentBufferPercentage = percent;
                }
            };

    private IMediaPlayer.OnTimedTextListener mTimedTextListener = new IMediaPlayer.OnTimedTextListener() {
        @Override
        public void onTimedText(IMediaPlayer mp, IjkTimedText text) {
            if (mOnTimedTextListener != null) {
                mOnTimedTextListener.onTimedText(mp, text);
            }
        }
    };
    private IMediaPlayer.OnTrackChangeListener mTrackChangeListener = new IMediaPlayer.OnTrackChangeListener() {
        @Override
        public void onTrackChange(IMediaPlayer mp, Object trackSelector, Object trackGroups, Object trackSelections) {
            if (mOnTrackChangeListener != null) {
                mOnTrackChangeListener.onTrackChange(mp, trackSelector, trackGroups, trackSelections);
            }
        }
    };
    private IMediaPlayer.OnSeekCompleteListener mSeekCompleteListener = new IMediaPlayer.OnSeekCompleteListener() {

        @Override
        public void onSeekComplete(IMediaPlayer mp) {
            mSeekEndTime = System.currentTimeMillis();

            if (mHudViewHolder != null) {
                mHudViewHolder.updateSeekCost(mSeekEndTime - mSeekStartTime);
            }
        }
    };


    public void setOnPreparedListener(IMediaPlayer.OnPreparedListener l) {
        mOnPreparedListener = l;
    }


    public void setOnCompletionListener(IMediaPlayer.OnCompletionListener l) {
        mOnCompletionListener = l;
    }

    public void setOnErrorListener(IMediaPlayer.OnErrorListener l) {
        mOnErrorListener = l;
    }

    public void setOnTimedTextListener(IMediaPlayer.OnTimedTextListener l) {
        mOnTimedTextListener = l;
    }

    public void setOnTrackChangeListener(IMediaPlayer.OnTrackChangeListener l) {
        mOnTrackChangeListener = l;
    }

    public void setOnInfoListener(IMediaPlayer.OnInfoListener l) {
        mOnInfoListener = l;
    }

    private void bindSurfaceHolder(IMediaPlayer mp, IRenderView.ISurfaceHolder holder) {
        if (mp == null)
            return;

        if (holder == null) {
            mp.setDisplay(null);
            return;
        }

        KANTVLog.d(TAG, "bind surface holder");
        holder.bindToMediaPlayer(mp);
    }

    IRenderView.IRenderCallback mSHCallback = new IRenderView.IRenderCallback() {
        @Override
        public void onSurfaceChanged(@NonNull IRenderView.ISurfaceHolder holder, int format, int w, int h) {
            KANTVLog.d(TAG, "surface changed");
            if (holder.getRenderView() != mRenderView) {
                KANTVLog.e(TAG, "onSurfaceChanged: unmatched render callback\n");
                return;
            }

            mSurfaceWidth = w;
            mSurfaceHeight = h;
            KANTVUtils.setVideoWidth(w);
            KANTVUtils.setVideoHeight(h);
            boolean isValidState = (mTargetState == STATE_PLAYING);
            boolean hasValidSize = !mRenderView.shouldWaitForResize() || (mVideoWidth == w && mVideoHeight == h);
            if (mMediaPlayer != null && isValidState && hasValidSize) {
                if (mSeekWhenPrepared != 0) {
                    seekTo(mSeekWhenPrepared);
                }
                KANTVLog.d(TAG, "calling start");
                start();
            }
        }

        @Override
        public void onSurfaceCreated(@NonNull IRenderView.ISurfaceHolder holder, int width, int height) {
            KANTVLog.d(TAG, "onSurfaceCreated");
            if (holder.getRenderView() != mRenderView) {
                KANTVLog.d(TAG, "onSurfaceCreated: unmatched render callback\n");
                return;
            }

            mSurfaceHolder = holder;
            if (mMediaPlayer != null)
                bindSurfaceHolder(mMediaPlayer, holder);
            else
                openVideo();
        }

        @Override
        public void onSurfaceDestroyed(@NonNull IRenderView.ISurfaceHolder holder) {
            if (holder.getRenderView() != mRenderView) {
                KANTVLog.d(TAG, "onSurfaceDestroyed: unmatched render callback\n");
                return;
            }

            mSurfaceHolder = null;
            releaseWithoutStop();
        }
    };

    public void releaseWithoutStop() {
        if (mMediaPlayer != null)
            mMediaPlayer.setDisplay(null);
    }

    public void release(boolean cleartargetstate) {
        KANTVLog.d(TAG, "enter release");
        if (mMediaPlayer != null) {
            KANTVLog.d(TAG, "not released");
            mMediaPlayer.reset();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mCurrentState = STATE_IDLE;
            _notifyMediaStatus();
            if (cleartargetstate) {
                mTargetState = STATE_IDLE;
            }
            AudioManager am = (AudioManager) mAppContext.getSystemService(Context.AUDIO_SERVICE);
            am.abandonAudioFocus(null);
        } else {
            KANTVLog.d(TAG, "already released");
        }
        KANTVLog.d(TAG, "leave release");
    }

    public void destroy() {
        KANTVLog.d(TAG, "destroy");
        release(true);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisibility();
        }
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisibility();
        }
        return false;
    }


    //add FSM for Android-based TV
    private boolean isKeyFSMCleared() {
        for (int index = 0; index < mKeyFSM.length; index++) {
            if (mKeyFSM[index] != 0)
                return false;
        }
        return true;
    }

    private boolean isKeyFSMSetted() {
        for (int index = 0; index < mKeyFSM.length; index++) {
            if (mKeyFSM[index] != 1)
                return false;
        }
        return true;
    }

    private boolean isGetValidKeyUpEvent() {
        if ((1 == mKeyFSM[0]) && (1 == mKeyFSM[1]))
            return true;
        return false;
    }

    private void clearKeyFSM() {
        for (int index = 0; index < mKeyFSM.length; index++) {
            mKeyFSM[index] = 0;
        }
    }

    public void toggleDevMode() {
        String keyDevMode = mAppContext.getString(R.string.pref_key_dev_mode);
        String keyPlayMode = mAppContext.getString(R.string.pref_key_play_mode);
        KANTVLog.d(TAG, "dev_mode: " + mSettings.getDevMode());
        boolean bEnableDevMode = (!mSettings.getDevMode());
        if (bEnableDevMode) {
            KANTVLog.d(TAG, "enter dev mode");
            //Toast.makeText(mActivity,   "enter dev mode", Toast.LENGTH_SHORT).show();
        } else {
            KANTVLog.d(TAG, "exit dev mode");
            //Toast.makeText(mActivity,   "exit dev mode", Toast.LENGTH_SHORT).show();
        }

        SharedPreferences.Editor editor = mSharedPreferences.edit();
        editor.putBoolean(keyDevMode, bEnableDevMode);
        editor.commit();

        KANTVLog.d(TAG, "dump mode " + mSettings.getDumpMode());
        KANTVLog.d(TAG, "dev  mode " + mSettings.getDevMode());
        KANTVLog.d(TAG, "dump duration " + KANTVUtils.getDumpDuration());
        KANTVLog.d(TAG, "play engine " + mSettings.getPlayerEngine());
        KANTVLog.d(TAG, "drm scheme " + KANTVUtils.getDrmSchemeName());
        if (bEnableDevMode) {
            setHudViewHolderVisibility(View.VISIBLE);
        } else {
            setHudViewHolderVisibility(View.INVISIBLE);
        }
        KANTVUtils.setDevMode(KANTVUtils.getTEEConfigFilePath(mAppContext), mSettings.getDevMode(), mSettings.getDumpMode(), mSettings.getPlayerEngine(), KANTVUtils.getDumpDuration(), KANTVUtils.getDumpSize(), KANTVUtils.getDumpCounts());
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK &&
                keyCode != KeyEvent.KEYCODE_VOLUME_UP &&
                keyCode != KeyEvent.KEYCODE_VOLUME_DOWN &&
                keyCode != KeyEvent.KEYCODE_VOLUME_MUTE &&
                keyCode != KeyEvent.KEYCODE_MENU &&
                keyCode != KeyEvent.KEYCODE_CALL &&
                keyCode != KeyEvent.KEYCODE_ENDCALL;

        KANTVLog.d(TAG, "keyCode:" + keyCode);

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            exitLoopPlayMode();
        }


        if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
            if (isKeyFSMCleared()) {
                //KANTVLog.d(TAG, "key up press, recv first up");
                mKeyFSM[0] = 1;
            } else if ((1 == mKeyFSM[0]) && (1 == mKeyFSM[1])) {
                //KANTVLog.d(TAG, "key up press, but above 2 valid key up event, clearKeyFSM");
                clearKeyFSM();
            } else if (1 == mKeyFSM[0]) {
                //KANTVLog.d(TAG, "key up press, recv second valid up");
                mKeyFSM[1] = 1;
            } else {
                //KANTVLog.d(TAG, "key up press, but not valid up, clearKeyFSM");
                clearKeyFSM();
            }
        } else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
            if (isGetValidKeyUpEvent()) {
                if ((0 == mKeyFSM[2]) && (0 == mKeyFSM[3])) {
                    //KANTVLog.d(TAG, "key down press, recv first down");
                    mKeyFSM[2] = 1;
                } else if ((1 == mKeyFSM[2]) && (1 == mKeyFSM[3])) {
                    //KANTVLog.d(TAG, "key down press, but above 2 valid key down event, clearKeyFSM");
                    clearKeyFSM();
                } else if (1 == mKeyFSM[2]) {
                    //KANTVLog.d(TAG, "key down press, recv second up");
                    mKeyFSM[3] = 1;
                } else {
                    //KANTVLog.d(TAG, "key down press, but not valid up, clearKeyFSM");
                    clearKeyFSM();
                }
            } else {
                //KANTVLog.d(TAG, "key down press, but valid key up event not gotted, clearKeyFSM");
                clearKeyFSM();
            }
        } else {
            //KANTVLog.d(TAG, "not key down/ up clearKeyFSM");
            clearKeyFSM();
        }

        if (isKeyFSMSetted()) {
            clearKeyFSM();
            toggleDevMode();
        }


        if (isInPlaybackState() && isKeyCodeSupported && mMediaController != null) {
            if (KANTVUtils.isRunningOnTV()) {
                if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {

                }
            }
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                    keyCode == KeyEvent.KEYCODE_DPAD_CENTER ||
                    keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                    mMediaController.show();
                } else {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_PLAY) {
                if (!mMediaPlayer.isPlaying()) {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE) {
                if (mMediaPlayer.isPlaying()) {
                    pause();
                    mMediaController.show();
                }
                return true;
            } else {
                toggleMediaControlsVisibility();
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    private void toggleMediaControlsVisibility() {
        if (mMediaController.isShowing()) {
            mMediaController.hide();
        } else {
            mMediaController.show();
        }
    }
    //end added


    @Override
    public void start() {
        KANTVLog.d(TAG, "start");
        if (isInPlaybackState()) {
            KANTVLog.d(TAG, "calling mMediaPlayer.start()");
            if (!(mMediaPlayer instanceof AndroidMediaPlayer)) {
                mMediaPlayer.start();
            }
            mCurrentState = STATE_PLAYING;
            _notifyMediaStatus();
        } else {
            KANTVLog.d(TAG, "already start playing");
        }
        mTargetState = STATE_PLAYING;
    }

    private void exitLoopPlayMode() {
        if (mSettings.getPlayMode() != KANTVUtils.PLAY_MODE_NORMAL) {
            String keyPlayMode = mAppContext.getString(R.string.pref_key_play_mode);
            SharedPreferences.Editor editor = mSharedPreferences.edit();
            editor.putString(keyPlayMode, "0");
            editor.commit();

            KANTVUtils.perfResetLoopPlayBeginTime();
        }
    }

    public long getBufferingCounts() {
        if (mBufferingCounts - 2 < 0)
            return 0;
        return mBufferingCounts - 2;
    }


    public void startUIBuffering(String status) {
        KANTVLog.d(TAG, "start UIBuffering {");
        mBufferingCounts++;

        /*
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog == null) {
                    mProgressDialog = new ProgressDialog(mActivity);
                    mProgressDialog.setMessage(status);
                    mProgressDialog.setIndeterminate(true);
                    mProgressDialog.setCancelable(true);
                    mProgressDialog.setCanceledOnTouchOutside(false);

                    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialogInterface) {
                        
                            exitLoopPlayMode();

                            stopPlayback();
                            mActivity.finish();
                        }
                    });
                    mProgressDialog.show();
                }
            }


        });
        */
    }

    public void stopUIBuffering() {
        KANTVLog.d(TAG, "stop UIBuffering }");
        /*
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                    mProgressDialog = null;
                }
                if (mCurrentState != STATE_PLAYING) {
                    if (mMediaPlayer instanceof KANTVExo2MediaPlayer) {
                        KANTVLog.d(TAG, "resetting to STATE_PLAYING");
                        mCurrentState = STATE_PLAYING;
                    }
                }
                if (isInPlaybackState()) {
                    //KANTVLog.d(TAG, "mMediaPlayer.start()");
                    //mMediaPlayer.start();
                    //KANTVLog.d(TAG, "mCurrentState = STATE_PLAYING");
                    //mCurrentState = STATE_PLAYING;
                }
                //mTargetState = STATE_PLAYING;
            }
        });
        */
    }


    public void startAudioAnimation() {
        boolean isPureAudioContent = false;
        FFmpegMediaPlayer mp = null;
        String tmpString = null;
        String videoInfo = null;
        String audioInfo = null;
        if (mMediaPlayer == null) {
            return;
        }

        if (mMediaPlayer instanceof FFmpegMediaPlayer) {
            mp = (FFmpegMediaPlayer) mMediaPlayer;
            videoInfo = mp.getVideoInfo();
            audioInfo = mp.getAudioInfo();
        } else {
            videoInfo = mMediaPlayer.getVideoInfo();
            audioInfo = mMediaPlayer.getAudioInfo();
        }

        if (videoInfo != null) {
            tmpString = KANTVUtils.convertLongString(videoInfo, 30);
            KANTVLog.d(TAG, "videoInfo:" + tmpString);
            if (videoInfo.contains("unknown"))
                isPureAudioContent = true;
        } else {
            KANTVLog.d(TAG, "is pure audio content");
            isPureAudioContent = true;
        }

        if (audioInfo != null) {
            tmpString = KANTVUtils.convertLongString(audioInfo, 30);
        } else {
            KANTVLog.d(TAG, "is pure video content");
        }


        if ((videoInfo != null) && (audioInfo != null)) {
            KANTVLog.d(TAG, "videoinfo :" + videoInfo);
            KANTVLog.d(TAG, "audioInfo :" + audioInfo);
            if (videoInfo.contains("unknown")) {
                if (audioInfo.contains("unknown")) {
                    KANTVLog.d(TAG, "special case, pls check");
                    //TODO
                    //stopPlayback();
                    //mCurrentState = STATE_ERROR;
                    //mTargetState = STATE_ERROR;
                    //mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
                    return;
                }
            }
        }


        if ((mAudioAnimatonView != null) && isPureAudioContent) {
            KANTVLog.d(TAG, "start audio animation {");
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    KANTVLog.d(TAG, "media type:" + mMediaType);
                    mAudioAnimatonView.setVisibility(View.VISIBLE);
                    mAudioAnimatonView.start();
                    if (mTextClock != null) {
                        //mTextClock.setVisibility(View.VISIBLE);
                    }

                    if (mTextInfo != null) {
                        mTextInfo.setVisibility(View.VISIBLE);
                    }

                }
            });
        }
    }

    public void stopAudioAnimation() {
        boolean isPureAudioContent = false;
        FFmpegMediaPlayer mp = null;
        String tmpString = null;
        String videoInfo = null;
        String audioInfo = null;
        if (mMediaPlayer == null) {
            return;
        }

        if (mMediaPlayer instanceof FFmpegMediaPlayer) {
            videoInfo = mp.getVideoInfo();
            audioInfo = mp.getAudioInfo();
        } else {
            videoInfo = mMediaPlayer.getVideoInfo();
            audioInfo = mMediaPlayer.getAudioInfo();
        }

        if (videoInfo != null) {
            tmpString = KANTVUtils.convertLongString(videoInfo, 30);
            if (videoInfo.contains("unknown"))
                isPureAudioContent = true;
        }

        if (audioInfo != null) {
            tmpString = KANTVUtils.convertLongString(audioInfo, 30);
        }

        if ((mAudioAnimatonView != null) && isPureAudioContent) {
            KANTVLog.d(TAG, "stop audio animation }");
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mAudioAnimatonView.setVisibility(View.INVISIBLE);
                    mAudioAnimatonView.stop();
                }
            });
        }


    }

    @Override
    public void pause() {
        if (isInPlaybackState()) {
            if (mMediaPlayer.isPlaying()) {
                mMediaPlayer.pause();
                mCurrentState = STATE_PAUSED;
                _notifyMediaStatus();
            }
        }
        mTargetState = STATE_PAUSED;
    }

    public void suspend() {
        release(false);
    }

    public void resume() {
        openVideo();
    }

    @Override
    public int getDuration() {
        if (isInPlaybackState()) {
            return (int) mMediaPlayer.getDuration();
        }

        return -1;
    }

    @Override
    public int getCurrentPosition() {
        if (isInPlaybackState()) {
            return (int) mMediaPlayer.getCurrentPosition();
        }
        return 0;
    }

    @Override
    public void seekTo(int msec) {
        KANTVLog.d(TAG, "seekTo: " + msec + " msecs");
        if (0 == msec)
            return;

        if ((mMediaPlayer != null) && (mMediaPlayer instanceof AndroidMediaPlayer)) {
            KANTVLog.d(TAG, "AndroidMediaPlayer is using, becareful");
        }

        if (isInPlaybackState()) {
            KANTVLog.d(TAG, "in playback state");
            mSeekStartTime = System.currentTimeMillis();
            mMediaPlayer.seekTo(msec);
            mSeekWhenPrepared = 0;
        } else {
            KANTVLog.d(TAG, "not in playback state");
            mSeekWhenPrepared = msec;
        }
    }

    @Override
    public boolean isPlaying() {
        return isInPlaybackState() && mMediaPlayer.isPlaying();
    }

    @Override
    public int getBufferPercentage() {
        if (mMediaPlayer != null) {
            return mCurrentBufferPercentage;
        }
        return 0;
    }

    private boolean isInPlaybackState() {
        return (mMediaPlayer != null &&
                mCurrentState != STATE_ERROR &&
                mCurrentState != STATE_IDLE &&
                mCurrentState != STATE_PREPARING);
    }

    @Override
    public boolean canPause() {
        return mCanPause;
    }

    @Override
    public boolean canSeekBackward() {
        return mCanSeekBack;
    }

    @Override
    public boolean canSeekForward() {
        return mCanSeekForward;
    }

    @Override
    public int getAudioSessionId() {
        return 0;
    }

    public long getPlayedTime() {
        long playTime = 0;
        long currentPos = 0;
        if (isPlaying()) {
            if (mMediaPlayer instanceof FFmpegMediaPlayer) {
                FFmpegMediaPlayer mp = (FFmpegMediaPlayer) mMediaPlayer;
                playTime = mp.getPlayTime();
            }
            currentPos = mMediaPlayer.getCurrentPosition();
            KANTVLog.d(TAG, "playedTime : " + playTime + " secs");
            KANTVLog.d(TAG, "currentPos : " + currentPos / 1000 + " secs");
            return playTime;

        }

        return 0;
    }

    private static final int[] s_allAspectRatio = {
            IRenderView.AR_ASPECT_FIT_PARENT,
            IRenderView.AR_ASPECT_FILL_PARENT,
            IRenderView.AR_ASPECT_WRAP_CONTENT,
            IRenderView.AR_MATCH_PARENT,
            IRenderView.AR_16_9_FIT_PARENT,
            IRenderView.AR_4_3_FIT_PARENT};

    private static int mPreviousAspectRatioIndex = 2;
    private static int mCurrentAspectRatioIndex = 3;
    private static int mCurrentAspectRatio = s_allAspectRatio[3];


    public void setAspectRatio(int aspectRatio) {
        mCurrentAspectRatio = aspectRatio;
        if (mRenderView != null)
            mRenderView.setAspectRatio(mCurrentAspectRatio);
    }

    public int toggleAspectRatio() {
        mCurrentAspectRatioIndex++;
        mCurrentAspectRatioIndex %= s_allAspectRatio.length;

        mCurrentAspectRatio = s_allAspectRatio[mCurrentAspectRatioIndex];
        if (mRenderView != null)
            mRenderView.setAspectRatio(mCurrentAspectRatio);
        mPreviousAspectRatioIndex = mCurrentAspectRatioIndex;
        return mCurrentAspectRatio;
    }

    public boolean toggleFullscreen_old() {
        KANTVLog.d(TAG, "current aspect index: " + mCurrentAspectRatioIndex);
        KANTVLog.d(TAG, "current aspect:" + mCurrentAspectRatio);
        KANTVLog.d(TAG, "prev aspect index:" + mPreviousAspectRatioIndex);
        if (mEnableFullscreen && (mPreviousAspectRatioIndex != IRenderView.AR_ASPECT_FILL_PARENT)) {
            mEnableFullscreen = true;
            mPreviousAspectRatioIndex = IRenderView.AR_ASPECT_FILL_PARENT;
        } else if (!mEnableFullscreen && (mPreviousAspectRatioIndex != IRenderView.AR_ASPECT_FILL_PARENT)) {
            mEnableFullscreen = true;
            mPreviousAspectRatioIndex = IRenderView.AR_ASPECT_FILL_PARENT;
        } else {
            mEnableFullscreen = false;
            mPreviousAspectRatioIndex = IRenderView.AR_MATCH_PARENT;
        }

        if (mEnableFullscreen) {
            mCurrentAspectRatioIndex = IRenderView.AR_ASPECT_FILL_PARENT;
        } else {
            mCurrentAspectRatioIndex = IRenderView.AR_MATCH_PARENT;
        }

        mCurrentAspectRatio = s_allAspectRatio[mCurrentAspectRatioIndex];
        KANTVLog.d(TAG, "current aspect index: " + mCurrentAspectRatioIndex);
        KANTVLog.d(TAG, "current aspect:" + mCurrentAspectRatio);
        KANTVLog.d(TAG, "prev aspect index:" + mPreviousAspectRatioIndex);
        if (mRenderView != null)
            mRenderView.setAspectRatio(mCurrentAspectRatio);
        return mEnableFullscreen;
    }

    public boolean toggleFullscreen() {
        KANTVLog.d(TAG, "current aspect index: " + mCurrentAspectRatioIndex);
        KANTVLog.d(TAG, "current aspect:" + mCurrentAspectRatio);
        KANTVLog.d(TAG, "prev aspect index:" + mPreviousAspectRatioIndex);
        mPreviousAspectRatioIndex = mCurrentAspectRatioIndex;
        if (!mEnableFullscreen) {
            mEnableFullscreen = true;
            mCurrentAspectRatioIndex = IRenderView.AR_MATCH_PARENT;
            //mCurrentAspectRatio = IRenderView.AR_MATCH_PARENT;
            mCurrentAspectRatio = s_allAspectRatio[mCurrentAspectRatioIndex];
        } else {
            mEnableFullscreen = false;
            mCurrentAspectRatioIndex = IRenderView.AR_ASPECT_WRAP_CONTENT;
            mCurrentAspectRatio = s_allAspectRatio[mCurrentAspectRatioIndex];
        }
        KANTVLog.d(TAG, "current aspect index: " + mCurrentAspectRatioIndex);
        KANTVLog.d(TAG, "current aspect:" + mCurrentAspectRatio);
        KANTVLog.d(TAG, "prev aspect index:" + mPreviousAspectRatioIndex);
        if (mRenderView != null)
            mRenderView.setAspectRatio(mCurrentAspectRatio);
        return mEnableFullscreen;
    }

    public boolean isEnableLandscape() {
        return mEnableLandscape;
    }

    public boolean toggleLandscape() {
        mEnableLandscape = !mEnableLandscape;
        KANTVLog.d(TAG, "toggle landscape");
        if (mActivity == null)
            return false;

        if (mEnableLandscape && (mActivity.getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)) {
            mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            KANTVUtils.setScreenOrientation(KANTVUtils.SCREEN_ORIENTATION_LANDSCAPE);
            //toggleFullscreen();
        }

        if (!mEnableLandscape && (mActivity.getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)) {
            mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
            KANTVUtils.setScreenOrientation(KANTVUtils.SCREEN_ORIENTATION_PORTRAIT);
            //toggleFullscreen();
        }

        return mEnableLandscape;
    }

    public boolean toggleTFLite() {
        mEnableTFLite = !mEnableTFLite;
        //MediaPlayerCompat.setTFLite(mMediaPlayer, mEnableTFLite);
        return mEnableTFLite;
    }


    //-------------------------
    // Extend: Render
    //-------------------------
    public static final int RENDER_NONE = 0;
    public static final int RENDER_SURFACE_VIEW = 1;
    public static final int RENDER_TEXTURE_VIEW = 2;

    private List<Integer> mAllRenders = new ArrayList<Integer>();
    private int mCurrentRenderIndex = 0;
    private int mCurrentRender = RENDER_NONE;

    private void initRenders() {
        mAllRenders.clear();

        if (mSettings.getEnableSurfaceView())
            mAllRenders.add(RENDER_SURFACE_VIEW);
        if (mSettings.getEnableTextureView() && Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
            mAllRenders.add(RENDER_TEXTURE_VIEW);
        if (mSettings.getEnableNoView())
            mAllRenders.add(RENDER_NONE);

        if (mAllRenders.isEmpty())
            mAllRenders.add(RENDER_SURFACE_VIEW);
        mCurrentRender = mAllRenders.get(mCurrentRenderIndex);
        KANTVLog.d(TAG, "current render index:" + mCurrentRenderIndex);
        KANTVLog.d(TAG, "current render:" + mCurrentRender);
        setRender(mCurrentRender);
    }

    public int toggleRender() {
        mCurrentRenderIndex++;
        mCurrentRenderIndex %= mAllRenders.size();

        mCurrentRender = mAllRenders.get(mCurrentRenderIndex);
        setRender(mCurrentRender);
        return mCurrentRender;
    }

    @NonNull
    public static String getRenderText(Context context, int render) {
        String text;
        switch (render) {
            case RENDER_NONE:
                text = context.getString(R.string.VideoView_render_none);
                break;
            case RENDER_SURFACE_VIEW:
                text = context.getString(R.string.VideoView_render_surface_view);
                break;
            case RENDER_TEXTURE_VIEW:
                text = context.getString(R.string.VideoView_render_texture_view);
                break;
            default:
                text = context.getString(R.string.N_A);
                break;
        }
        return text;
    }


    public int togglePlayerEngine() {
        if (mMediaPlayer != null)
            mMediaPlayer.release();

        if (mRenderView != null)
            mRenderView.getView().invalidate();
        openVideo();
        return mSettings.getPlayerEngine();
    }

    @NonNull
    public static String getPlayerEngineText(Context context, int playerEngine) {
        String text;
        switch (playerEngine) {
            case KANTVUtils.PV_PLAYERENGINE__AndroidMediaPlayer:
                text = context.getString(R.string.VideoView_playerengine_AndroidMediaPlayer);
                break;
            case KANTVUtils.PV_PLAYERENGINE__FFmpeg:
                text = context.getString(R.string.VideoView_playerengine_FFmpeg);
                break;
            case KANTVUtils.PV_PLAYERENGINE__Exoplayer:
                text = context.getString(R.string.VideoView_playerengine_Exoplayer);
                break;
            default:
                text = context.getString(R.string.N_A);
                break;
        }
        return text;
    }

    public IMediaPlayer getPlayer() {
        return mMediaPlayer;
    }

    public IMediaPlayer createPlayer(int playerEngineType) {
        IMediaPlayer mediaPlayer = null;
        KANTVLog.d(TAG, "playEngine: " + playerEngineType);
        if ((0 == playerEngineType) && KANTVUtils.isRunningOnAmlogicBox()) {
            playerEngineType = KANTVUtils.PV_PLAYERENGINE__AndroidMediaPlayer;
        }
        KANTVLog.d(TAG, "disableAudio:" + KANTVUtils.getDisableAudioTrack());
        KANTVLog.d(TAG, "disableVideo:" + KANTVUtils.getDisableVideoTrack());
        switch (playerEngineType) {
            case KANTVUtils.PV_PLAYERENGINE__Exoplayer: {
                KANTVExo2MediaPlayer kantvExoMediaPlayer = new KANTVExo2MediaPlayer(mActivity, mAppContext);
                mediaPlayer = kantvExoMediaPlayer;

                if (mSettings.getTEEEnabled()) {
                    KANTVLog.d(TAG, "using TEE");
                    KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_TEE);
                    mediaPlayer.setDecryptMode(KANTVUtils.DECRYPT_TEE);
                } else {
                    KANTVLog.d(TAG, "not using TEE");
                    KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
                    mediaPlayer.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
                }

                if (mDrmScheme != null && (!mDrmScheme.isEmpty())) {
                    KANTVUtils.setDrmSchemeName(mDrmScheme);
                }
                if (mDrmLicenseURL != null && (!mDrmLicenseURL.isEmpty())) {
                    KANTVUtils.setDrmLicenseURL(mDrmLicenseURL);
                }
                //if (mIsLive) {
                if (true) {
                    KANTVUtils.setHLSPlaylistType(KANTVUtils.PLAYLIST_TYPE_LIVE);
                } else {
                    KANTVUtils.setHLSPlaylistType(KANTVUtils.PLAYLIST_TYPE_VOD);
                }
                mediaPlayer.setDisableAudioTrack(KANTVUtils.getDisableAudioTrack());
                mediaPlayer.setDisableVideoTrack(KANTVUtils.getDisableVideoTrack());
                mediaPlayer.setEnableDumpVideoES(KANTVUtils.getEnableDumpVideoES());
                mediaPlayer.setEnableDumpAudioES(KANTVUtils.getEnableDumpAudioES());
                mediaPlayer.setDurationDumpES(KANTVUtils.getDumpDuration());
            }
            break;


                case KANTVUtils.PV_PLAYERENGINE__FFmpeg:
            default: {
                FFmpegMediaPlayer ffMediaPlayer = null;
                if (mUri != null) {
                    ffMediaPlayer = new FFmpegMediaPlayer();

                    String scheme = mUri.getScheme();
                    if ((scheme != null) && scheme.equals("rtsp")) {
                        if (mSettings.getRtspUseTcp()) {
                            ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "rtsp_transport", "tcp");
                        }


                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "analyzemaxduration", "100");
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "analyzeduration", "1");
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "probesize", "10240");
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "flush_packets", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "packet-buffering", 0);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "framedrop", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "enable-accurate-seek", 1);
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "max-fps", 0);
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "fps", 30);
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_CODEC, "skip_loop_filter", 48);
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", IjkMediaPlayer.SDL_FCC_YV12);
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "fflags", "nobuffer");
                        //ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "max-buffer-size", 1024);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "min-frames", 3);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "start-on-prepared", 1);
                    }

                    if (KANTVUtils.getReleaseMode()) {
                        ffMediaPlayer.native_setLogLevel(FFmpegMediaPlayer.IJK_LOG_SILENT);
                    } else {
                        ffMediaPlayer.native_setLogLevel(FFmpegMediaPlayer.IJK_LOG_WARN);
                    }

                    KANTVLog.j(TAG, "FFmpeg version:" + ffMediaPlayer.getFFmpegVersion());

                    if (mSettings.getUsingMediaCodec()) {
                        KANTVLog.j(TAG, "hardware decoding by MediaCodec with ffmpeg");
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-all-videos", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-hevc", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-mpeg2", 1);
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-mpeg4", 1);
                        if (mSettings.getUsingMediaCodecAutoRotate()) {
                            ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 1);
                        } else {
                            ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-auto-rotate", 0);
                        }
                        if (mSettings.getMediaCodecHandleResolutionChange()) {
                            ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 1);
                        } else {
                            ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec-handle-resolution-change", 0);
                        }
                    } else {
                        KANTVLog.j(TAG, "software decoding with ffmpeg");
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "mediacodec", 0);
                    }

                    if (mSettings.getUsingOpenSLES()) {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "opensles", 1);
                    } else {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "opensles", 0);
                    }

                    String pixelFormat = mSettings.getPixelFormat();
                    KANTVLog.d(TAG, "pixelFormat:" + pixelFormat);
                    if (TextUtils.isEmpty(pixelFormat)) {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", FFmpegMediaPlayer.SDL_FCC_RV32);
                    } else {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", pixelFormat);
                    }
                    if (mSettings.getUsingMediaCodec()) {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", FFmpegMediaPlayer.SDL_FCC_YV12);
                    } else {
                        ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "overlay-format", FFmpegMediaPlayer.SDL_FCC_RV16);
                    }
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "enable-accurate-seek", 1);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "framedrop", 2);
                    /*
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "analyzeduration", "1");
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "start-on-prepared", 0);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "analyzemaxduration", "100");
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "probesize", "1024*64");
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_CODEC,  "skip_loop_filter", 48);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "http-detect-range-support", 0);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "flush_packets", 1);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "packet-buffering", 0);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "min-frames", 3);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "max-fps", 0);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "fps", 30);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "max-buffer-size", 1024);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "max_delay", 0);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "reconnect", 1);
                    ffMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_FORMAT, "dns_cache_clear", 1);
                    */
                }

                mediaPlayer = (IMediaPlayer) ffMediaPlayer;
                mediaPlayer.setDisableAudioTrack(KANTVUtils.getDisableAudioTrack());
                mediaPlayer.setDisableVideoTrack(KANTVUtils.getDisableVideoTrack());
                KANTVLog.d(TAG, "not using TEE");
                KANTVUtils.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
                mediaPlayer.setDecryptMode(KANTVUtils.DECRYPT_SOFT);
                mediaPlayer.setEnableDumpVideoES(KANTVUtils.getEnableDumpVideoES());
                mediaPlayer.setEnableDumpAudioES(KANTVUtils.getEnableDumpAudioES());
                mediaPlayer.setDurationDumpES(KANTVUtils.getDumpDuration());
            }
            break;
        }

        if (mSettings.getEnableDetachedSurfaceTextureView()) {
            mediaPlayer = new TextureMediaPlayer(mediaPlayer);
        }

        return mediaPlayer;
    }


    private boolean mEnableBackgroundPlay = false;

    private void initBackground() {
        mEnableBackgroundPlay = mSettings.getEnableBackgroundPlay();
        if (mEnableBackgroundPlay) {
            KANTVLog.d(TAG, "enable background play");
            MediaPlayerService.intentToStart(getContext());
            mMediaPlayer = MediaPlayerService.getMediaPlayer();
        } else {
            KANTVLog.d(TAG, "disable background play");
        }
    }

    public boolean isBackgroundPlayEnabled() {
        mEnableBackgroundPlay = mSettings.getEnableBackgroundPlay();

        return mEnableBackgroundPlay;
    }

    public void enterBackground() {
        MediaPlayerService.setMediaPlayer(mMediaPlayer);
    }

    public void stopBackgroundPlay() {
        MediaPlayerService.setMediaPlayer(null);
    }


    private String buildTimeMilli(long duration) {
        long total_seconds = duration / 1000;
        long hours = total_seconds / 3600;
        long minutes = (total_seconds % 3600) / 60;
        long seconds = total_seconds % 60;
        if (duration <= 0) {
            return "--:--";
        }
        if (hours >= 100) {
            return String.format(Locale.US, "%d:%02d:%02d", hours, minutes, seconds);
        } else if (hours > 0) {
            return String.format(Locale.US, "%02d:%02d:%02d", hours, minutes, seconds);
        } else {
            return String.format(Locale.US, "%02d:%02d", minutes, seconds);
        }
    }

    public ITrackInfo[] getTrackInfo() {
        if (mMediaPlayer == null)
            return null;

        return mMediaPlayer.getTrackInfo();
    }

    public void selectTrack(int stream) {
        MediaPlayerCompat.selectTrack(mMediaPlayer, stream);
    }

    public void deselectTrack(int stream) {
        MediaPlayerCompat.deselectTrack(mMediaPlayer, stream);
    }

    public int getSelectedTrack(int trackType) {
        return MediaPlayerCompat.getSelectedTrack(mMediaPlayer, trackType);
    }


    private boolean mIsUsingMediaCodec = true;

    private boolean mIsUsingOpenSLES = false;

    private boolean mIsUsingMediaDataSource = false;

    private boolean mIsUsingDetachedSurfaceTextureView = false;

    private boolean mIsUsingSurfaceRenders = true;

    private int mIsUsingPlayerType = KANTVUtils.PV_PLAYERENGINE__FFmpeg;

    private String mPixelFormat = "";

    public void setIsUsingMediaCodec(boolean mIsUsingMediaCodec) {
        this.mIsUsingMediaCodec = mIsUsingMediaCodec;
    }

    public void setIsUsingOpenSLES(boolean mIsUsingOpenSLES) {
        this.mIsUsingOpenSLES = mIsUsingOpenSLES;
    }

    public void setIsUsingMediaDataSource(boolean mIsUsingMediaDataSource) {
        this.mIsUsingMediaDataSource = mIsUsingMediaDataSource;
    }

    public void setIsUsingDetachedSurfaceTextureView(boolean mIsUsingDetachedSurfaceTextureView) {
        this.mIsUsingDetachedSurfaceTextureView = mIsUsingDetachedSurfaceTextureView;
    }

    public void setIsUsingSurfaceRenders(boolean mIsUsingSurfaceRenders) {
        this.mIsUsingSurfaceRenders = mIsUsingSurfaceRenders;
        initRenders();
    }

    public void setIsUsingPlayerType(int mIsUsingPlayerType) {
        this.mIsUsingPlayerType = mIsUsingPlayerType;
    }

    public void setPixelFormat(String mPixelFormat) {
        this.mPixelFormat = mPixelFormat;
    }

    public void setSpeed(float speed) {
        if (mIsUsingPlayerType == KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
            FFmpegMediaPlayer ijkMediaPlayer = (FFmpegMediaPlayer) mMediaPlayer;
            ijkMediaPlayer.setOption(FFmpegMediaPlayer.OPT_CATEGORY_PLAYER, "soundtouch", 1);
            ijkMediaPlayer.setSpeed(speed);
            mMediaPlayer.start();
        }
    }

    public float getSpeed() {
        if (mIsUsingPlayerType == KANTVUtils.PV_PLAYERENGINE__FFmpeg) {
            FFmpegMediaPlayer ijkMediaPlayer = (FFmpegMediaPlayer) mMediaPlayer;
            return ijkMediaPlayer.getSpeed();
        } else {
            return 1.0f;
        }
    }


    // not used since kantv-1.0
    public void showMediaInfo() {
        if (!bShowInfo) {
            bShowInfo = true;
            setHudViewHolderVisibility(View.VISIBLE);
            if (mMediaPlayer.isPlaying()) {
                mMediaController.show();
            }
        } else {
            bShowInfo = false;
            setHudViewHolderVisibility(View.INVISIBLE);
        }
        return;

        /*
        if (mMediaPlayer == null)
            return;

        int selectedVideoTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_VIDEO);
        int selectedAudioTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_AUDIO);
        int selectedSubtitleTrack = MediaPlayerCompat.getSelectedTrack(mMediaPlayer, ITrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT);

        TableLayoutBinder builder = new TableLayoutBinder(getContext());
        mSettings.updateUILang(mActivity);
        //builder.appendSection(R.string.mi_player);
        //builder.appendRow2(R.string.mi_player, MediaPlayerCompat.getName(mMediaPlayer));
        builder.appendSection(R.string.mi_media);
        builder.appendRow2(R.string.mi_resolution, KANTVUtils.buildResolution(mVideoWidth, mVideoHeight, mVideoSarNum, mVideoSarDen));
        String regEx = "[`~!@#$%^&*()+=|{}:;\\\\[\\\\].<>/?~！@（）——+|{}【】‘；：”“’。，、？']";
        String videoTitle = Pattern.compile(regEx).matcher(mVideoTitle).replaceAll("").trim();
        builder.appendRow2(R.string.mi_name, videoTitle);
        builder.appendRow2(R.string.mi_length, KANTVUtils.buildTimeMilli(mMediaPlayer.getDuration()));

        String tmpString = "";
        if (mMediaPlayer instanceof FFmpegMediaPlayer) {
            FFmpegMediaPlayer mp = (FFmpegMediaPlayer) mMediaPlayer;
            int vdec = mp.getVideoDecoder();
            switch (vdec) {
                case FFmpegMediaPlayer.FFP_PROPV_DECODER_AVCODEC:
                    tmpString = "ffmpeg";
                    break;
                case FFmpegMediaPlayer.FFP_PROPV_DECODER_MEDIACODEC:
                    tmpString = "MediaCodec";
                    break;
                default:
                    tmpString = "unknown";
                    break;
            }
            tmpString = tmpString + mActivity.getBaseContext().getString(R.string.playengine) + ": " + "FFmpeg";
            tmpString = KANTVUtils.convertLongString(tmpString, 30);
            builder.appendRow2(R.string.vdec, tmpString);
        }

        if (mMediaPlayer instanceof KANTVExo2MediaPlayer) {
            tmpString = "MediaCodec  " + mActivity.getBaseContext().getString(R.string.playengine) + ": " + "ExoPlayer";
            tmpString = KANTVUtils.convertLongString(tmpString, 30);
            builder.appendRow2(R.string.vdec, tmpString);
        }
        if (mMediaPlayer instanceof AndroidMediaPlayer) {
            tmpString = "MediaCodec  " + mActivity.getBaseContext().getString(R.string.playengine) + ": " + " AndroidMediaPlayer";
            tmpString = KANTVUtils.convertLongString(tmpString, 30);
            builder.appendRow2(R.string.vdec, tmpString);
        }
        if (!mMediaPlayer.isClearContent()) {
            String drmInfo = KANTVUtils.getDrminfoString(mActivity, mMediaPlayer, R.string.drminfo, R.string.encryptinfo, R.string.decrypt);
            tmpString = KANTVUtils.convertLongString(drmInfo, 30);
            builder.appendRow2(R.string.clear_content, tmpString);
        } else {
            builder.appendRow2(R.string.clear_content, "Clear Content");
        }
        builder.appendRow2(R.string.disable_audiotrack, (KANTVUtils.getDisableAudioTrack() ? "YES" : "NO"));
        StringBuilder boardInfo = new StringBuilder();
        boardInfo.append("Device:" + Build.DEVICE + " ABI:" + Build.CPU_ABI + " Product:" + Build.PRODUCT);
        tmpString = boardInfo.toString() + "(" + mActivity.getBaseContext().getString(R.string.network_info) + ": " + KANTVUtils.getNetworkTypeString() + ")";
        tmpString = KANTVUtils.convertLongString(tmpString, 30);
        builder.appendRow2(R.string.device_info, tmpString);

        String videoProfileLevel = " ";
        ITrackInfo trackInfos[] = mMediaPlayer.getTrackInfo();
        if (trackInfos != null) {
            int index = -1;
            for (ITrackInfo trackInfo : trackInfos) {
                index++;

                int trackType = trackInfo.getTrackType();
                if (index == selectedVideoTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_video_track));
                } else if (index == selectedAudioTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_audio_track));
                } else if (index == selectedSubtitleTrack) {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index) + " " + getContext().getString(R.string.mi__selected_subtitle_track));
                } else {
                    builder.appendSection(getContext().getString(R.string.mi_stream_fmt1, index));
                }
                builder.appendRow2(R.string.mi_type, buildTrackType(trackType));
                builder.appendRow2(R.string.mi_language, buildLanguage(trackInfo.getLanguage()));

                IMediaFormat mediaFormat = trackInfo.getFormat();
                if (mediaFormat == null) {
                } else if (mediaFormat instanceof IjkMediaFormat) {
                    switch (trackType) {
                        case ITrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                            //builder.appendRow2(R.string.mi_video_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_NAME_UI));
                            //builder.appendRow2(R.string.mi_profile_level, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI));
                            //builder.appendRow2(R.string.mi_pixel_format, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PIXEL_FORMAT_UI));
                            //builder.appendRow2(R.string.mi_resolution, mediaFormat.getString(IjkMediaFormat.KEY_IJK_RESOLUTION_UI));
                            //builder.appendRow2(R.string.mi_frame_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_FRAME_RATE_UI));
                            //builder.appendRow2(R.string.mi_bit_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_BIT_RATE_UI));
                            videoProfileLevel = " (" + mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI) + ") ";
                            break;
                        case ITrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                            //builder.appendRow2(R.string.mi_audio_codec, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_NAME_UI));
                            //builder.appendRow2(R.string.mi_profile_level, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CODEC_PROFILE_LEVEL_UI));
                            //builder.appendRow2(R.string.mi_sample_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_SAMPLE_RATE_UI));
                            //builder.appendRow2(R.string.mi_channels, mediaFormat.getString(IjkMediaFormat.KEY_IJK_CHANNEL_UI));
                            //builder.appendRow2(R.string.mi_bit_rate, mediaFormat.getString(IjkMediaFormat.KEY_IJK_BIT_RATE_UI));
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        if (mMediaPlayer instanceof FFmpegMediaPlayer) {
            FFmpegMediaPlayer mp = (FFmpegMediaPlayer) mMediaPlayer;
            String videoInfo = mp.getVideoInfo();
            String audioInfo = mp.getAudioInfo();
            if (videoInfo != null) {
                tmpString = KANTVUtils.convertLongString(videoInfo.substring(6, videoInfo.indexOf(",")) + videoProfileLevel + videoInfo.substring(videoInfo.indexOf(",")), 30);
                builder.appendRow2(R.string.videoinfo, tmpString);
            }
            if (audioInfo != null) {
                tmpString = KANTVUtils.convertLongString(audioInfo.substring(6), 30);
                builder.appendRow2(R.string.audioinfo, tmpString);
            }
        }

        AlertDialog.Builder adBuilder = builder.buildAlertDialogBuilder();
        adBuilder.setTitle(R.string.media_information);
        adBuilder.setNegativeButton(R.string.close, null);
        adBuilder.show();
        */
    }

    // not used since kantv-1.0
    public void showCalendar() {
        if (mMediaPlayer == null)
            return;

        TableLayoutBinder builder = new TableLayoutBinder(getContext());
        mSettings.updateUILang(mActivity);
        builder.appendCalendar();

        AlertDialog.Builder adBuilder = builder.buildAlertDialogBuilder();
        adBuilder.setTitle(R.string.show_calendar);
        adBuilder.setNegativeButton(R.string.close, null);
        adBuilder.show();
    }


    // not used since kantv-1.0
    //@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void screenShot() {
        KANTVLog.d(TAG, "ANDROID SDK:" + String.valueOf(android.os.Build.VERSION.SDK_INT));
        KANTVLog.d(TAG, "OS Version:" + android.os.Build.VERSION.RELEASE);
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                //mActivity.screenShotWithVideo();
            } else {
                Toast.makeText(mActivity, "ScreenShot(with video) not supported under Android L(5.0)", Toast.LENGTH_SHORT).show();
                //just for development purpose
                //mActivity.screenShotWithoutVideo();
            }
        } catch (ActivityNotFoundException exception) {
            KANTVLog.d(TAG, "ScreenShot(with video) failed: " + exception.toString());
            //mActivity.screenShotWithoutVideo();
        }
    }


    private String buildTrackType(int type) {
        Context context = getContext();
        switch (type) {
            case ITrackInfo.MEDIA_TRACK_TYPE_VIDEO:
                return context.getString(R.string.TrackType_video);
            case ITrackInfo.MEDIA_TRACK_TYPE_AUDIO:
                return context.getString(R.string.TrackType_audio);
            case ITrackInfo.MEDIA_TRACK_TYPE_SUBTITLE:
                return context.getString(R.string.TrackType_subtitle);
            case ITrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT:
                return context.getString(R.string.TrackType_timedtext);
            case ITrackInfo.MEDIA_TRACK_TYPE_METADATA:
                return context.getString(R.string.TrackType_metadata);
            case ITrackInfo.MEDIA_TRACK_TYPE_UNKNOWN:
            default:
                return context.getString(R.string.TrackType_unknown);
        }
    }

    private String buildLanguage(String language) {
        if (TextUtils.isEmpty(language))
            return "und";
        return language;
    }

    public boolean isExoPlayer() {
        return mIsExoplayer;
    }

    public static native int kantv_anti_remove_rename_this_file();

}
