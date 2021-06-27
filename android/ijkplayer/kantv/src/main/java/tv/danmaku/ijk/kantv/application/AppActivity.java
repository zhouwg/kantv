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

package tv.danmaku.ijk.kantv.application;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.view.Menu;
import android.view.MenuItem;

import java.util.Locale;

import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.kantv.activities.FileExplorerActivity;
import tv.danmaku.ijk.kantv.activities.RecentMediaActivity;
import tv.danmaku.ijk.kantv.activities.SampleRadioActivity;
import tv.danmaku.ijk.kantv.activities.SampleMovieActivity;
import tv.danmaku.ijk.kantv.activities.SampleTVActivity;
import tv.danmaku.ijk.kantv.activities.SettingsActivity;


@SuppressLint("Registered")
public abstract class AppActivity extends AppCompatActivity {
    private static final int MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 1;
    private Settings mSettings;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mSettings = new Settings(getApplicationContext());
        mSettings.updateUILang(this);

        setContentView(R.layout.activity_app);

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(false);
        getSupportActionBar().setDisplayShowTitleEnabled(false);
        getSupportActionBar().setDisplayShowHomeEnabled(false);
        getSupportActionBar().setDisplayUseLogoEnabled(false);

        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.READ_EXTERNAL_STORAGE)) {
                // TODO: show explanation
            } else {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                        MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE);
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE: {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    // permission was granted, yay! Do the task you need to do.
                } else {
                    // permission denied, boo! Disable the functionality that depends on this permission.
                }
            }
        }
    }

    protected abstract int getOptionMenuId();

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_app, menu);
        MenuItem item = menu.findItem(getOptionMenuId());
        SpannableString spanString = new SpannableString((item.getTitle().toString()));
        spanString.setSpan(new ForegroundColorSpan(Color.LTGRAY), 0, spanString.length(), 0);
        item.setTitle(spanString);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            SettingsActivity.intentTo(this);
            return true;
        } else if (id == R.id.action_recent) {
            RecentMediaActivity.intentTo(this);
        } else if (id == R.id.action_file) {
            FileExplorerActivity.intentTo(this);
        } else if (id == R.id.action_movie) {
            SampleMovieActivity.intentTo(this);
        } else if (id == R.id.action_radio) {
            SampleRadioActivity.intentTo(this);
        } else if (id == R.id.action_tv) {
            SampleTVActivity.intentTo(this);
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        boolean show = super.onPrepareOptionsMenu(menu);
        if (!show)
            return show;

        //MenuItem item = menu.findItem(R.id.action_recent);
        //if (item != null)
        //    item.setVisible(false);

        return true;
    }
}
