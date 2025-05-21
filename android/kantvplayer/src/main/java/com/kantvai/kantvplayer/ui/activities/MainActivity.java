 /*
  * Copyright (c) Project KanTV. 2021-2023
  * Copyright (c) 2024- KanTV Authors
  */
package com.kantvai.kantvplayer.ui.activities;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.fragment.app.FragmentTransaction;

import com.blankj.utilcode.util.AppUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.ui.fragment.AIResearchFragment;
import com.kantvai.kantvplayer.ui.fragment.LLMResearchFragment;
import com.kantvai.kantvplayer.ui.fragment.TVGridFragment;
import com.kantvai.kantvplayer.utils.Settings;
import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.kantvai.kantvplayer.ui.fragment.EPGListFragment;
import com.tbruyelle.rxpermissions2.RxPermissions;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvpActivity;
import com.kantvai.kantvplayer.base.BaseMvpFragment;
import com.kantvai.kantvplayer.bean.event.UpdateFragmentEvent;
import com.kantvai.kantvplayer.mvp.impl.MainPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.MainPresenter;
import com.kantvai.kantvplayer.mvp.view.MainView;
import com.kantvai.kantvplayer.ui.fragment.PersonalFragment;
import com.kantvai.kantvplayer.ui.fragment.LocalMediaFragment;
import com.kantvai.kantvplayer.ui.weight.dialog.CommonEditTextDialog;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import butterknife.BindView;
import kantvai.media.player.KANTVUtils;
import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import kantvai.media.player.KANTVLog;

public class MainActivity extends BaseMvpActivity<MainPresenter> implements MainView {
    @BindView(R.id.navigation_view)
    BottomNavigationView navigationView;

    private static final String TAG = MainActivity.class.getName();

    private TVGridFragment homeFragment;
    private LocalMediaFragment LocalMediaFragment;
    private PersonalFragment personalFragment;
    private BaseMvpFragment previousFragment;
    private AIResearchFragment airesearchFragment;
    private LLMResearchFragment llmFragment;
    //private AIAgentFragment agentFragment;

    private Menu      optionMenu;
    private MenuItem  menuNetItem;
    private MenuItem  menuAIAgent;

    private Context mContext;
    private Activity mActivity;
    private Settings mSettings;

    private long touchTime = 0;

