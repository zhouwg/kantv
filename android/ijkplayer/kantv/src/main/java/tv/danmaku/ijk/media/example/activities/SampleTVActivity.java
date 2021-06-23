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

package tv.danmaku.ijk.media.example.activities;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuItem;

import tv.danmaku.ijk.media.example.R;
import tv.danmaku.ijk.media.example.application.AppActivity;
import tv.danmaku.ijk.media.example.application.MediaType;
import tv.danmaku.ijk.media.example.fragments.SampleMediaListFragment;
import tv.danmaku.ijk.media.player.IjkLog;

import static tv.danmaku.ijk.media.example.application.MediaType.MEDIA_TV;

public class SampleTVActivity extends AppActivity  {
    private static String TAG = SampleTVActivity.class.getName();


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Fragment newFragment = SampleMediaListFragment.newInstance(MEDIA_TV);
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();

        transaction.replace(R.id.body, newFragment);
        transaction.commit();

        IjkLog.d(TAG, "******** Android build Informations:******\n");
        addBuildField("BOARD", Build.BOARD);
        addBuildField("BOOTLOADER", Build.BOOTLOADER);
        addBuildField("BRAND", Build.BRAND);
        addBuildField("CPU_ABI", Build.CPU_ABI);
        addBuildField("DEVICE", Build.DEVICE);
        addBuildField("DISPLAY", Build.DISPLAY);
        addBuildField("FINGERPRINT", Build.FINGERPRINT);
        addBuildField("HARDWARE", Build.HARDWARE);
        addBuildField("HOST", Build.HOST);
        addBuildField("ID", Build.ID);
        addBuildField("MANUFACTURER", Build.MANUFACTURER);
        addBuildField("MODEL", Build.MODEL);
        addBuildField("PRODUCT", Build.PRODUCT);
        addBuildField("SERIAL", Build.SERIAL);
        addBuildField("TAGS", Build.TAGS);
        addBuildField("TYPE", Build.TYPE);
        addBuildField("USER", Build.USER);
        addBuildField("ANDROID SDK", String.valueOf(android.os.Build.VERSION.SDK_INT));
        addBuildField("OS Version", android.os.Build.VERSION.RELEASE);
        IjkLog.d(TAG, "******************************************\n");
    }


    private void addBuildField(String name, String value) {
        //TODO: save the build filed infos to file
        //TODO: should I upload the user's device info to server? better user experience if keep kantv apk be a purely client application
        IjkLog.d(TAG, "  " + name + ": " + value + "\n");
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        boolean show = super.onPrepareOptionsMenu(menu);
        if (!show)
            return show;

        MenuItem item = menu.findItem(R.id.action_recent);
        if (item != null)
            item.setVisible(false);

        return true;
    }
}
