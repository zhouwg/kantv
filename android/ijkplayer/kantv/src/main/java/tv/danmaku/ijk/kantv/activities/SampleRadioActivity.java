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

package tv.danmaku.ijk.kantv.activities;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuItem;

import tv.danmaku.ijk.kantv.R;
import tv.danmaku.ijk.kantv.application.AppActivity;
import tv.danmaku.ijk.kantv.fragments.ContentListFragment;

import static tv.danmaku.ijk.kantv.content.MediaType.MEDIA_RADIO;


public class SampleRadioActivity extends AppActivity {

    public static Intent newIntent(Context context) {
        Intent intent = new Intent(context, SampleRadioActivity.class);
        return intent;
    }

    public static void intentTo(Context context) {
        context.startActivity(newIntent(context));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Fragment newFragment = ContentListFragment.newInstance(MEDIA_RADIO);

        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();

        transaction.replace(R.id.body, newFragment);
        transaction.commit();


    }


}

