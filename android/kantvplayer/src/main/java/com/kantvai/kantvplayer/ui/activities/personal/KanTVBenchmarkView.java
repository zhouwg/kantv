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
package com.kantvai.kantvplayer.ui.activities.personal;

import static kantvai.media.player.KANTVBenchmarkType.AV_PIX_FMT_YUV420P;
import static kantvai.media.player.KANTVEvent.KANTV_INFO_PREVIEW;

import android.animation.ValueAnimator;
import android.annotation.TargetApi;
import android.app.AlertDialog;
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
import android.view.Surface;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.MediaController;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.player.ffplayer.media.IMediaController;
import com.kantvai.kantvplayer.player.ffplayer.media.IRenderView;
import com.kantvai.kantvplayer.player.ffplayer.media.SurfaceRenderView;
import com.kantvai.kantvplayer.player.ffplayer.media.TableLayoutBinder;
import com.kantvai.kantvplayer.player.ffplayer.media.TextureRenderView;
import com.kantvai.kantvplayer.utils.Settings;

import java.util.Locale;


import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;
import kantvai.media.player.KANTVEvent;
import kantvai.media.player.KANTVEventListener;
import kantvai.media.player.KANTVEventType;
import kantvai.media.player.KANTVException;
import kantvai.media.player.KANTVMgr;

import android.content.DialogInterface;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;


public class KanTVBenchmarkView extends FrameLayout implements MediaController.MediaPlayerControl {
    private static final String TAG = KanTVBenchmarkView.class.getName();

    private int SURFACE_WIDTH = 640;
    private int SURFACE_HEIGHT = 720;

    private Surface objSurface;
    private IRenderView mRenderView;

    private KANTVMgr mGraphicBenchMgr = null;
    private MyEventListener mEventListener = new MyEventListener();
    private boolean bShowInfo = false;

    private int mVideoWidth;
    private int mVideoHeight;

    private int mVideoRotationDegree;

    private int mVideoTargetRotationDegree;

    private Matrix mOriginalMatrix;

    private IMediaController mMediaController;

    private AppCompatActivity mActivity;
    private KanTVBenchmarkActivity mKanTVBenchmarkActivity;
    private Context mAppContext;
    private Settings mSettings;
    private SharedPreferences mSharedPreferences;
    private int mVideoSarNum = 0;
    private int mVideoSarDen = 0;
    private boolean mBenchmarkDone = false;

    private float mVideoScale = 1.0f;

    private boolean mIsNormalScreen = true;

    private int mScreenOrWidth;
    private int mScreenOrHeight;
    private Matrix mSaveMatrix;


    private boolean mEnableLandscape = true;
    private boolean mEnableFullscreen = false;
    private boolean isActionBarShowing = false;

    private int mKeyFSM[] = new int[4];

    public KanTVBenchmarkView(Context context) {
        super(context);
        initVideoView(context);
    }

