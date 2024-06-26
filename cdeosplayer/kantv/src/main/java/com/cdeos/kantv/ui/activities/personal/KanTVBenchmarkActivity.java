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
package com.cdeos.kantv.ui.activities.personal;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;
import cdeos.media.player.KANTVDRM;


import com.cdeos.kantv.player.ffplayer.media.AndroidMediaController;
import com.cdeos.kantv.utils.Settings;
import com.cdeos.kantv.R;


public class KanTVBenchmarkActivity extends AppCompatActivity {
    private static final String TAG = KanTVBenchmarkActivity.class.getName();

    private Context mContext;
    private Activity mActivity;
    private Context mAppContext;
    private Settings mSettings;
    private SharedPreferences mSharedPreferences;
    private final static KANTVDRM mKANTVDRM = KANTVDRM.getInstance();


    private AndroidMediaController mMediaController;
    private KanTVBenchmarkView mBenchmarkView;


    private DrawerLayout mDrawerLayout;
    private ViewGroup mRightDrawer;


    private boolean mBackPressed;


    private void setFullScreen() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                        View.SYSTEM_UI_FLAG_FULLSCREEN |
                        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        WindowManager.LayoutParams attributes = getWindow().getAttributes();
        attributes.screenBrightness = 1.0f;
        getWindow().setAttributes(attributes);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        setFullScreen();

        int currentOrientation = getResources().getConfiguration().orientation;
        if (currentOrientation == Configuration.ORIENTATION_PORTRAIT) {
            CDELog.j(TAG, "portrait");
        } else if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE) {
            CDELog.j(TAG, "landscape");

        }
        setContentView(R.layout.activity_benchmark);

        mActivity = this;
        mContext = mActivity.getBaseContext();
        mAppContext = mActivity.getApplicationContext();
        mSettings = new Settings(this);
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext);


        CDEUtils.umStartGraphicBenchmark();


        {
            Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
            setSupportActionBar(toolbar);
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowHomeEnabled(true);
            getSupportActionBar().setDisplayUseLogoEnabled(true);
            getSupportActionBar().setDisplayShowTitleEnabled(true);
            String title = getResources().getString(R.string.graphic_benchmark_title);
            getSupportActionBar().setTitle(title);

            ActionBar actionBar = getSupportActionBar();
            mMediaController = new AndroidMediaController(this, false, false);
            mMediaController.setSupportActionBar(actionBar);
        }


        mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        mRightDrawer = (ViewGroup) findViewById(R.id.right_drawer);
        mDrawerLayout.setScrimColor(Color.TRANSPARENT);

        mBenchmarkView = (KanTVBenchmarkView) findViewById(R.id.benchmark_view);
        mBenchmarkView.setActivity(this);
        mBenchmarkView.setMediaController(mMediaController);

        mBenchmarkView.start();
    }


    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        CDELog.j(TAG, "detect configuration changed");
        String message = (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) ? "屏幕设置为横屏" : "屏幕设置为竖屏";
        CDELog.j(TAG, "message:" + message);

    }

    @Override
    public void onBackPressed() {
        CDELog.j(TAG, "onBackPressed");


        if (CDEUtils.getCouldExitApp()) {
            mBenchmarkView.destroy();
            super.onBackPressed();
        } else {
            CDELog.j(TAG, "can't exit benchmark view");
            showMsgBox(mActivity, mContext.getString(R.string.graphic_benchmark_is_doing));
        }

    }


    @Override
    protected void onDestroy() {
        CDELog.j(TAG, "on destroy");
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onDestroy();
    }

    @Override
    protected void onStop() {
        CDELog.j(TAG, "on stop");
        super.onStop();
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_benchmark, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                if (CDEUtils.getCouldExitApp()) {
                    mBenchmarkView.destroy();
                } else {
                    CDELog.j(TAG, "can't exit benchmark view");
                    showMsgBox(mActivity, mContext.getString(R.string.graphic_benchmark_is_doing));
                }
                break;
            case R.id.action_start_benchmark:
                mBenchmarkView.showMediaInfo();
                break;
        }
        return super.onOptionsItemSelected(item);
    }


    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        boolean show = super.onPrepareOptionsMenu(menu);
        if (!show)
            return show;

        return true;
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

    public static native int kantv_anti_remove_rename_this_file();

}
