/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package tv.danmaku.ijk.kantv.activities;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TableLayout;
import android.widget.TextView;

import java.util.regex.Pattern;

import tv.danmaku.ijk.kantv.content.MediaType;
import tv.danmaku.ijk.kantv.widget.media.VoisePlayingIcon;
import tv.danmaku.ijk.media.player.IjkLog;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;
import tv.danmaku.ijk.media.player.misc.ITrackInfo;
import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.kantv.application.Settings;
import tv.danmaku.ijk.kantv.content.RecentMediaStorage;
import tv.danmaku.ijk.kantv.fragments.TracksFragment;
import tv.danmaku.ijk.kantv.widget.media.AndroidMediaController;
import tv.danmaku.ijk.kantv.widget.media.IjkVideoView;
import tv.danmaku.ijk.kantv.widget.media.MeasureHelper;

public class VideoActivity extends AppCompatActivity implements TracksFragment.ITrackHolder {
    private static final String TAG = "VideoActivity";

    private String mVideoPath;
    private Uri    mVideoUri;
    private String mVideoTitle;
    private String mMediaType;

    private AndroidMediaController mMediaController;
    private IjkVideoView mVideoView;
    private TextView mToastTextView;
    private TableLayout mHudView;
    private VoisePlayingIcon mAudioAnimationView;

    private DrawerLayout mDrawerLayout;
    private ViewGroup mRightDrawer;

    private Settings mSettings;
    private boolean mBackPressed;

    public static Intent newIntent(Context context, String videoPath, String videoTitle, MediaType mediaType) {
        Intent intent = new Intent(context, VideoActivity.class);
        intent.putExtra("videoPath", videoPath);
        intent.putExtra("videoTitle", videoTitle);
        intent.putExtra("mediaType", mediaType.toString());
        return intent;
    }

    public static void intentTo(Context context, String videoPath, String videoTitle, MediaType mediaType) {
        context.startActivity(newIntent(context, videoPath, videoTitle, mediaType));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_player);

        mSettings = new Settings(this);

        // handle arguments
        mVideoPath  = getIntent().getStringExtra("videoPath");
        mVideoTitle = getIntent().getStringExtra("videoTitle");
        mMediaType  = getIntent().getStringExtra("mediaType");

        IjkLog.d(TAG, "media type:" + mMediaType + " media path:" + mVideoPath + " media title:" + mVideoTitle );

