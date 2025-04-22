package com.kantvai.kantvplayer.ui.activities;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.app.SkinAppCompatDelegateImpl;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.blankj.utilcode.util.AppUtils;
import com.blankj.utilcode.util.ServiceUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.gyf.immersionbar.ImmersionBar;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.utils.CommonUtils;

import kantvai.media.player.KANTVLog;


public class ShellActivity extends AppCompatActivity {
    private static ShellActivity mActivity;
    private final static String TAG = ShellActivity.class.getName();

    @SuppressWarnings("unchecked")
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        mActivity = this;
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_shell);
        ImmersionBar.with(this)
                .statusBarColorInt(CommonUtils.getResColor(R.color.colorPrimaryDark))
                .init();
        Intent intent = getIntent();
        if (intent != null) {
            String className = intent.getStringExtra("fragment");
            if (!TextUtils.isEmpty(className)) {
                try {
                    FragmentManager manager = getSupportFragmentManager();
                    Fragment fragment = manager.getFragmentFactory().instantiate(getClassLoader(),className);
                    FragmentTransaction transaction = manager.beginTransaction();
                    transaction.add(R.id.frame_shell,fragment,className);
                    transaction.setPrimaryNavigationFragment(fragment);
                    transaction.commit();
                }catch ( ClassCastException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @NonNull
    @Override
    public AppCompatDelegate getDelegate() {
        return SkinAppCompatDelegateImpl.get(ShellActivity.this,this);
    }

    @NonNull
    public static ShellActivity getInstance() {
        return mActivity;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
