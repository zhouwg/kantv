/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package com.cdeos.kantv.player.ffplayer;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Color;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.ViewCompat;
import androidx.core.view.ViewPropertyAnimatorListenerAdapter;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.lifecycle.Lifecycle;

import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TableLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.blankj.utilcode.util.ConvertUtils;
import com.blankj.utilcode.util.SPUtils;
import com.blankj.utilcode.util.StringUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.cdeos.kantv.player.common.widgets.CaptionStyleCompat;
import com.cdeos.kantv.player.ffplayer.media.VoisePlayingIcon;
import com.cdeos.kantv.R;

import com.cdeos.kantv.player.common.listener.PlayerViewListener;
import com.cdeos.kantv.player.common.utils.AnimHelper;
import com.cdeos.kantv.player.common.utils.CommonPlayerUtils;
import com.cdeos.kantv.player.common.utils.Constants;
import com.cdeos.kantv.player.common.utils.PlayerConfigShare;
import com.cdeos.kantv.player.common.utils.TimeFormatUtils;
import com.cdeos.kantv.player.common.utils.TrackInfoUtils;
import com.cdeos.kantv.player.common.widgets.BottomBarView;

import com.cdeos.kantv.player.common.widgets.DialogScreenShot;

import com.cdeos.kantv.player.common.widgets.SettingPlayerView;
import com.cdeos.kantv.player.common.widgets.SettingSubtitleView;
import com.cdeos.kantv.player.common.widgets.SkipTipView;
import com.cdeos.kantv.player.common.widgets.TopBarView;

import com.cdeos.kantv.player.ffplayer.media.CDEOSVideoView;
import com.cdeos.kantv.player.ffplayer.media.MediaPlayerParams;
import com.cdeos.kantv.player.subtitle.SubtitleManager;
import com.cdeos.kantv.player.subtitle.SubtitleParser;
import com.cdeos.kantv.player.subtitle.SubtitleView;
import com.cdeos.kantv.player.subtitle.util.TimedTextObject;
import com.cdeos.kantv.utils.Settings;


import org.ggml.ggmljava;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import cdeos.media.player.KANTVDRM;
import cdeos.media.player.IMediaPlayer;
import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;
import cdeos.media.player.bean.ExoTrackInfoBean;
import cdeos.media.player.bean.IJKTrackInfoBean;
import cdeos.media.player.bean.TrackInfoBean;
import cdeos.media.player.misc.ITrackInfo;
import cdeos.media.player.misc.IjkTrackInfo;

public class FFPlayerView extends FrameLayout implements PlayerViewListener {
    private final static String TAG = FFPlayerView.class.getName();

    private static final int HIDE_VIEW_ALL = 0;

    private static final int HIDE_VIEW_AUTO = 1;

    private static final int HIDE_VIEW_LOCK_SCREEN = 2;

    private static final int HIDE_VIEW_END_GESTURE = 3;

    private static final int HIDE_VIEW_EDIT = 4;


    private static final int MAX_VIDEO_SEEK = 1000;

    private static final int DEFAULT_HIDE_TIMEOUT = 5000;

    private static final int MSG_UPDATE_SEEK = 10086;

    private static final int MSG_ENABLE_ORIENTATION = 10087;

    private static final int MSG_UPDATE_SUBTITLE = 10088;

    private static final int MSG_SET_SUBTITLE_SOURCE = 10089;

    private static final int INVALID_VALUE = -1;


    private CDEOSVideoView mVideoView;
    private TableLayout mHudView;
    private VoisePlayingIcon mAudioAnimationView;
    private boolean mBackPressed;

    private TopBarView topBarView;

    private BottomBarView bottomBarView;

    private SkipTipView skipTipView;

    private SkipTipView skipSubView;

    private ProgressBar mLoadingView;

    private TextView mTvVolume;

    private TextView mTvBrightness;

    private TextView mSkipTimeTv;

    private FrameLayout mFlTouchLayout;

    private FrameLayout mFlVideoBox;

    private ImageView mIvPlayerLock;

    private ImageView mIvScreenShot;

    private Context mAppContext;
    private Settings mSettings;
    private SharedPreferences mSharedPreferences;
    private IMediaPlayer mMediaPlayer;

    private AppCompatActivity mAttachActivity;

    private AudioManager mAudioManager;

    private GestureDetector mGestureDetector;

    private OrientationEventListener mOrientationListener;

    private IMediaPlayer.OnOutsideListener mOutsideListener;

    private long mPlayPos = 0;

    private int mMaxVolume;

    private boolean mIsForbidTouch = false;

    private boolean mIsShowBar = true;

    private boolean mIsSeeking;

    private long mTargetPosition = INVALID_VALUE;

    private long mCurPosition = INVALID_VALUE;

    private int mCurVolume = INVALID_VALUE;

    private float mCurBrightness = INVALID_VALUE;

    private int mInitHeight;

    private int mWidthPixels;

    private boolean mIsNeverPlay = true;

    private long mSkipPosition = INVALID_VALUE;

    private boolean mIsPlayComplete = false;

    private boolean isUseSurfaceView;

    private boolean isUsemediaCodeC;

    private boolean isUseOpenSLES;

    private int usePlayerType;

    private String usePixelFormat;

    private boolean mIsIjkPlayerReady = false;

    private boolean mIsRenderingStart = false;

    private boolean isQueryNetworkSubtitle = false;

    private boolean isAutoLoadLocalSubtitle = false;

    private boolean isAutoLoadNetworkSubtitle = false;

    private String subtitleDownloadFolder;


    private List<TrackInfoBean> audioTrackList = new ArrayList<>();

    private List<TrackInfoBean> subtitleTrackList = new ArrayList<>();


    private Runnable mHideBarRunnable = () -> hideView(HIDE_VIEW_AUTO);

    private Runnable mHideTouchViewRunnable = () -> hideView(HIDE_VIEW_END_GESTURE);

    private Runnable mHideSkipTipRunnable = this::_hideSkipTip;

    private Runnable mHideSkipSubRunnable = this::_hideSkipSub;


    private SubtitleManager subtitleManager;

    private String mVideoName;
    private String mVideoUrl;

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                case MSG_UPDATE_SEEK:
                    long pos = _setProgress();
                    if (!mIsSeeking && mIsShowBar && mVideoView.isPlaying()) {
                        msg = obtainMessage(MSG_UPDATE_SEEK);
                        sendMessageDelayed(msg, 1000 - (pos % 1000));
                    }
                    break;

                case MSG_ENABLE_ORIENTATION:
                    setOrientationEnable(true);
                    break;

                case MSG_UPDATE_SUBTITLE:
                    CDELog.j(TAG, "update subtitle");
                    if (topBarView.getSubtitleSettingView().isLoadSubtitle()
                            && subtitleManager.isShowExternalSubtitle()) {
                        long position = mVideoView.getCurrentPosition() + (int) (topBarView.getSubtitleSettingView().getTimeOffset() * 1000);
                        subtitleManager.seekExSubTo(position);
                        msg = obtainMessage(MSG_UPDATE_SUBTITLE);
                        sendMessageDelayed(msg, 1000);
                    }
                    break;

