 /*
  * Copyright (C) 2021 zhouwg <zhouwg2000@gmail.com>
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

//2021-06-24, weiguo: this file only used in "developer mode"
//because it's replaced with ContentListFragment.java which
//load EPG infos from local xml file(remote EPG server not available in open source project)
//and then render/layout the EPG data accordingly

 package tv.danmaku.ijk.kantv.activities;

 import android.content.Context;
 import android.content.Intent;
 import android.os.Bundle;
 import android.view.Menu;
 import android.view.MenuItem;

 import androidx.appcompat.widget.Toolbar;
 import androidx.fragment.app.Fragment;
 import androidx.fragment.app.FragmentTransaction;

 import tv.danmaku.ijk.kantv.R;
 import tv.danmaku.ijk.kantv.application.AppActivity;
 import tv.danmaku.ijk.kantv.application.Settings;
 import tv.danmaku.ijk.kantv.fragments.SampleDevFragment;


 public class SampleDevActivity extends AppActivity {
     private Settings mSettings;

     public static Intent newIntent(Context context) {
         Intent intent = new Intent(context, SampleDevActivity.class);
         intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
         return intent;
     }

     public static void intentTo(Context context) {
         context.startActivity(newIntent(context));
     }

     @Override
     protected void onCreate(Bundle savedInstanceState) {
         super.onCreate(savedInstanceState);

         mSettings = new Settings(this);

         Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
         setSupportActionBar(toolbar);
         getSupportActionBar().setDisplayHomeAsUpEnabled(true);
         getSupportActionBar().setDisplayShowHomeEnabled(true);
         getSupportActionBar().setDisplayUseLogoEnabled(true);
         getSupportActionBar().setDisplayShowTitleEnabled(true);
         String title = getResources().getString(R.string.devmode);
         getSupportActionBar().setTitle(title);

         Fragment newFragment = SampleDevFragment.newInstance();
         FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
         transaction.replace(R.id.body, newFragment);
         transaction.commit();
     }

     @Override
     protected int getOptionMenuId() {
         return R.id.action_devmode;
     }

     @Override
     public boolean onOptionsItemSelected(MenuItem item) {
         int id = item.getItemId();
         if (id == android.R.id.home) {
             finish();
         }

         return super.onOptionsItemSelected(item);
     }

     @Override
     public boolean onPrepareOptionsMenu(Menu menu) {
         boolean show = super.onPrepareOptionsMenu(menu);
         if (!show)
             return show;

         if (mSettings.getDevMode()) {
             MenuItem item = null;

             item = menu.findItem(R.id.action_tv);
             if (item != null)
                 item.setVisible(false);
             item = menu.findItem(R.id.action_movie);
             if (item != null)
                 item.setVisible(false);
             item = menu.findItem(R.id.action_radio);
             if (item != null)
                 item.setVisible(false);
             item = menu.findItem(R.id.action_file);
             if (item != null)
                 item.setVisible(false);

             item = menu.findItem(R.id.action_recent);
             if (item != null)
                 item.setVisible(false);

             item = menu.findItem(R.id.action_settings);
             if (item != null)
                 item.setVisible(false);

             item = menu.findItem(R.id.action_devmode);
             if (item != null)
                 item.setVisible(false);

             item = menu.findItem(R.id.action_quit);
             if (item != null)
                 item.setVisible(false);

         }

         return true;
     }
 }

