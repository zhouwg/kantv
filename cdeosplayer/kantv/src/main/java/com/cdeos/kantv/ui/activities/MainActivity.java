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
package com.cdeos.kantv.ui.activities;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
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
import com.cdeos.kantv.ui.fragment.AIResearchFragment;
import com.cdeos.kantv.ui.fragment.LLMResearchFragment;
import com.cdeos.kantv.ui.fragment.TVGridFragment;
import com.cdeos.kantv.utils.Settings;
import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.cdeos.kantv.ui.fragment.EPGListFragment;
import com.tbruyelle.rxpermissions2.RxPermissions;
import com.cdeos.kantv.R;
import com.cdeos.kantv.app.IApplication;
import com.cdeos.kantv.base.BaseMvpActivity;
import com.cdeos.kantv.base.BaseMvpFragment;
import com.cdeos.kantv.bean.event.UpdateFragmentEvent;
import com.cdeos.kantv.mvp.impl.MainPresenterImpl;
import com.cdeos.kantv.mvp.presenter.MainPresenter;
import com.cdeos.kantv.mvp.view.MainView;
import com.cdeos.kantv.ui.fragment.PersonalFragment;
import com.cdeos.kantv.ui.fragment.LocalMediaFragment;
import com.cdeos.kantv.ui.weight.dialog.CommonEditTextDialog;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import butterknife.BindView;
import cdeos.media.player.CDEUtils;
import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import cdeos.media.player.CDELog;

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
                            CDELog.d(TAG, "init scan folder");
                            presenter.initScanFolder();
                            if (LocalMediaFragment != null) {
                                LocalMediaFragment.initVideoData();
                            }
                        } else {
                            ToastUtils.showLong("can't scan video because required permission was not granted");
                            CDELog.d(TAG, "can't scan video because required permission was not granted");
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                        e.printStackTrace();
                        CDELog.d(TAG, "error: " + e.toString());
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
            CDELog.d(TAG, "item id: " + item.getItemId());
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
                    setTitle("ggml-hexagon on Android");
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
        CDELog.d(TAG, "keyCode:" + keyCode);
        if (!CDEUtils.getCouldExitApp()) {
            CDELog.d(TAG, "can't exit app at the moment");
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

        CDEUtils.dumpDeviceInfo();

        //if (CDEUtils.isEmulator(this))
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

        if (CDEUtils.getReleaseMode()) {
            //if (CDEUtils.isAppDebuggable(this) || CDEUtils.isDebuggerAttached())
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
                CDELog.d(TAG, "release ASR resource");
                airesearchFragment.release();
            }

            if (fragmentName.contains("LLMResearchFragment")) {
                CDELog.d(TAG, "release LLM resource");
                llmFragment.release();
            }

            if (fragmentName.contains("AgentFragment")) {
                CDELog.d(TAG, "release Agent resource");
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
                                CDEUtils.exitAPK(mActivity);
                            }
                        })
                .show();
    }

    public static native int kantv_anti_remove_rename_this_file();
}
