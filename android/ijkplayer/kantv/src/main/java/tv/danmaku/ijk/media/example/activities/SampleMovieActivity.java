package tv.danmaku.ijk.media.example.activities;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.view.Menu;
import android.view.MenuItem;

import tv.danmaku.ijk.media.example.R;
import tv.danmaku.ijk.media.example.application.AppActivity;
import tv.danmaku.ijk.media.example.fragments.SampleMediaListFragment;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import static tv.danmaku.ijk.media.example.application.MediaType.MEDIA_MOVIE;


public class SampleMovieActivity extends AppActivity {

    public static Intent newIntent(Context context) {
        Intent intent = new Intent(context, SampleMovieActivity.class);
        return intent;
    }

    public static void intentTo(Context context) {
        context.startActivity(newIntent(context));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Fragment newFragment = SampleMediaListFragment.newInstance(MEDIA_MOVIE);

        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();

        transaction.replace(R.id.body, newFragment);
        transaction.commit();
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