    public KanTVBenchmarkView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initVideoView(context);
    }

    public KanTVBenchmarkView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initVideoView(context);
    }


    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public KanTVBenchmarkView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initVideoView(context);
    }


    private void initVideoView(Context context) {
        mAppContext = context.getApplicationContext();
        mSettings = new Settings(mAppContext);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);

        initRenders();

        mVideoWidth = 0;
        mVideoHeight = 0;

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
    }

    public void setRenderView(IRenderView renderView) {
        if (mRenderView != null) {
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
                {
                    renderView.setVideoSize(SURFACE_WIDTH, SURFACE_WIDTH);
                    renderView.setVideoSampleAspectRatio(4, 3);
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
                KANTVLog.j(TAG, String.format(Locale.getDefault(), "invalid render %d\n", render));
                break;
        }
    }


    public void setActivity(AppCompatActivity activity) {
        mActivity = activity;
        mKanTVBenchmarkActivity = (KanTVBenchmarkActivity) activity;
    }


    public void setMediaController(IMediaController controller) {
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = controller;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaController != null) {
            mMediaController.setMediaPlayer(this);
            View anchorView = this.getParent() instanceof View ? (View) this.getParent() : this;
            mMediaController.setAnchorView(anchorView);
            mMediaController.setEnabled(true);
        }
    }
    public void release() {
        stopBenchmark();
    }

    public void destroy() {
        release();
        mKanTVBenchmarkActivity.finish();
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (mMediaController != null) {
            toggleMediaControlsVisibility();
        }
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        if (mMediaController != null) {
            toggleMediaControlsVisibility();
        }
        return false;
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
        if (isKeyCodeSupported && mMediaController != null) {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                    keyCode == KeyEvent.KEYCODE_DPAD_CENTER ||
                    keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                mMediaController.show();
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_PLAY) {
                mMediaController.hide();
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE) {
                mMediaController.show();
                return true;
            } else {
                toggleMediaControlsVisibility();
            }
        }

        return super.onKeyDown(keyCode, event);
    }

    private void toggleMediaControlsVisibility() {
        if (isActionBarShowing)
        {
            mMediaController.hide();
            isActionBarShowing = false;
        } else {
            mMediaController.show();
            isActionBarShowing = true;
        }
    }

    @Override
    public void start() {
        KANTVLog.d(TAG, "start");
    }

    @Override
    public void pause() {

    }


    @Override
    public int getDuration() {
        return 0;
    }

    @Override
    public int getCurrentPosition() {
        return 0;
    }

    @Override
    public void seekTo(int pos) {

    }

    @Override
    public boolean isPlaying() {
        return false;
    }

    @Override
    public int getBufferPercentage() {
        return 0;
    }

    @Override
    public boolean canPause() {
        return false;
    }

    @Override
    public boolean canSeekBackward() {
        return false;
    }

    @Override
    public boolean canSeekForward() {
        return false;
    }

    @Override
    public int getAudioSessionId() {
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
        setRender(mCurrentRender);
    }

    IRenderView.IRenderCallback mSHCallback = new IRenderView.IRenderCallback() {
        @Override
        public void onSurfaceChanged(@NonNull IRenderView.ISurfaceHolder holder, int format, int w, int h) {
            KANTVLog.j(TAG, "surface changed: width=" + w + ",height=" + h + ",format=" + format);
            if (holder.getRenderView() != mRenderView) {
                Log.e(TAG, "onSurfaceChanged: unmatched render callback\n");
                return;
            }
            objSurface = holder.openSurface();
            if (objSurface != null) {
                KANTVLog.j(TAG, "got valid surface");
                startBenchmark();
            }
        }

        @Override
        public void onSurfaceCreated(@NonNull IRenderView.ISurfaceHolder holder, int width, int height) {
            KANTVLog.j(TAG, "onSurfaceCreated: width=" + width + ",height=" + height);
            if (holder.getRenderView() != mRenderView) {
                KANTVLog.j(TAG, "onSurfaceCreated: unmatched render callback\n");
                return;
            }
        }

        @Override
        public void onSurfaceDestroyed(@NonNull IRenderView.ISurfaceHolder holder) {
            KANTVLog.j(TAG, "on surface destroyed");
            if (holder.getRenderView() != mRenderView) {
                KANTVLog.j(TAG, "onSurfaceDestroyed: unmatched render callback\n");
                return;
            }
            if (mBenchmarkDone)
                return;

            if (!KANTVUtils.getCouldExitApp()) {
                KANTVLog.j(TAG, "force exit app");
                KANTVUtils.exitAPK(mActivity);
                return;
            }
            stopBenchmark();
        }
    };

    private void initKANTVMgr() {
        if (mGraphicBenchMgr != null) {
            return;
        }

        try {
            mGraphicBenchMgr = new KANTVMgr(mEventListener);
            KANTVLog.j(TAG, "Mgr version:" + mGraphicBenchMgr.getMgrVersion());
        } catch (KANTVException ex) {
            String errorMsg = "An Exception was thrown because:\n" + " " + ex.getMessage();
            KANTVLog.j(TAG, "error occured: " + errorMsg);
            showMsgBox(mActivity, errorMsg);
            ex.printStackTrace();
        }
    }

    public void startBenchmark() {
        if (mGraphicBenchMgr != null)
            return;
        if (objSurface == null)
            return;

        initKANTVMgr();

        if (mGraphicBenchMgr != null) {
            mGraphicBenchMgr.open();
            //KANTVLog.j(TAG, "encode id:" + encodeID + ",width:" + width + ",height:" + height + ",fps:" + fps);
            mGraphicBenchMgr.setStreamFormat(0, 0, SURFACE_WIDTH, SURFACE_HEIGHT, 25, AV_PIX_FMT_YUV420P, 0, 1, 0);
            KANTVLog.j(TAG, "surface:" + objSurface.toString());
            mGraphicBenchMgr.enablePreview(0, objSurface);
            mGraphicBenchMgr.startGraphicBenchmarkPreview();
            KANTVUtils.setCouldExitApp(false);
        }
    }

    public void stopBenchmark() {
        if (mGraphicBenchMgr == null) {
            KANTVLog.j(TAG, "benchmark already released");
            return;
        }

        KANTVLog.j(TAG, "release kantvmgr");
        if (mGraphicBenchMgr != null) {
            mGraphicBenchMgr.stopGraphicBenchmarkPreview();
            mGraphicBenchMgr.disablePreview(0);
            mGraphicBenchMgr.close();
            mGraphicBenchMgr.release();
            mGraphicBenchMgr = null;
        }
    }

    protected class MyEventListener implements KANTVEventListener {

        MyEventListener() {
        }


        @Override
        public void onEvent(KANTVEventType eventType, int what, int arg1, int arg2, Object obj) {
            String eventString = "got event from native layer: " + eventType.toString() + " (" + what + ":" + arg1 + " ) :" + (String) obj;
            String content = (String) obj;
            KANTVLog.j(TAG, "event:" + eventString);

            if (eventType.getValue() == KANTVEvent.KANTV_ERROR) {

            }
            if (eventType.getValue() == KANTVEvent.KANTV_INFO) {
                if (arg1 == KANTVEvent.KANTV_INFO_GRAPHIC_BENCHMARK_START) {
                    KANTVLog.j(TAG, "benchmark start from native layer");
                }

                if (arg1 == KANTVEvent.KANTV_INFO_GRAPHIC_BENCHMARK_STOP) {
                    KANTVLog.j(TAG, "benchmark stop from native layer");
                    mBenchmarkDone = true;
                    KANTVUtils.setCouldExitApp(true);
                }
            }

        }
    }


    private void showMsgBox(Context context, String message) {
        android.app.AlertDialog dialog = new AlertDialog.Builder(context).create();
        dialog.setMessage(message);
        dialog.setButton(DialogInterface.BUTTON_NEUTRAL, "OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {

            }
        });
        dialog.show();
    }

    public void showMediaInfo() {
        if (!bShowInfo) {
            bShowInfo = true;
            mMediaController.show();
        } else {
            bShowInfo = false;
        }
        TableLayoutBinder builder = new TableLayoutBinder(getContext());
        mSettings.updateUILang(mActivity);
        builder.setBackgroundColor(Color.parseColor("#80FFFFFF"));
        builder.appendRow2(R.string.brand_info, Build.BRAND + "                                                                       ");
        builder.appendRow2(R.string.cpu_abi, Build.CPU_ABI);
        builder.appendRow2(R.string.hardware_info, Build.HARDWARE);
        builder.appendRow2(R.string.os_info, "Android " + android.os.Build.VERSION.RELEASE);
        //builder.appendRow2(R.string.fingerprint_info, Build.FINGERPRINT);
        androidx.appcompat.app.AlertDialog.Builder adBuilder = builder.buildAlertDialogBuilder();
        adBuilder.setTitle(R.string.device_info);
        adBuilder.setNegativeButton(R.string.close, null);
        adBuilder.show();
    }

    public static native int kantv_anti_remove_rename_this_file();
}