        Intent intent = getIntent();
        String intentAction = intent.getAction();
        if (!TextUtils.isEmpty(intentAction)) {
            if (intentAction.equals(Intent.ACTION_VIEW)) {
                mVideoPath = intent.getDataString();
            } else if (intentAction.equals(Intent.ACTION_SEND)) {
                mVideoUri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                    String scheme = mVideoUri.getScheme();
                    if (TextUtils.isEmpty(scheme)) {
                        Log.e(TAG, "Null unknown scheme\n");
                        finish();
                        return;
                    }
                    if (scheme.equals(ContentResolver.SCHEME_ANDROID_RESOURCE)) {
                        mVideoPath = mVideoUri.getPath();
                    } else if (scheme.equals(ContentResolver.SCHEME_CONTENT)) {
                        Log.e(TAG, "Can not resolve content below Android-ICS\n");
                        finish();
                        return;
                    } else {
                        Log.e(TAG, "Unknown scheme " + scheme + "\n");
                        finish();
                        return;
                    }
                }
            }
        }

        if (!TextUtils.isEmpty(mVideoPath)) {
            String regEx = "[`~!@#$%^&*()+=|{}:;\\\\[\\\\].<>/?~！@（）——+|{}【】‘；：”“’。，、？']";
            String videoTitle  = Pattern.compile(regEx).matcher(mVideoTitle).replaceAll("").trim();
            //use "_KANTV_" as delimiter here
            String syncString = mVideoPath + "_KANTV_" + mMediaType + "_KANTV_" + videoTitle;
            new RecentMediaStorage(this).saveUrlAsync(syncString);
        }

        // init UI
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setDisplayShowHomeEnabled(true);
        getSupportActionBar().setDisplayUseLogoEnabled(true);
        getSupportActionBar().setDisplayShowTitleEnabled(true);
        String title = getResources().getString(R.string.app_name);
        if (MediaType.toMediaType(mMediaType) == MediaType.MEDIA_TV) {
            title = getResources().getString(R.string.tv);
        } if (MediaType.toMediaType(mMediaType) == MediaType.MEDIA_RADIO) {
            title = getResources().getString(R.string.radio);
        } if (MediaType.toMediaType(mMediaType) == MediaType.MEDIA_MOVIE) {
            title = getResources().getString(R.string.movie);
        } if (MediaType.toMediaType(mMediaType) == MediaType.MEDIA_FILE) {
            title = getResources().getString(R.string.file);
        }
        getSupportActionBar().setTitle(title);

        ActionBar actionBar = getSupportActionBar();
        mMediaController = new AndroidMediaController(this, false);
        mMediaController.setSupportActionBar(actionBar);

        mToastTextView = (TextView) findViewById(R.id.toast_text_view);
        mHudView = (TableLayout) findViewById(R.id.hud_view);
        mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        mRightDrawer = (ViewGroup) findViewById(R.id.right_drawer);
        mAudioAnimationView = (VoisePlayingIcon) findViewById(R.id.audio_animation_view);

        mDrawerLayout.setScrimColor(Color.TRANSPARENT);

        // init player
        IjkMediaPlayer.loadLibrariesOnce(null);
        IjkMediaPlayer.native_profileBegin("libijkplayer.so");

        mVideoView = (IjkVideoView) findViewById(R.id.video_view);
        mVideoView.setActivity(this);
        mVideoView.setMediaController(mMediaController);
        mVideoView.setHudView(mHudView);
        mVideoView.setAudioView(mAudioAnimationView);
        mVideoView.setMediaType(mMediaType);
        mVideoView.setVideoTitle(mVideoTitle);

        if (!isNetworkAvailable()) {
            mToastTextView.setText(R.string.network_un_available);
            mToastTextView.setGravity(Gravity.CENTER);
            mToastTextView.setVisibility(View.VISIBLE);
            return;
        }

        // prefer mVideoPath
        if (mVideoPath != null) {
            mVideoView.setVideoPath(mVideoPath);
        } else if (mVideoUri != null)
            mVideoView.setVideoURI(mVideoUri);
        else {
            Log.e(TAG, "Null Data Source\n");
            finish();
            return;
        }

        mVideoView.start();
        String status = getResources().getString(R.string.wating);
        mVideoView.startUIBuffering(status);
    }

    private boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        return ((activeNetworkInfo != null) && activeNetworkInfo.isConnected());
    }


    @Override
    public void onBackPressed() {
        mBackPressed = true;
        if (mVideoView.isEnableLandscape())
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        super.onBackPressed();
    }

    @Override
    protected void onStop() {
        super.onStop();

        if (mBackPressed || !mVideoView.isBackgroundPlayEnabled()) {
            mVideoView.stopPlayback();
            mVideoView.release(true);
            mVideoView.stopBackgroundPlay();
            finish();
        } else {
            mVideoView.enterBackground();
        }
        IjkMediaPlayer.native_profileEnd();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (mSettings.getDevMode()) {
            getMenuInflater().inflate(R.menu.menu_player_devmode, menu);
        } else {
            getMenuInflater().inflate(R.menu.menu_player, menu);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_toggle_ratio) {
            int aspectRatio = mVideoView.toggleAspectRatio();
            String aspectRatioText = MeasureHelper.getAspectRatioText(this, aspectRatio);
            mToastTextView.setText(aspectRatioText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_toggle_fullscreen) {
            boolean enableFullscreen = mVideoView.toggleFullscreen();
            String enabledText = getResources().getString(R.string.enter_fullscreen);
            String disabledText = getResources().getString(R.string.leave_fullscreen);
            String infoText = (enableFullscreen ? enabledText : disabledText);
            mToastTextView.setText(infoText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_toggle_landscape) {
            boolean enableLandscape = mVideoView.toggleLandscape();
            String landscapeText = getResources().getString(R.string.landscape_playback);
            String portraitText = getResources().getString(R.string.portrait_playback);
            String infoText = (enableLandscape ? landscapeText : portraitText);
            mToastTextView.setText(infoText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_toggle_tflite) {
            boolean enableTFLite = mVideoView.toggleTFLite();
            //this menu item only available in development mode and we think english is not a problem to developers
            String tfliteText = (enableTFLite ? "enable tensorflowlite"  : "disable tensorflowlite");
            mToastTextView.setText(tfliteText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_toggle_player) {
            int player = mVideoView.togglePlayer();
            String playerText = IjkVideoView.getPlayerText(this, player);
            mToastTextView.setText(playerText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_toggle_render) {
            int render = mVideoView.toggleRender();
            String renderText = IjkVideoView.getRenderText(this, render);
            mToastTextView.setText(renderText);
            mMediaController.showOnce(mToastTextView);
            return true;
        } else if (id == R.id.action_show_info) {
            mVideoView.showMediaInfo();
        } else if (id == R.id.action_show_tracks) {
            if (mDrawerLayout.isDrawerOpen(mRightDrawer)) {
                Fragment f = getSupportFragmentManager().findFragmentById(R.id.right_drawer);
                if (f != null) {
                    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                    transaction.remove(f);
                    transaction.commit();
                }
                mDrawerLayout.closeDrawer(mRightDrawer);
            } else {
                Fragment f = TracksFragment.newInstance();
                FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
                transaction.replace(R.id.right_drawer, f);
                transaction.commit();
                mDrawerLayout.openDrawer(mRightDrawer);
            }
        } else if (id == android.R.id.home) {
            mVideoView.stopPlayback();
            mVideoView.release(true);
            mVideoView.stopBackgroundPlay();
            finish();
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public ITrackInfo[] getTrackInfo() {
        if (mVideoView == null)
            return null;

        return mVideoView.getTrackInfo();
    }

    @Override
    public void selectTrack(int stream) {
        mVideoView.selectTrack(stream);
    }

    @Override
    public void deselectTrack(int stream) {
        mVideoView.deselectTrack(stream);
    }

    @Override
    public int getSelectedTrack(int trackType) {
        if (mVideoView == null)
            return -1;

        return mVideoView.getSelectedTrack(trackType);
    }

}