    private long switchTime = 0;

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_main;
    }

    @NonNull
    @Override
    protected MainPresenterImpl initPresenter() {
        return new MainPresenterImpl(this, this);
    }

    @Override
    public void initView() {
        if (hasBackActionbar() && getSupportActionBar() != null) {
            ActionBar actionBar = getSupportActionBar();
            actionBar.setDisplayHomeAsUpEnabled(false);
            actionBar.setDisplayShowTitleEnabled(true);
        }

        boolean usingLocalMediaAsHome = false;
        //navigationView.setLabelVisibilityMode(LabelVisibilityMode.LABEL_VISIBILITY_LABELED);
        if (usingLocalMediaAsHome) {
            setTitle(getBaseContext().getString(R.string.localmedia));
            navigationView.setSelectedItemId(R.id.navigation_play);
            switchFragment(LocalMediaFragment.class);
        } else {
            setTitle(getBaseContext().getString(R.string.onlinetv));
            navigationView.setSelectedItemId(R.id.navigation_home);
            switchFragment(TVGridFragment.class);
        }

        initPermission();
    }

    private void initPermission() {
        new RxPermissions(this).
                request(Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                .subscribe(new Observer<Boolean>() {
                    @Override
                    public void onSubscribe(Disposable d) {

                    }

                    @Override
                    public void onNext(Boolean granted) {
                        if (granted) {
                            KANTVLog.d(TAG, "init scan folder");
                            presenter.initScanFolder();
                            if (LocalMediaFragment != null) {
                                LocalMediaFragment.initVideoData();
                            }
                        } else {
                            ToastUtils.showLong("can't scan video because required permission was not granted");
                            KANTVLog.d(TAG, "can't scan video because required permission was not granted");
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                        e.printStackTrace();
                        KANTVLog.d(TAG, "error: " + e.toString());
                    }

                    @Override
                    public void onComplete() {

                    }
                });
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        if (!IApplication.startCorrectlyFlag) {
            IApplication.startCorrectlyFlag = true;
            FragmentTransaction fragmentTransaction = getFragmentTransaction();
            if (homeFragment != null)
                fragmentTransaction.remove(homeFragment);
            if (LocalMediaFragment != null)
                fragmentTransaction.remove(LocalMediaFragment);
            if (llmFragment != null)
                fragmentTransaction.remove(llmFragment);
            if (personalFragment != null)
                fragmentTransaction.remove(personalFragment);
            if (airesearchFragment != null)
                fragmentTransaction.remove(airesearchFragment);
            //if (agentFragment != null)
            //    fragmentTransaction.remove(agentFragment);

            fragmentTransaction.commitAllowingStateLoss();
        }
        super.onSaveInstanceState(outState);
    }

    @Override
    public void initListener() {
        navigationView.setOnNavigationItemSelectedListener(item -> {
            KANTVLog.g(TAG, "System.currentTimeMillis() - switchTime " + (System.currentTimeMillis() - switchTime));
            //FIXME:workaround to fix a potential deadlock in a special scenario of AI inference(realtime video inference)
            if (item.getItemId() == R.id.navigation_aiagent) {
                if (System.currentTimeMillis() - switchTime < 900) {
                    ToastUtils.showShort("switch duration is too short");
                    switchTime = System.currentTimeMillis();
                    return false;
                }
            }
            switchTime = System.currentTimeMillis();

            KANTVLog.d(TAG, "item id: " + item.getItemId());
            switch (item.getItemId()) {
                case R.id.navigation_home:
                    setTitle(mActivity.getBaseContext().getString(R.string.onlinetv));
                    switchFragment(TVGridFragment.class);
                    //menuNetItem.setVisible(false);
                    hideOptionMenu();
                    return true;

                case R.id.navigation_play:
                    setTitle(mActivity.getBaseContext().getString(R.string.localmedia));
                    switchFragment(LocalMediaFragment.class);
                    //menuNetItem.setVisible(true);
                    showLocalMediaMenu();
                    return true;

                case R.id.navigation_aiagent:
                    //setTitle("Realtime Inference");
                    //switchFragment(AIAgentFragment.class);
                    setTitle("LLM Inference");
                    switchFragment(LLMResearchFragment.class);
                    //menuNetItem.setVisible(false);
                    //showAIAgentMenu();
                    return true;

                case R.id.navigation_asr:
                    setTitle("on-device AI on Android phone");
                    switchFragment(AIResearchFragment.class);
                    //menuNetItem.setVisible(false);
                    hideOptionMenu();
                    return true;

                case R.id.navigation_personal:
                    setTitle(mActivity.getBaseContext().getString(R.string.personal_center));
                    switchFragment(PersonalFragment.class);
                    //menuNetItem.setVisible(false);
                    hideOptionMenu();
                    return true;
            }
            return false;
        });
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        KANTVLog.d(TAG, "keyCode:" + keyCode);
        if (!KANTVUtils.getCouldExitApp()) {
            KANTVLog.d(TAG, "can't exit app at the moment");
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (System.currentTimeMillis() - touchTime > 1500) {
                ToastUtils.showShort(getBaseContext().getString(R.string.key_press_warning));
                touchTime = System.currentTimeMillis();
            } else {
                AppUtils.exitApp();
            }
        }
        return super.onKeyDown(keyCode, event);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_main, menu);
        menuNetItem = menu.findItem(R.id.menu_item_network);
        menuNetItem.setVisible(false);

        optionMenu = menu;
        for (int i = 0; i < menu.size(); i++) {
            menu.getItem(i).setVisible(false);
        }
        return super.onCreateOptionsMenu(menu);
    }

    private void hideOptionMenu()
    {
        for (int i = 0; i < optionMenu.size(); i++) {
            optionMenu.getItem(i).setVisible(false);
        }
    }

    private void showLocalMediaMenu() {
        for (int i = 0; i < optionMenu.size(); i++) {
            optionMenu.getItem(i).setVisible(false);
        }
        menuNetItem.setVisible(true);
    }

    private void showAIAgentMenu() {
        for (int i = 0; i < optionMenu.size(); i++) {
            optionMenu.getItem(i).setVisible(true);
        }
        menuNetItem.setVisible(false);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_item_network:
                new CommonEditTextDialog(this, CommonEditTextDialog.NETWORK_LINK).show();
                break;
            default:
                //if (agentFragment != null)
                //    agentFragment.onMenuItemSelected(item);
                break;
        }
        return super.onOptionsItemSelected(item);
    }


    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EventBus.getDefault().register(this);

        mActivity = this;
        mContext = mActivity.getBaseContext();
        mSettings = new Settings(mContext);

        KANTVUtils.dumpDeviceInfo();

        //if (KANTVUtils.isEmulator(this))
        if (false)
        {
            showWarningDialog(this, "can't running this app in emulator");
            navigationView.getMenu().findItem(R.id.navigation_play).setEnabled(false);
            navigationView.getMenu().findItem(R.id.navigation_home).setEnabled(false);
            navigationView.getMenu().findItem(R.id.navigation_aiagent).setEnabled(false);
            navigationView.getMenu().findItem(R.id.navigation_asr).setEnabled(false);
            navigationView.getMenu().findItem(R.id.navigation_personal).setEnabled(false);
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
        }

        if (KANTVUtils.getReleaseMode()) {
            //if (KANTVUtils.isAppDebuggable(this) || KANTVUtils.isDebuggerAttached())
            if (false)
            {
                showWarningDialog(this, "warning:the app seems running within debugger");
                navigationView.getMenu().findItem(R.id.navigation_play).setEnabled(false);
                navigationView.getMenu().findItem(R.id.navigation_home).setEnabled(false);
                navigationView.getMenu().findItem(R.id.navigation_aiagent).setEnabled(false);
                navigationView.getMenu().findItem(R.id.navigation_asr).setEnabled(false);
                navigationView.getMenu().findItem(R.id.navigation_personal).setEnabled(false);
                getWindow().setFlags(WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE, WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
            }

        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onEvent(UpdateFragmentEvent event) {
        if (event.getClazz() == LocalMediaFragment.class && LocalMediaFragment != null) {
            LocalMediaFragment.refreshFolderData(event.getUpdateType());
        } else if (event.getClazz() == PersonalFragment.class && personalFragment != null) {
            personalFragment.initView();
        }
    }

    @SuppressLint("CommitTransaction")
    private FragmentTransaction getFragmentTransaction() {
        return getSupportFragmentManager().beginTransaction();
    }

    private void switchFragment(Class clazz) {
        if (previousFragment != null && clazz.isInstance(previousFragment)) {
            return;
        } else if (previousFragment != null) {
            getFragmentTransaction().hide(previousFragment).commit();
        }

        if (previousFragment != null) {
            String fragmentName = previousFragment.getClass().getName();
            if (fragmentName.contains("AIResearchFragment")) {
                KANTVLog.d(TAG, "release ASR resource");
                airesearchFragment.release();
                airesearchFragment.stopLLMInference();
            }

            if (fragmentName.contains("LLMResearchFragment")) {
                KANTVLog.d(TAG, "release LLM resource");
                llmFragment.stopLLMInference();
                llmFragment.release();
            }

            if (fragmentName.contains("AgentFragment")) {
                KANTVLog.d(TAG, "release Agent resource");
                //agentFragment.release();
            }
        }


        if (clazz == TVGridFragment.class) {
            if (homeFragment == null) {
                homeFragment = TVGridFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, homeFragment).commit();
            } else {
                getFragmentTransaction().show(homeFragment).commit();
            }
            previousFragment = homeFragment;
        } else if (clazz == PersonalFragment.class) {
            if (personalFragment == null) {
                personalFragment = PersonalFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, personalFragment).commit();
            } else {
                getFragmentTransaction().show(personalFragment).commit();
            }
            previousFragment = personalFragment;
        } else if (clazz == LocalMediaFragment.class) {
            if (LocalMediaFragment == null) {
                LocalMediaFragment = LocalMediaFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, LocalMediaFragment).commit();
            } else {
                getFragmentTransaction().show(LocalMediaFragment).commit();
            }
            previousFragment = LocalMediaFragment;
        } else if (clazz == LLMResearchFragment.class) {
            if (llmFragment == null) {
                llmFragment = LLMResearchFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, llmFragment).commit();
            } else {
                getFragmentTransaction().show(llmFragment).commit();
                llmFragment.reload(0);
            }
            previousFragment = llmFragment;
        } else if (clazz == AIResearchFragment.class) {
            if (airesearchFragment == null) {
                airesearchFragment = AIResearchFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, airesearchFragment).commit();
            } else {
                getFragmentTransaction().show(airesearchFragment).commit();
            }
            previousFragment = airesearchFragment;
        } /*else if (clazz == AIAgentFragment.class) {
            if (agentFragment == null) {
                agentFragment = AIAgentFragment.newInstance();
                getFragmentTransaction().add(R.id.fragment_container, agentFragment).commit();
            } else {
                getFragmentTransaction().show(agentFragment).commit();
                agentFragment.initCamera();
            }
            previousFragment = agentFragment;
        } */
    }


    private void showWarningDialog(Context context, String warningInfo) {
        new AlertDialog.Builder(this)
                .setTitle("Tip")
                .setMessage(warningInfo)
                .setCancelable(true)
                .setNegativeButton(context.getString(R.string.OK),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                //dialog.dismiss();
                                KANTVUtils.exitAPK(mActivity);
                            }
                        })
                .show();
    }

    public static native int kantv_anti_remove_rename_this_file();
}