                case MSG_SET_SUBTITLE_SOURCE:
                    CDELog.j(TAG, "update subtitle source");
                    TimedTextObject subtitleObj = (TimedTextObject) msg.obj;
                    topBarView.getSubtitleSettingView().setLoadSubtitle(true);
                    topBarView.getSubtitleSettingView().setSubtitleLoadStatus(true);
                    subtitleManager.setExSubData(subtitleObj);
                    Toast.makeText(getContext(), "load subtitle ok", Toast.LENGTH_LONG).show();
                    break;
            }
        }
    };

    public FFPlayerView(Context context) {
        this(context, null);
    }

    public FFPlayerView(Context context, AttributeSet attrs) {
        super(context, attrs);

        initViewBefore(context);

        initView();

        initViewCallBak();

        initViewAfter();
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (mInitHeight == 0) {
            mInitHeight = getHeight();
            mWidthPixels = getResources().getDisplayMetrics().widthPixels;
        }
    }

    private void initViewBefore(Context context) {
        if (!(context instanceof AppCompatActivity)) {
            throw new IllegalArgumentException("Context must be AppCompatActivity");
        }

        mAttachActivity = (AppCompatActivity) context;
        mAppContext = context.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);


        View.inflate(context, R.layout.layout_ijk_player_view_v2, this);

        mOrientationListener = new OrientationEventListener(mAttachActivity) {
            @SuppressLint("SourceLockedOrientationActivity")
            @Override
            public void onOrientationChanged(int orientation) {
                CDELog.d(TAG, "on orientation changed, orientation is:" + orientation);
                if (mIsNeverPlay) {
                    return;
                }

                if (orientation >= 60 && orientation <= 120) {
                    mAttachActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE);
                } else if (orientation >= 240 && orientation <= 300) {
                    mAttachActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
                }
            }
        };

        mAudioManager = (AudioManager) mAttachActivity.getSystemService(Context.AUDIO_SERVICE);
        if (mAudioManager != null)
            mMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);


        isUseSurfaceView = SPUtils.getInstance().getBoolean("surface_renders");

        isUsemediaCodeC = SPUtils.getInstance().getBoolean("media_code_c");

        isUseOpenSLES = SPUtils.getInstance().getBoolean("open_sles");

        usePlayerType = SPUtils.getInstance().getInt("player_type", Constants.PLAYERENGINE__Exoplayer);

        usePixelFormat = SPUtils.getInstance().getString("pixel_format", "fcc-_es2");
    }

    private void initView() {
        mVideoView = findViewById(R.id.ijk_video_view);
        mHudView = (TableLayout) findViewById(R.id.hud_view);
        mAudioAnimationView = (VoisePlayingIcon) findViewById(R.id.audio_animation_view);

        SubtitleView subtitleView = findViewById(R.id.subtitle_view);
        subtitleManager = new SubtitleManager(subtitleView);

        bottomBarView = findViewById(R.id.bottom_bar_view);
        topBarView = findViewById(R.id.top_bar_view);
        skipTipView = findViewById(R.id.skip_tip_view);
        skipSubView = findViewById(R.id.skip_subtitle_view);

        mLoadingView = findViewById(R.id.pb_loading);
        mTvVolume = findViewById(R.id.tv_volume);
        mTvBrightness = findViewById(R.id.tv_brightness);
        mSkipTimeTv = findViewById(R.id.tv_skip_time);
        mFlTouchLayout = findViewById(R.id.fl_touch_layout);

        mFlVideoBox = findViewById(R.id.fl_video_box);

        mIvPlayerLock = findViewById(R.id.iv_player_lock);
        mIvScreenShot = findViewById(R.id.iv_player_shot);

        initSubtitleSettingView();

        initPlayerSettingView();

        mVideoView.setActivity(mAttachActivity);

        mVideoView.setHudView(mAttachActivity, mHudView);
        mVideoView.setAudioView(mAudioAnimationView);
        setFullScreen();
    }

    @SuppressLint("ClickableViewAccessibility")
    private void initViewCallBak() {
        View.OnTouchListener mPlayerTouchListener = (v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    mHandler.removeCallbacks(mHideBarRunnable);
                    break;
                case MotionEvent.ACTION_UP:
                    _endGesture();
                    break;
            }
            return mGestureDetector.onTouchEvent(event);
        };

        GestureDetector.OnGestureListener mPlayerGestureListener = new GestureDetector.SimpleOnGestureListener() {
            private boolean isDownTouch;

            private boolean isVolume;

            private boolean isLandscape;

            @Override
            public boolean onDown(MotionEvent e) {
                isDownTouch = true;
                return super.onDown(e);
            }

            @Override
            public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
                if (!mIsForbidTouch && !mIsNeverPlay) {
                    float mOldX = e1.getX(), mOldY = e1.getY();
                    float deltaY = mOldY - e2.getY();
                    float deltaX = mOldX - e2.getX();
                    if (isDownTouch) {

                        isLandscape = Math.abs(distanceX) >= Math.abs(distanceY);

                        isVolume = mOldX > getResources().getDisplayMetrics().widthPixels * 0.5f;
                        isDownTouch = false;
                    }

                    if (isLandscape) {
                        _onProgressSlide(-deltaX / mVideoView.getWidth());
                    } else {
                        float percent = deltaY / mVideoView.getHeight();
                        if (isVolume) {
                            _onVolumeSlide(percent);
                        } else {
                            _onBrightnessSlide(percent);
                        }
                    }
                }
                return super.onScroll(e1, e2, distanceX, distanceY);
            }

            @Override
            public boolean onSingleTapConfirmed(MotionEvent e) {
                _toggleControlBar();
                return true;
            }

            @Override
            public boolean onDoubleTap(MotionEvent e) {
                if (mIsNeverPlay) {
                    return true;
                }
                if (!mIsForbidTouch) {
                    _refreshHideRunnable();
                    _togglePlayStatus();
                }
                return true;
            }
        };

        SeekBar.OnSeekBarChangeListener mSeekListener = new SeekBar.OnSeekBarChangeListener() {

            private long curPosition;

            @Override
            public void onStartTrackingTouch(SeekBar bar) {
                mIsSeeking = true;
                _showControlBar(3600000);
                mHandler.removeMessages(MSG_UPDATE_SEEK);
                curPosition = mVideoView.getCurrentPosition();
            }

            @Override
            public void onProgressChanged(SeekBar bar, int progress, boolean fromUser) {
                if (!fromUser) {
                    return;
                }
                long duration = mVideoView.getDuration();

                mTargetPosition = (duration * progress) / MAX_VIDEO_SEEK;
                int deltaTime = (int) ((mTargetPosition - curPosition) / 1000);
                String desc;

                if (mTargetPosition > curPosition) {
                    desc = TimeFormatUtils.generateTime(mTargetPosition) + "/" + TimeFormatUtils.generateTime(duration) + "\n" + "+" + deltaTime + "秒";
                } else {
                    desc = TimeFormatUtils.generateTime(mTargetPosition) + "/" + TimeFormatUtils.generateTime(duration) + "\n" + deltaTime + "秒";
                }
                setSkipTimeText(desc);
            }

            @Override
            public void onStopTrackingTouch(SeekBar bar) {
                hideView(HIDE_VIEW_AUTO);
                mIsSeeking = false;

                seekTo((int) mTargetPosition);
                mTargetPosition = INVALID_VALUE;
                _setProgress();
                _showControlBar(DEFAULT_HIDE_TIMEOUT);
            }
        };

        IMediaPlayer.OnPreparedListener ijkPreparedCallback = iMediaPlayer -> {
            CDELog.d(TAG, "on prepared listener");
            ITrackInfo[] info = mVideoView.getTrackInfo();
            if (info != null) {
                int selectedAudioTrack = mVideoView.getSelectedTrack(IjkTrackInfo.MEDIA_TRACK_TYPE_AUDIO);
                int selectedSubTrack = mVideoView.getSelectedTrack(IjkTrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT);

                TrackInfoUtils trackInfoUtils = new TrackInfoUtils();
                trackInfoUtils.initTrackInfo(info, selectedAudioTrack, selectedSubTrack);

                audioTrackList.clear();
                subtitleTrackList.clear();
                audioTrackList.addAll(trackInfoUtils.getAudioTrackList());
                subtitleTrackList.addAll(trackInfoUtils.getSubTrackList());
                topBarView.getSubtitleSettingView().setInnerSubtitleCtrl(false);
                topBarView.getSubtitleSettingView().setSubtitleTrackList(subtitleTrackList);
                topBarView.getPlayerSettingView().setAudioTrackList(audioTrackList);
            }
        };

        IMediaPlayer.OnErrorListener ijkErrorCallback = (iMediaPlayer, i, i1) -> {
            CDELog.d(TAG, "on error listener");
            mOutsideListener.onAction(Constants.INTENT_PLAY_FAILED, 0);
            mLoadingView.setVisibility(GONE);
            return false;
        };

        IMediaPlayer.OnInfoListener ijkPlayInfoCallback = (iMediaPlayer, status, extra) -> {
            CDELog.d(TAG, "on info listener");
            _switchStatus(status);
            return true;
        };

        IMediaPlayer.OnTimedTextListener ijkPlayTimedTextListener = (mp, text) -> {
            CDELog.d(TAG, "recv subtitle: " + text.getText());
            if (!CDEUtils.getTVASR())
                return;

            if (text != null) {
                subtitleManager.setInnerSub(text.getText());
            }
        };


        IMediaPlayer.OnTrackChangeListener ijkPlayTrackChangeListener = (mp, trackSelector, trackGroups, trackSelections) -> {
            CDELog.d(TAG, "on track changed listener");
            /*
            if ((mp != null) && (mp instanceof CDEOSExo2MediaPlayer)) {
                CDELog.j(TAG, "track change");
                TrackInfoUtils trackInfoUtils = new TrackInfoUtils();
                trackInfoUtils.initTrackInfo((DefaultTrackSelector) trackSelector, (TrackSelectionArray) trackSelections);

                audioTrackList.clear();
                subtitleTrackList.clear();
                audioTrackList.addAll(trackInfoUtils.getAudioTrackList());
                subtitleTrackList.addAll(trackInfoUtils.getSubTrackList());

                topBarView.getSubtitleSettingView().setInnerSubtitleCtrl(subtitleTrackList.size() > 0);
                topBarView.getSubtitleSettingView().setSubtitleTrackList(subtitleTrackList);
                topBarView.getPlayerSettingView().setAudioTrackList(audioTrackList);
            }
            */
        };


        TopBarView.TopBarListener topBarCallBack = new TopBarView.TopBarListener() {
            @Override
            public void onBack() {
                CDELog.d(TAG, "onBack because back button on top view");

                if (CDEUtils.getTVRecording()) {
                    showWarningDialog(mAttachActivity, "under recording, pls stop recording before exit playback");
                    return;
                }

                stopRecord();
                stopASR();

                stop();
                mAttachActivity.finish();
            }

            @Override
            public void topBarItemClick() {
                hideView(HIDE_VIEW_ALL);
            }
        };

        BottomBarView.BottomBarListener bottomBarCallBack = new BottomBarView.BottomBarListener() {
            @Override
            public void onPlayClick() {
                _togglePlayStatus();
            }
        };

        SkipTipView.SkipTipListener skipTipCallBack = new SkipTipView.SkipTipListener() {
            @Override
            public void onCancel() {
                mHandler.removeCallbacks(mHideSkipTipRunnable);
                _hideSkipTip();
            }

            @Override
            public void onSkip() {
                mLoadingView.setVisibility(VISIBLE);
                seekTo(mSkipPosition);
                mHandler.removeCallbacks(mHideSkipTipRunnable);
                _hideSkipTip();
                _setProgress();
            }
        };


        SkipTipView.SkipTipListener skipSubCallBack = new SkipTipView.SkipTipListener() {
            @Override
            public void onCancel() {
                mHandler.removeCallbacks(mHideSkipSubRunnable);
                _hideSkipSub();
            }

            @Override
            public void onSkip() {
                _hideSkipSub();
                mOutsideListener.onAction(Constants.INTENT_SELECT_SUBTITLE, 0);
            }
        };


        mFlVideoBox.setOnTouchListener(mPlayerTouchListener);
        mGestureDetector = new GestureDetector(mAttachActivity, mPlayerGestureListener);
        bottomBarView.setSeekCallBack(mSeekListener);
        mVideoView.setOnPreparedListener(ijkPreparedCallback);
        mVideoView.setOnErrorListener(ijkErrorCallback);
        mVideoView.setOnInfoListener(ijkPlayInfoCallback);
        mVideoView.setOnTimedTextListener(ijkPlayTimedTextListener);
        mVideoView.setOnTrackChangeListener(ijkPlayTrackChangeListener);

        topBarView.setCallBack(topBarCallBack);
        bottomBarView.setCallBack(bottomBarCallBack);
        skipTipView.setCallBack(skipTipCallBack);
        skipSubView.setCallBack(skipSubCallBack);


        if (mIvPlayerLock != null) {
            mIvPlayerLock.setOnClickListener(v -> _togglePlayerLock());
        }

        if (mIvScreenShot != null) {
            mIvScreenShot.setOnClickListener(v -> {
                if (!isUseSurfaceView) {
                    pause();
                    new DialogScreenShot(mAttachActivity, mVideoView.getScreenshot()).show();
                } else {
                    ToastUtils.showShort("screenshot not supported by current render");
                }
            });
        }


        bottomBarView.setSeekBarTouchCallBack((view, motionEvent) -> {
            if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                hideView(HIDE_VIEW_END_GESTURE);
            }
            return false;
        });

        mVideoView.setOnTouchListener((v, event) -> {
            if (isEditViewVisible()) {
                hideView(HIDE_VIEW_EDIT);
                return true;
            }
            return false;
        });
    }

    private void initViewAfter() {
        mVideoView.setIsUsingMediaCodec(isUsemediaCodeC);
        mVideoView.setPixelFormat(usePixelFormat);
        mVideoView.setIsUsingOpenSLES(isUseOpenSLES);
        mVideoView.setIsUsingSurfaceRenders(isUseSurfaceView);
        mVideoView.setIsUsingPlayerType(usePlayerType);

        if (isUseOpenSLES || usePlayerType == Constants.PLAYERENGINE__AndroidMediaPlayer) {
            topBarView.getPlayerSettingView().setSpeedCtrlLLVis(false);
        }

        bottomBarView.setSeekMax(MAX_VIDEO_SEEK);
        mHandler.post(mHideBarRunnable);
        mFlVideoBox.setClickable(true);
    }


    public void initSubtitleSettingView() {
        int subtitleTextSizeProgress = PlayerConfigShare.getInstance().getSubtitleTextSize();
        subtitleManager.setTextSizeProgress(subtitleTextSizeProgress);
        if (mSettings.getPlayerEngine() == CDEUtils.PV_PLAYERENGINE__FFmpeg) {
            topBarView.getSubtitleSettingView()
                    .initSubtitleTextSize(subtitleTextSizeProgress)
                    .initListener(new SettingSubtitleView.SettingSubtitleListener() {
                        @Override
                        public void selectTrack(TrackInfoBean trackInfoBean, boolean isAudio) {
                            IJKTrackInfoBean ijkTrackInfoBean = (IJKTrackInfoBean) trackInfoBean;
                            mVideoView.selectTrack(ijkTrackInfoBean.getStreamId());
                            mVideoView.seekTo(mVideoView.getCurrentPosition());
                        }

                        @Override
                        public void deselectTrack(TrackInfoBean trackInfoBean, boolean isAudio) {
                            IJKTrackInfoBean ijkTrackInfoBean = (IJKTrackInfoBean) trackInfoBean;
                            mVideoView.deselectTrack(ijkTrackInfoBean.getStreamId());
                        }

                        @Override
                        public void setSubtitleSwitch(Switch switchView, boolean isChecked) {
                            if (!topBarView.getSubtitleSettingView().isLoadSubtitle() && isChecked) {
                                switchView.setChecked(false);
                                Toast.makeText(getContext(), "subtitle was not loaded", Toast.LENGTH_LONG).show();
                                subtitleManager.hideExSub();
                                return;
                            }
                            if (isChecked) {
                                subtitleManager.showExSub();
                                mHandler.sendEmptyMessage(MSG_UPDATE_SUBTITLE);
                            } else {
                                subtitleManager.hideExSub();
                            }
                        }

                        @Override
                        public void setSubtitleTextSize(int progress) {
                            subtitleManager.setTextSizeProgress(progress);
                            PlayerConfigShare.getInstance().setSubtitleTextSize(progress);
                        }

                        @Override
                        public void setInterSubtitleSize(int progress) {

                        }

                        @Override
                        public void setInterBackground(CaptionStyleCompat compat) {

                        }

                        @Override
                        public void setOpenSubtitleSelector() {
                            pause();
                            hideView(HIDE_VIEW_ALL);
                            mOutsideListener.onAction(Constants.INTENT_OPEN_SUBTITLE, 0);
                        }

                        @Override
                        public void onShowNetworkSubtitle() {
                            hideView(HIDE_VIEW_ALL);
                            if (mOutsideListener != null)
                                mOutsideListener.onAction(Constants.INTENT_SELECT_SUBTITLE, 0);
                        }
                    })
                    .init();
        }


        if (mSettings.getPlayerEngine() == CDEUtils.PV_PLAYERENGINE__AndroidMediaPlayer) {
            CDELog.j(TAG, "subtitle not supported with AndroidMediaPlayer");
        }

    }

    private void initPlayerSettingView() {
        boolean allowOrientationChange = PlayerConfigShare.getInstance().isAllowOrientationChange();
        topBarView.getPlayerSettingView()
                .setSettingListener(new SettingPlayerView.SettingVideoListener() {

                    @Override
                    public void selectTrack(TrackInfoBean trackInfoBean, boolean isAudio) {
                        IJKTrackInfoBean ijkTrackInfoBean = (IJKTrackInfoBean) trackInfoBean;
                        mVideoView.selectTrack(ijkTrackInfoBean.getStreamId());
                        mVideoView.seekTo(mVideoView.getCurrentPosition());
                    }

                    @Override
                    public void deselectTrack(TrackInfoBean trackInfoBean, boolean isAudio) {
                        IJKTrackInfoBean ijkTrackInfoBean = (IJKTrackInfoBean) trackInfoBean;
                        mVideoView.deselectTrack(ijkTrackInfoBean.getStreamId());
                    }

                    @Override
                    public void setSpeed(float speed) {
                        mVideoView.setSpeed(speed);
                    }

                    @Override
                    public void setAspectRatio(int type) {
                        mVideoView.setAspectRatio(type);
                    }

                    @Override
                    public void setOrientationStatus(boolean isEnable) {
                        CDELog.j(TAG, "orientation status :" + isEnable);
                        PlayerConfigShare.getInstance().setAllowOrientationChange(isEnable);
                        if (mOrientationListener != null) {
                            if (isEnable)
                                mOrientationListener.enable();
                            else
                                mOrientationListener.disable();
                        }
                    }

                    @Override
                    public void setLandscapeStatus(boolean bLandScape) {
                        CDELog.j(TAG, "landscape :" + bLandScape);
                        //mVideoView.toggleLandscape();
                        mVideoView.setHudViewHolderVisibility(bLandScape ? View.VISIBLE : View.INVISIBLE);
                    }


                    @Override
                    public void setTVRecordingStatus(boolean bRecording) {
                        int recordCountsFromServer = CDEUtils.getRecordCountsFromServer();
                        boolean isPaidUser = CDEUtils.getIsPaidUser();
                        CDELog.j(TAG, "tv recording :" + bRecording);
                        CDELog.j(TAG, "current video name:" + mVideoName);
                        CDELog.j(TAG, "current video url:" + mVideoUrl);
                        CDELog.j(TAG, "recordCountsFromServer:" + recordCountsFromServer);
                        CDELog.j(TAG, "isPaidUser:" + isPaidUser);

                        //topBarView.updatePlaySettingVisibility(false);
                        topBarView.hideItemView();

                        if (!mVideoView.isPlaying()) {
                            Toast.makeText(getContext(), "record not supported currently, pls check \"Playback Setting", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVRecordingVisibility(false);
                            return;
                        }

                        if (mVideoName.startsWith("kantv-record")) {
                            Toast.makeText(getContext(), "record not supported because it seems a recorded file", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVRecordingVisibility(false);
                            return;
                        }

                        if (mVideoName.startsWith("kantv-asr")) {
                            Toast.makeText(getContext(), "record not supported because it seems a saved ASR file", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVRecordingVisibility(false);
                            return;
                        }

                        if (mVideoUrl.startsWith("/storage/emulated")) {
                            //Toast.makeText(getContext(), "record not supported because it seems a local media file", Toast.LENGTH_SHORT).show();
                            //topBarView.updateTVRecordingVisibility(false);
                            //return;
                        }

                        //TODO:kantv-record with Exoplayer engine is not finished currently,so disable it here
                        if ((CDEUtils.PV_PLAYERENGINE__FFmpeg != mSettings.getPlayerEngine()) || (mSettings.getUsingMediaCodec())
                        ) {
                            Toast.makeText(getContext(), "record not supported because playEngine is not FFmpeg or pls check FFmpeg configuration in \"Playback Setting", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVRecordingVisibility(false);
                            return;
                        }

                        if ((KANTVDRM.getInstance().ANDROID_JNI_GetVideoWidth() == 0) || (KANTVDRM.getInstance().ANDROID_JNI_GetVideoHeight() == 0)) {
                            Toast.makeText(getContext(), "record not supported because it seems a pure audio content", Toast.LENGTH_LONG).show();
                            topBarView.updateTVRecordingVisibility(false);
                            return;
                        }

                        if ((KANTVDRM.getInstance().ANDROID_JNI_GetVideoWidth() > 1920) || (KANTVDRM.getInstance().ANDROID_JNI_GetVideoHeight() > 1080)) {
                            //Toast.makeText(getContext(), "A/V sync issue might be occurred during recording because video resolution is above 1920x1080 or powerful phone is required", Toast.LENGTH_SHORT).show();
                            //topBarView.updateTVRecordingVisibility(false);
                            //return;
                        }

                        {
                            KANTVDRM instance = KANTVDRM.getInstance();
                            File sdcardFile = Environment.getExternalStorageDirectory();
                            String sdcardPath = sdcardFile.getAbsolutePath();
                            CDELog.j(TAG, "sdcard path:" + sdcardPath);
                            int diskFreeSize = instance.ANDROID_JNI_GetDiskFreeSize(sdcardPath);
                            int diskTotalSize = instance.ANDROID_JNI_GetDiskSize(sdcardPath);
                            CDELog.j(TAG, "disk free:" + diskFreeSize + "MB");
                            CDELog.j(TAG, "disk total:" + diskTotalSize + "MB");
                            if (diskFreeSize < CDEUtils.getDiskThresholdFreeSize()) {
                                Toast.makeText(getContext(), "record not supported because lack of storage:" + " total space: " + diskTotalSize
                                        + " M， available space: " + diskFreeSize + " M ", Toast.LENGTH_SHORT).show();
                            } else {
                                mVideoView.setEnableRecord(bRecording);
                            }
                        }
                    }


                    @Override
                    public void setTVASRStatus(boolean bEnableASR) {
                        CDELog.j(TAG, "enable ASR:" + bEnableASR);
                        topBarView.hideItemView();

                        if (mVideoName.startsWith("kantv-asr")) {
                            Toast.makeText(getContext(), "ASR not supported because it seems a saved ASR file", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVASRVisibility(false);
                            return;
                        }

                        if ((CDEUtils.PV_PLAYERENGINE__FFmpeg != mSettings.getPlayerEngine())
                        ) {
                            Toast.makeText(getContext(), "ASR not supported because playEngine is not FFmpeg", Toast.LENGTH_SHORT).show();
                            topBarView.updateTVASRVisibility(false);
                            return;
                        }

                        mVideoView.setEnableASR(bEnableASR);
                    }
                })
                .setOrientationAllow(allowOrientationChange);
    }


    @Override
    public void onResume() {
        if (mCurPosition != INVALID_VALUE) {
            seekTo(mCurPosition);
            mCurPosition = INVALID_VALUE;
        }
    }


    @Override
    public void onPause() {
        CDELog.j(TAG, "onPause");
        mCurPosition = mVideoView.getCurrentPosition();

        if ((mVideoView != null) && (mVideoView.isBackgroundPlayEnabled())) {

        } else {
            CDELog.j(TAG, "detect pause, exit playback");
            onBackPressed();
        }
    }


    @Override
    public void onDestroy() {
        CDELog.j(TAG, "onDestroy");
        mVideoView.destroy();

        mAttachActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }


    @Override
    public boolean handleVolumeKey(int keyCode) {
        if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            _setVolume(true);
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            _setVolume(false);
            return true;
        } else {
            return false;
        }
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
        CDELog.j(TAG, "keyCode:" + keyCode);
        if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            _setVolume(true);
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            _setVolume(false);
            return true;
        } else {
            return super.onKeyDown(keyCode, event);
        }
    }

    @Override
    public void configurationChanged(Configuration newConfig) {
        _refreshOrientationEnable();
        hideView(HIDE_VIEW_ALL);
    }


    @Override
    public boolean onBackPressed() {
        CDELog.j(TAG, "onBackPressed");
        mBackPressed = true;
        if (isEditViewVisible()) {
            hideView(HIDE_VIEW_EDIT);
            return true;
        }

        stopRecord();
        stopASR();

        CDELog.j(TAG, "isBackgroundEnabled:" + mVideoView.isBackgroundPlayEnabled());
        if (!mVideoView.isBackgroundPlayEnabled()) {
            mVideoView.stopPlayback();
            mVideoView.release(true);
            mVideoView.stopBackgroundPlay();
            mAttachActivity.finish();
        } else {
            hideView(HIDE_VIEW_ALL);
            mVideoView.enterBackground();
        }
        return true;
    }


    @Override
    public void setBatteryChanged(int status, int progress) {
        if (topBarView != null) {
            topBarView.setBatteryChanged(status, progress);
        }
    }


    @Override
    public void setSubtitlePath(String subtitlePath) {
        if (TextUtils.isEmpty(subtitlePath))
            return;
        topBarView.getSubtitleSettingView().setLoadSubtitle(false);
        topBarView.getSubtitleSettingView().setSubtitleLoadStatus(false);
        new Thread(() -> {
            TimedTextObject subtitleObj = new SubtitleParser(subtitlePath).parser();
            if (subtitleObj != null) {
                Message message = new Message();
                message.what = MSG_SET_SUBTITLE_SOURCE;
                message.obj = subtitleObj;
                mHandler.sendMessage(message);
            }
        }).start();
    }


    @Override
    public void onSubtitleQuery(int size) {
        topBarView.getSubtitleSettingView().setNetworkSubtitleVisible(true);
        if (isAutoLoadNetworkSubtitle)
            mOutsideListener.onAction(Constants.INTENT_AUTO_SUBTITLE, 0);
        else
            _showSkipSub(size);
    }

    @Override
    public void onScreenLocked() {

    }


    public FFPlayerView setSubtitleFolder(String subtitleFolder) {
        this.subtitleDownloadFolder = subtitleFolder;
        return this;
    }


    public FFPlayerView setVideoPath(String videoPath) {
        mVideoUrl = videoPath;
        loadDefaultSubtitle(videoPath);
        mVideoView.setVideoURI(Uri.parse(videoPath));
        CDELog.j(TAG, "video url:" + videoPath);
        seekTo(0);
        return this;
    }


    public FFPlayerView setTitle(String title) {
        if (topBarView != null)
            topBarView.setTitleText(title);

        mVideoName = title;
        mVideoView.setVideoTitle(title);
        CDELog.j(TAG, "video name:" + title);
        return this;
    }


    public FFPlayerView setOnInfoListener(IMediaPlayer.OnOutsideListener listener) {
        mOutsideListener = listener;
        return this;
    }


    public FFPlayerView setSkipTip(long targetPositionMs) {
        if (targetPositionMs > 0) {
            mSkipPosition = targetPositionMs;
        }
        return this;
    }


    public FFPlayerView setNetworkSubtitle(boolean isOpen) {
        isQueryNetworkSubtitle = isOpen;
        return this;
    }


    public FFPlayerView setAutoLoadLocalSubtitle(boolean isAuto) {
        isAutoLoadLocalSubtitle = isAuto;
        return this;
    }


    public FFPlayerView setAutoLoadNetworkSubtitle(boolean isAuto) {
        isAutoLoadNetworkSubtitle = isAuto;
        return this;
    }


    public void start() {
        CDELog.d(TAG, "start");
        CDELog.d(TAG, "manually start play position:" + CDEUtils.getStartPlayPos() + " minutes");
        CDELog.d(TAG, "last play position: " + mPlayPos + " secs");
        if (CDEUtils.getStartPlayPos() != 0) {
            mPlayPos = CDEUtils.getStartPlayPos() * 60;
        }
        if (mPlayPos != 0) {
            mVideoView.seekTo(((int) mPlayPos) * 1000);
        }

        if (mIsNeverPlay) {
            mIsNeverPlay = false;
            mLoadingView.setVisibility(VISIBLE);
            mIsShowBar = false;


            if (isQueryNetworkSubtitle && mOutsideListener != null) {
                mOutsideListener.onAction(Constants.INTENT_QUERY_SUBTITLE, 0);
            }
        }

        bottomBarView.setPlayIvStatus(true);

        if (!mVideoView.isPlaying()) {
            mVideoView.start();

            mHandler.sendEmptyMessage(MSG_UPDATE_SEEK);
        }


        if (mIsPlayComplete) {
            mIsPlayComplete = false;
        }
        if (topBarView.getSubtitleSettingView().isLoadSubtitle())
            subtitleManager.showExSub();
    }


    public void pause() {
        CDELog.d(TAG, "pause");
        bottomBarView.setPlayIvStatus(false);
        if (mVideoView.isPlaying()) {
            stopRecord();
            stopASR();
            mVideoView.pause();
        }
    }


    public void stop() {
        CDELog.j(TAG, "enter stop");
        pause();
        if (mOutsideListener != null) {
            mOutsideListener.onAction(Constants.INTENT_SAVE_CURRENT, mVideoView.getCurrentPosition());
            mOutsideListener.onAction(Constants.INTENT_PLAY_END, 0);
        }
        mVideoView.stopPlayback();
        CDELog.j(TAG, "leave stop");
    }


    public void seekTo(long position) {
        mVideoView.seekTo(Long.valueOf(position).intValue());
    }


    private void _setControlBarVisible(boolean isShowBar) {
        if (mIsForbidTouch) {
            hideShowLockScreen(isShowBar);
        } else {
            if (isShowBar) {
                topBarView.updateSystemTime();
                hideShowBar(true);
                hideShowLockScreen(true);
            } else {
                hideView(HIDE_VIEW_AUTO);
            }
        }
    }


    private void _toggleControlBar() {
        mIsShowBar = !mIsShowBar;
        _setControlBarVisible(mIsShowBar);
        if (mIsShowBar) {

            mHandler.postDelayed(mHideBarRunnable, DEFAULT_HIDE_TIMEOUT);
            mHandler.sendEmptyMessage(MSG_UPDATE_SEEK);
        }
    }


    private void _showControlBar(int timeout) {
        if (!mIsShowBar) {
            _setProgress();
            mIsShowBar = true;
        }
        _setControlBarVisible(true);
        mHandler.sendEmptyMessage(MSG_UPDATE_SEEK);
        mHandler.removeCallbacks(mHideBarRunnable);
        if (timeout != 0) {
            mHandler.postDelayed(mHideBarRunnable, timeout);
        }
    }


    private void _togglePlayStatus() {
        if (mVideoView.isPlaying()) {
            pause();
        } else {
            start();
        }
    }


    private void _refreshHideRunnable() {
        mHandler.removeCallbacks(mHideBarRunnable);
        mHandler.postDelayed(mHideBarRunnable, DEFAULT_HIDE_TIMEOUT);
    }


    private void _togglePlayerLock() {
        mIsForbidTouch = !mIsForbidTouch;
        if (mIvPlayerLock != null) {
            mIvPlayerLock.setSelected(mIsForbidTouch);
        }
        if (mIsForbidTouch) {
            setOrientationEnable(false);
            hideView(HIDE_VIEW_LOCK_SCREEN);
        } else {
            if (topBarView.getPlayerSettingView().isAllowScreenOrientation()) {
                setOrientationEnable(true);
            }
            hideShowBar(true);
        }
    }


    private void _refreshOrientationEnable() {
        if (topBarView.getPlayerSettingView().isAllowScreenOrientation()) {
            setOrientationEnable(false);
            mHandler.removeMessages(MSG_ENABLE_ORIENTATION);
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_ORIENTATION, 3000);
        }
    }


    private long _setProgress() {
        if (mVideoView == null || mIsSeeking) {
            return 0;
        }

        long position = mVideoView.getCurrentPosition();
        long duration = mVideoView.getDuration();
        if (duration > 0) {
            long pos = (long) MAX_VIDEO_SEEK * position / duration;
            bottomBarView.setSeekProgress((int) pos);
        }
        int percent = mVideoView.getBufferPercentage();
        bottomBarView.setSeekSecondaryProgress(percent * 10);
        bottomBarView.setEndTime(TimeFormatUtils.generateTime(duration));
        bottomBarView.setCurTime(TimeFormatUtils.generateTime(position));
        return position;
    }


    private void setSkipTimeText(String time) {
        if (mFlTouchLayout.getVisibility() == View.GONE) {
            mFlTouchLayout.setVisibility(View.VISIBLE);
        }
        if (mSkipTimeTv.getVisibility() == View.GONE) {
            mSkipTimeTv.setVisibility(View.VISIBLE);
        }
        mSkipTimeTv.setText(time);
    }


    private void _onProgressSlide(float percent) {
        long position = mVideoView.getCurrentPosition();
        long duration = mVideoView.getDuration();
        long deltaMax = Math.min(100 * 1000, duration / 2);
        long delta = (long) (deltaMax * percent);
        mTargetPosition = delta + position;
        if (mTargetPosition > duration) {
            mTargetPosition = duration;
        } else if (mTargetPosition <= 0) {
            mTargetPosition = 0;
        }
        int deltaTime = (int) ((mTargetPosition - position) / 1000);
        String desc;
        if (mTargetPosition > position) {
            desc = TimeFormatUtils.generateTime(mTargetPosition) + "/" + TimeFormatUtils.generateTime(duration) + "\n" + "+" + deltaTime + "秒";
        } else {
            desc = TimeFormatUtils.generateTime(mTargetPosition) + "/" + TimeFormatUtils.generateTime(duration) + "\n" + deltaTime + "秒";
        }
        setSkipTimeText(desc);
    }


    @SuppressLint("SetTextI18n")
    private void _setVolumeInfo(int volume) {
        if (mFlTouchLayout.getVisibility() == View.GONE) {
            mFlTouchLayout.setVisibility(View.VISIBLE);
        }
        if (mTvVolume.getVisibility() == View.GONE) {
            mTvVolume.setVisibility(View.VISIBLE);
        }
        mTvVolume.setText((volume * 100 / mMaxVolume) + "%");
    }


    private void _onVolumeSlide(float percent) {
        if (mCurVolume == INVALID_VALUE) {
            mCurVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            if (mCurVolume < 0) {
                mCurVolume = 0;
            }
        }
        int index = (int) (percent * mMaxVolume) + mCurVolume;
        if (index > mMaxVolume) {
            index = mMaxVolume;
        } else if (index < 0) {
            index = 0;
        }
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, index, 0);
        _setVolumeInfo(index);
    }


    private void _setVolume(boolean isIncrease) {
        int curVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        if (isIncrease) {
            curVolume += mMaxVolume / 15;
        } else {
            curVolume -= mMaxVolume / 15;
        }
        if (curVolume > mMaxVolume) {
            curVolume = mMaxVolume;
        } else if (curVolume < 0) {
            curVolume = 0;
        }
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, curVolume, 0);
        _setVolumeInfo(curVolume);
        mHandler.removeCallbacks(mHideTouchViewRunnable);
        mHandler.postDelayed(mHideTouchViewRunnable, 1000);
    }


    @SuppressLint("SetTextI18n")
    private void _setBrightnessInfo(float brightness) {
        if (mFlTouchLayout.getVisibility() == View.GONE) {
            mFlTouchLayout.setVisibility(View.VISIBLE);
        }
        if (mTvBrightness.getVisibility() == View.GONE) {
            mTvBrightness.setVisibility(View.VISIBLE);
        }
        mTvBrightness.setText(Math.ceil(brightness * 100) + "%");
    }


    private void _onBrightnessSlide(float percent) {
        if (mCurBrightness < 0) {
            mCurBrightness = mAttachActivity.getWindow().getAttributes().screenBrightness;
            if (mCurBrightness < 0.0f) {
                mCurBrightness = 0.1f;
            } else if (mCurBrightness < 0.01f) {
                mCurBrightness = 0.01f;
            }
        }
        WindowManager.LayoutParams attributes = mAttachActivity.getWindow().getAttributes();
        attributes.screenBrightness = mCurBrightness + percent;
        if (attributes.screenBrightness > 1.0f) {
            attributes.screenBrightness = 1.0f;
        } else if (attributes.screenBrightness < 0.01f) {
            attributes.screenBrightness = 0.01f;
        }
        _setBrightnessInfo(attributes.screenBrightness);
        mAttachActivity.getWindow().setAttributes(attributes);
    }


    private void _endGesture() {
        if (mTargetPosition >= 0 && mTargetPosition != mVideoView.getCurrentPosition() && mVideoView.getDuration() != 0) {
            seekTo((int) mTargetPosition);
            bottomBarView.setSeekProgress((int) (mTargetPosition * MAX_VIDEO_SEEK / mVideoView.getDuration()));
            mTargetPosition = INVALID_VALUE;
        }
        hideView(HIDE_VIEW_END_GESTURE);
        _refreshHideRunnable();
        mCurVolume = INVALID_VALUE;
        mCurBrightness = INVALID_VALUE;
    }

    private void onRecordStart() {
        CDELog.j(TAG, "recording start");
        if ((CDEUtils.PV_PLAYERENGINE__FFmpeg != mSettings.getPlayerEngine()) || (mSettings.getUsingMediaCodec())
        ) {
            Toast.makeText(getContext(), "pls check playback setting", Toast.LENGTH_SHORT).show();
            topBarView.updateTVRecordingVisibility(false);
        } else {
            CDEUtils.setTVRecording(true);
            CDEUtils.perfGetRecordBeginTime();
            topBarView.updateTVRecordingVisibility(true);
            CDELog.j(TAG, "recording filename:" + CDEUtils.getRecordingFileName());

            int videoWidth = KANTVDRM.getInstance().ANDROID_JNI_GetVideoWidth();
            int videoHeight = KANTVDRM.getInstance().ANDROID_JNI_GetVideoHeight();
            int recordMode = mSettings.getRecordMode();
            int recordFormat = mSettings.getRecordFormat();
            int recordCodec = mSettings.getRecordCodec();
            String tipInfo = "TV Recording will be launched.\n";
            if (0 == recordMode) {
                tipInfo = tipInfo + "video resolution:" + videoWidth + "x" + videoHeight + "\n"
                        + "video recording configuration:\n"
                        + "record mode:" + CDEUtils.getRecordModeString(recordMode)
                        + ",file format:" + CDEUtils.getRecordFormatString(recordFormat)
                        + ",video codec:" + CDEUtils.getVideoCodecString(recordCodec)
                        + ",audio codec:" + "aac"
                        + ",max record file size: " + mSettings.getRecordSizeString() + " M bytes"
                        + ",max record duration: " + mSettings.getRecordDurationString() + " minutes";
            } else if (1 == recordMode) {
                tipInfo = tipInfo + "video resolution:" + videoWidth + "x" + videoHeight + "\n"
                        + "video recording configuration:\n"
                        + "record mode:" + CDEUtils.getRecordModeString(recordMode)
                        + ",file format:" + CDEUtils.getRecordFormatString(recordFormat)
                        + ",video codec:" + CDEUtils.getVideoCodecString(recordCodec)
                        + ",max record file size: " + mSettings.getRecordSizeString() + " M bytes"
                        + ",max record duration: " + mSettings.getRecordDurationString() + " minutes";
            } else { // audio only
                tipInfo = tipInfo + "video resolution:" + videoWidth + "x" + videoHeight + "\n"
                        + "video recording configuration:\n"
                        + "record mode:" + CDEUtils.getRecordModeString(recordMode)
                        + ",file format:" + CDEUtils.getRecordFormatString(recordFormat)
                        + ",audio codec:" + "aac"
                        + ",max record file size: " + mSettings.getRecordSizeString() + " M bytes"
                        + ",max record duration: " + mSettings.getRecordDurationString() + " minutes";
            }
            CDELog.j(TAG, "record mode:" + recordMode);
            if (videoWidth > 1920 || videoHeight > 1080 || recordCodec != 0) {
                if ((0 == recordMode) || (1 == recordMode)) {
                    tipInfo += "\n\n A/V sync issue or other issue might be occurred during recording because video resolution is above 1920x1080 or video codec is not H264 or powerful phone is required";
                }
            }

            showWarningDialog(mAttachActivity, tipInfo);
            CDEUtils.umStartRecord(CDEUtils.getRecordingFileName());
        }
    }

    private void onRecordStop() {
        CDELog.j(TAG, "recording stop");
        if (!CDEUtils.getTVRecording()) {
            CDELog.j(TAG, "record not started, why called again?");
            return;
        }
        CDEUtils.setTVRecording(false);
        topBarView.updateTVRecordingVisibility(false);
        CDELog.j(TAG, "recording file:" + CDEUtils.getRecordingFileName());
        File file = new File(CDEUtils.getRecordingFileName());
        if (file.exists()) {
            CDELog.j(TAG, "recording file " + CDEUtils.getRecordingFileName() + ", size:" + file.length() + " bytes");
            Toast.makeText(getContext(), "recording stopped，size of recorded file: " + CDEUtils.formattedSize(file.length()), Toast.LENGTH_LONG).show();
            CDEUtils.umStopRecord(CDEUtils.getRecordingFileName());
        } else {
            CDELog.j(TAG, "it shouldn't happen");
            Toast.makeText(getContext(), "recording stopped", Toast.LENGTH_SHORT).show();
            CDEUtils.umStopRecord("unknown");
        }


        Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        Uri uri = Uri.fromFile(new File(CDEUtils.getDataPath()));
        intent.setData(uri);
        if (getContext() != null)
            getContext().sendBroadcast(intent);
    }

    private void stopRecord() {
        if (mVideoView.getEnableRecord()) {
            CDELog.j(TAG, "stop record");
            KANTVDRM.getInstance().ANDROID_JNI_StopRecord();
            onRecordStop();
            mVideoView.setEnableRecord(false);
        }
    }

    private void stopASR() {
        if (mVideoView.getEnableASR()) {
            CDELog.j(TAG, "stop ASR");
            mVideoView.setEnableASR(false);
            KANTVDRM.getInstance().ANDROID_JNI_StopASR();
        }
    }

    //this is thread safety operation
    private void onASRStart(int asrMode) {
        CDELog.j(TAG, "ASR start");
        if (CDEUtils.PV_PLAYERENGINE__FFmpeg != mSettings.getPlayerEngine()) {
            Toast.makeText(getContext(), "ASR only supported with FFmpeg engine currently", Toast.LENGTH_SHORT).show();
            topBarView.updateTVASRVisibility(false);
            CDEUtils.setTVASR(false);
            return;
        }

        if (asrMode == CDEUtils.ASR_MODE_TRANSCRIPTION_RECORD) {
            CDELog.j(TAG, "asr saved filename:" + CDEUtils.getASRSavedFileName());
        }

        //String ggmlModelFileName = "ggml-small.en.bin";       // 466M
        //String ggmlModelFileName = "ggml-tiny-q5_1.bin";      // 31M
        //String ggmlModelFileName = "ggml-tiny.en-q5_1.bin";   // 31M
        String ggmlModelFileName = "ggml-tiny.en-q8_0.bin";     // 42M, very good, about 500-700 ms
        String userChooseModelName = CDEUtils.getGGMLModeString(mSettings.getGGMLMode());
        String userChooseModelFileName = "ggml-" + userChooseModelName + ".bin";
        CDELog.j(TAG, "ggml model name of user's choose:" + userChooseModelFileName);

        if (!ggmlModelFileName.equals(userChooseModelFileName)) {
            CDELog.j(TAG, "pls choose GGML model :" + ggmlModelFileName);
            Toast.makeText(getContext(), "pls choose GGML model: " + ggmlModelFileName, Toast.LENGTH_SHORT).show();
            topBarView.updateTVASRVisibility(false);
            CDEUtils.setTVASR(false);
            CDEUtils.showMsgBox(mAttachActivity, "pls choose GGML model: " + ggmlModelFileName);
            return;
        }

        ggmlModelFileName = userChooseModelFileName;
        CDELog.j(TAG, "model: " + ggmlModelFileName);

        File file = new File(CDEUtils.getDataPath() + ggmlModelFileName);
        if (!file.exists()) {
            CDELog.j(TAG, "GGML model file not found:" + file.getAbsolutePath());
            Toast.makeText(getContext(), "GGML model file not found:" + file.getAbsolutePath(), Toast.LENGTH_SHORT).show();
            topBarView.updateTVASRVisibility(false);
            CDEUtils.setTVASR(false);
            CDEUtils.showMsgBox(mAttachActivity, "GGML model file not found:" + file.getAbsolutePath());
            return;
        } else {
            CDELog.j(TAG, "ASR with GGML model file:" + file.getAbsolutePath());
        }

        if (CDEUtils.getASRSubsystemInit()) {
            if ((CDEUtils.ASR_MODE_NORMAL == mSettings.getASRMode()) || (CDEUtils.ASR_MODE_TRANSCRIPTION_RECORD == mSettings.getASRMode())) {
                ggmljava.asr_reset(CDEUtils.getDataPath() + "/models/" + ggmlModelFileName, mSettings.getASRThreadCounts(), CDEUtils.ASR_MODE_NORMAL, CDEUtils.HEXAGON_BACKEND_GGML);
            } else {
                ggmljava.asr_reset(CDEUtils.getDataPath() + "/models/" + ggmlModelFileName, mSettings.getASRThreadCounts(), CDEUtils.ASR_MODE_PRESURETEST, CDEUtils.HEXAGON_BACKEND_GGML);
            }
            ggmljava.asr_start();
        } else {
            topBarView.updateTVASRVisibility(false);
            CDEUtils.setTVASR(false);
            CDEUtils.showMsgBox(mAttachActivity, "ASR subsystem not initialized, pls check why");
            return;
        }

        CDEUtils.setTVASR(true);
        topBarView.updateTVASRVisibility(true);

        String tipInfo = "TV transcription will be launched.\n";
        if (asrMode == CDEUtils.ASR_MODE_TRANSCRIPTION_RECORD) {
            tipInfo += "raw audio data would be saved to file for further usage:" + CDEUtils.getASRSavedFileName();
        }
        showWarningDialog(mAttachActivity, tipInfo);
    }


    //this is thread safety operation
    private void onASRStop(int asrMode) {
        CDELog.j(TAG, "ASR stop");
        if (!CDEUtils.getTVASR()) {
            CDELog.j(TAG, "TV transcription not started, why called?");
            return;
        }

        if (subtitleManager != null) {
            subtitleManager.setInnerSub("");
        }

        ggmljava.asr_stop();
        //ggmljava.asr_finalize();
        CDEUtils.setTVASR(false);
        topBarView.updateTVASRVisibility(false);
        if (asrMode == CDEUtils.ASR_MODE_TRANSCRIPTION_RECORD) {
            CDELog.j(TAG, "saved raw audio data file:" + CDEUtils.getASRSavedFileName());
            File file = new File(CDEUtils.getASRSavedFileName());
            if (file.exists()) {
                CDELog.j(TAG, "saved raw audio data file " + CDEUtils.getASRSavedFileName() + ", size:" + file.length() + " bytes");
                Toast.makeText(getContext(), "TV transcription stopped，size of saved raw audio data file: "
                        + CDEUtils.formattedSize(file.length()), Toast.LENGTH_LONG).show();
            } else {
                CDELog.j(TAG, "it shouldn't happen");
                Toast.makeText(getContext(), "TV transcription stopped", Toast.LENGTH_SHORT).show();
            }


            //TODO:add thumbnail to saved raw audio data file
            Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
            Uri uri = Uri.fromFile(new File(CDEUtils.getDataPath()));
            intent.setData(uri);
            if (getContext() != null)
                getContext().sendBroadcast(intent);
        } else {
            CDELog.j(TAG, "TV transcription stopped");
            Toast.makeText(getContext(), "TV transcription stopped", Toast.LENGTH_SHORT).show();
        }

    }


    private void _switchStatus(int status) {
        CDELog.d(TAG, "status:" + status);
        switch (status) {
            case 5:
                CDELog.j(TAG, "receive playback completed");
                stopRecord();
                stopASR();
                break;
            case 1983:
                CDELog.j(TAG, "recording start");
                CDELog.j(TAG, "recording filename:" + CDEUtils.getRecordingFileName());
                break;
            case 1984:
                CDELog.j(TAG, "recording stop");
            {
                File file = new File(CDEUtils.getRecordingFileName());
                if (file.exists()) {
                    CDELog.j(TAG, "recording file size:" + file.length());
                    //Toast.makeText(getContext(), "recording finished，size of recorded file:" + CDEUtils.formattedSize(file.length()), Toast.LENGTH_LONG).show();
                } else {
                    CDELog.j(TAG, "it shouldn't happen, pls check why?");
                    //Toast.makeText(getContext(), "recording finished", Toast.LENGTH_SHORT).show();
                }
            }
            break;
            case IMediaPlayer.MEDIA_INFO_RECORDING_START:
                onRecordStart();
                break;
            case IMediaPlayer.MEDIA_INFO_RECORDING_STOP:
                onRecordStop();
                break;
            case IMediaPlayer.MEDIA_INFO_ASR_START:
                onASRStart(mSettings.getASRMode());
                break;
            case IMediaPlayer.MEDIA_INFO_ASR_STOP:
                onASRStop(mSettings.getASRMode());
                break;

            case IMediaPlayer.MEDIA_INFO_ASR_START_NORMAL:
                onASRStart(mSettings.getASRMode());
                break;
            case IMediaPlayer.MEDIA_INFO_ASR_STOP_NORMAL:
                onASRStop(mSettings.getASRMode());
                break;

            case IMediaPlayer.MEDIA_INFO_BUFFERING_START:
                mOutsideListener.onAction(Constants.INTENT_PLAY_COMPLETE, 0);
                mIsIjkPlayerReady = false;
                if (!mIsNeverPlay) {
                    mLoadingView.setVisibility(View.VISIBLE);
                }
            case MediaPlayerParams.STATE_PREPARING:
                break;
            case IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START:
                mIsRenderingStart = true;
            case IMediaPlayer.MEDIA_INFO_BUFFERING_END:
                mIsIjkPlayerReady = true;
                mLoadingView.setVisibility(View.GONE);
                mHandler.sendEmptyMessage(MSG_UPDATE_SEEK);
                if (mSkipPosition != INVALID_VALUE) {
                    _showSkipTip();
                }
                break;
            case MediaPlayerParams.STATE_PLAYING:
                break;
            case MediaPlayerParams.STATE_ERROR:
                break;
            case MediaPlayerParams.STATE_COMPLETED:
                pause();
                mIsPlayComplete = true;
                mOutsideListener.onAction(Constants.INTENT_PLAY_COMPLETE, 1);
                break;
            default:
                break;
        }
    }


    private void _showSkipTip() {
        if (mSkipPosition != INVALID_VALUE && skipTipView.getVisibility() == GONE) {
            skipTipView.setVisibility(VISIBLE);
            skipTipView.setSkipTime(TimeFormatUtils.generateTime(mSkipPosition));
            AnimHelper.doSlide(skipTipView, mWidthPixels, 0, 800);
            mHandler.postDelayed(mHideSkipTipRunnable, DEFAULT_HIDE_TIMEOUT * 3);
        }
    }


    private void _hideSkipTip() {
        if (skipTipView.getVisibility() == GONE) {
            return;
        }
        ViewCompat.animate(skipTipView).translationX(-skipTipView.getWidth()).alpha(0).setDuration(500)
                .setListener(new ViewPropertyAnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(View view) {
                        skipTipView.setVisibility(GONE);
                    }
                }).start();
        mSkipPosition = INVALID_VALUE;
    }


    private void _showSkipSub(int size) {
        if (skipSubView.getVisibility() == GONE) {
            skipSubView.setVisibility(VISIBLE);
            skipSubView.setSkipContent(size);
            AnimHelper.doSlide(skipSubView, mWidthPixels, 0, 800);
            mHandler.postDelayed(mHideSkipSubRunnable, DEFAULT_HIDE_TIMEOUT * 3);
        }
    }


    private void _hideSkipSub() {
        if (skipSubView.getVisibility() == GONE) {
            return;
        }
        ViewCompat.animate(skipSubView).translationX(-skipSubView.getWidth()).alpha(0).setDuration(500)
                .setListener(new ViewPropertyAnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(View view) {
                        skipSubView.setVisibility(GONE);
                    }
                }).start();
    }


    private void hideView(int hideType) {
        if (hideType == HIDE_VIEW_END_GESTURE) {
            if (mFlTouchLayout.getVisibility() == VISIBLE) {
                mFlTouchLayout.setVisibility(GONE);
                mSkipTimeTv.setVisibility(View.GONE);
                mTvVolume.setVisibility(View.GONE);
                mTvBrightness.setVisibility(View.GONE);
            }
            return;
        }


        if (topBarView.getTopBarVisibility() == VISIBLE && hideType != HIDE_VIEW_EDIT) {
            hideShowBar(false);
        }

        if (mIvPlayerLock != null) {
            if (mIvPlayerLock.getVisibility() == VISIBLE && hideType != HIDE_VIEW_LOCK_SCREEN && hideType != HIDE_VIEW_EDIT) {
                hideShowLockScreen(false);
            }
        }

        if (topBarView.isItemShowing() && hideType != HIDE_VIEW_AUTO) {
            topBarView.hideItemView();
        }
    }


    private boolean isEditViewVisible() {
        return topBarView.isItemShowing();
    }


    private void hideShowBar(boolean isShow) {
        if (isShow) {
            AnimHelper.viewTranslationY(bottomBarView, 0);
            bottomBarView.setVisibility(View.VISIBLE);
            topBarView.setTopBarVisibility(true);
            mIsShowBar = true;
        } else {
            AnimHelper.viewTranslationY(bottomBarView, bottomBarView.getHeight());
            topBarView.setTopBarVisibility(false);
            mIsShowBar = false;
        }

        if (mIvScreenShot != null) {
            if (isShow) {
                AnimHelper.viewTranslationX(mIvScreenShot, 0, 300);
            } else {
                AnimHelper.viewTranslationX(mIvScreenShot, ConvertUtils.dp2px(60), 300);
            }
        }

        if (mVideoView.isExoPlayer()) {
            topBarView.updateTVRecordingVisibility(CDEUtils.getTVRecording());
        }
    }


    private void hideShowLockScreen(boolean isShow) {
        if (mIvPlayerLock != null) {
            if (isShow) {
                AnimHelper.viewTranslationX(mIvPlayerLock, 0, 300);
            } else {
                AnimHelper.viewTranslationX(mIvPlayerLock, -ConvertUtils.dp2px(60), 300);
            }
        }
    }


    public void loadDefaultSubtitle(String videoPath) {
        if (!isAutoLoadLocalSubtitle) return;
        String subtitlePath = CommonPlayerUtils.getSubtitlePath(videoPath, subtitleDownloadFolder);
        if (!StringUtils.isEmpty(subtitlePath)) {

            isAutoLoadNetworkSubtitle = false;
            setSubtitlePath(subtitlePath);
        }
    }


    private void setOrientationEnable(boolean isEnable) {
        if (topBarView.getPlayerSettingView() != null)
            topBarView.getPlayerSettingView().setOrientationChangeEnable(isEnable);
    }

    private void showWarningDialog(Context context, String warningInfo) {
        new AlertDialog.Builder(context)
                .setTitle("Tip")
                .setMessage(warningInfo)
                .setCancelable(true)
                .setNegativeButton(context.getString(R.string.OK),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.dismiss();
                            }
                        })
                .show();
    }


    private void setFullScreen() {
        View decorView = mAttachActivity.getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                        View.SYSTEM_UI_FLAG_FULLSCREEN |
                        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );
        mAttachActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        mAttachActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        WindowManager.LayoutParams attributes = mAttachActivity.getWindow().getAttributes();
        attributes.screenBrightness = 1.0f;
        mAttachActivity.getWindow().setAttributes(attributes);

        mAttachActivity.requestWindowFeature(Window.FEATURE_NO_TITLE);
    }

    public static native int kantv_anti_remove_rename_this_file();
}
