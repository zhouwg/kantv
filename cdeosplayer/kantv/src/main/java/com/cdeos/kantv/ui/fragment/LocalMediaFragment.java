package com.cdeos.kantv.ui.fragment;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.KeyEvent;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout;

import com.blankj.utilcode.util.ToastUtils;
import com.cdeos.kantv.mvp.impl.LocalMediaFragmentPresenterImpl;
import com.cdeos.kantv.utils.Settings;
import com.tbruyelle.rxpermissions2.RxPermissions;
import com.cdeos.kantv.R;
import com.cdeos.kantv.base.BaseMvpFragment;
import com.cdeos.kantv.base.BaseRvAdapter;
import com.cdeos.kantv.bean.FolderBean;
import com.cdeos.kantv.bean.event.OpenFolderEvent;
import com.cdeos.kantv.mvp.presenter.LocalMediaFragmentPresenter;
import com.cdeos.kantv.mvp.view.LocalMediaFragmentView;
import com.cdeos.kantv.ui.activities.play.FolderActivity;
import com.cdeos.kantv.ui.weight.dialog.CommonDialog;
import com.cdeos.kantv.ui.weight.item.FolderItem;
import com.cdeos.kantv.utils.CommonUtils;
import com.cdeos.kantv.utils.FileNameComparator;
import com.cdeos.kantv.utils.interf.AdapterItem;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import butterknife.BindView;
import cdeos.media.player.CDEUtils;
import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import cdeos.media.player.CDELog;


public class LocalMediaFragment extends BaseMvpFragment<LocalMediaFragmentPresenter> implements LocalMediaFragmentView {
    private static final String TAG = LocalMediaFragment.class.getName();
    public static final int UPDATE_ADAPTER_DATA = 0;
    public static final int UPDATE_DATABASE_DATA = 1;
    public static final int UPDATE_SYSTEM_DATA = 2;

    @BindView(R.id.refresh_layout)
    SwipeRefreshLayout refresh;
    @BindView(R.id.rv)
    RecyclerView recyclerView;

    private BaseRvAdapter<FolderBean> adapter;
    private Disposable permissionDis;
    private boolean updateVideoFlag = true;
    private int mCurrentPlayEngine = CDEUtils.PV_PLAYERENGINE__Exoplayer;

    public static LocalMediaFragment newInstance() {
        return new LocalMediaFragment();
    }

    @NonNull
    @Override
    protected LocalMediaFragmentPresenter initPresenter() {
        CDELog.j(TAG, "initPresenter");
        return new LocalMediaFragmentPresenterImpl(this, this);
    }

    @Override
    protected int initPageLayoutId() {
        return R.layout.fragment_play;
    }

    @SuppressLint("CheckResult")
    @Override
    public void initView() {
        refresh.setColorSchemeResources(R.color.theme_color);

        CDELog.j(TAG, "init view");
        FolderItem.PlayFolderListener itemListener = new FolderItem.PlayFolderListener() {
            @Override
            public void onClick(String folderPath) {
                CDELog.j(TAG, "on click");
                Intent intent = new Intent(getContext(), FolderActivity.class);
                intent.putExtra(OpenFolderEvent.FOLDERPATH, folderPath);
                startActivity(intent);
            }

            @Override
            public void onDelete(String folderPath, String title) {
                CDELog.j(TAG, "on delete");
                new CommonDialog.Builder(getContext())
                        .setOkListener(dialog -> {
                            presenter.deleteFolderVideo(folderPath);
                            refresh.setRefreshing(true);
                            refreshVideo(false);
                        })
                        .setAutoDismiss()
                        .build()
                        .show("确认删除文件夹 [" + title + "] 内视频文件？");
            }

            @Override
            public void onShield(String folderPath, String title) {
                CDELog.j(TAG, "on shield");
                new CommonDialog.Builder(getContext())
                        .setOkListener(dialog -> {
                            presenter.filterFolder(folderPath);
                            refresh.setRefreshing(true);
                            refreshVideo(false);
                        })
                        .setAutoDismiss()
                        .build()
                        .show("确认屏蔽文件夹 [" + title + "]及其子文件夹？");
            }
        };

        adapter = new BaseRvAdapter<FolderBean>(new ArrayList<>()) {
            @NonNull
            @Override
            public AdapterItem<FolderBean> onCreateItem(int viewType) {
                return new FolderItem(itemListener);
            }
        };

        recyclerView.setLayoutManager(new LinearLayoutManager(this.getContext(), LinearLayoutManager.VERTICAL, false));
        recyclerView.setNestedScrollingEnabled(false);
        recyclerView.setItemViewCacheSize(10);
        recyclerView.setAdapter(adapter);

        CDELog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
        if (updateVideoFlag) {
            CDELog.j(TAG, "calling initVideoData");
            refresh.setRefreshing(true);
            initVideoData();
        }


        if (true) {
            Settings mSettings;
            Context mContext;
            SharedPreferences mSharedPreferences;

            mContext = getContext();
            mSettings = new Settings(getActivity().getApplicationContext());
            mSettings.updateUILang((AppCompatActivity) getActivity());

            //attention here: force to use ffmpeg engine for local media
            mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
            SharedPreferences.Editor editor = mSharedPreferences.edit();
            mCurrentPlayEngine = mSettings.getPlayerEngine();
            CDELog.j(TAG, "current play engine is:" + mCurrentPlayEngine);
            String key = mContext.getString(R.string.pref_key_playerengine);
            String value = Integer.toString(CDEUtils.PV_PLAYERENGINE__FFmpeg);
            editor.putString(key, value);
            editor.commit();
            CDELog.j(TAG, "force play engine to:" + mSettings.getPlayerEngine());
        }

    }

    @Override
    public void initListener() {
        CDELog.j(TAG, "initListener");
        refresh.setOnRefreshListener(() -> refreshVideo(true));
    }

    @Override
    public void refreshAdapter(List<FolderBean> beans) {
        Collections.sort(beans, new FileNameComparator<FolderBean>() {
            @Override
            public String getCompareValue(FolderBean folderBean) {
                return CommonUtils.getFolderName(folderBean.getFolderPath());
            }
        });
        adapter.setData(beans);
        if (refresh != null)
            refresh.setRefreshing(false);
    }

    @Override
    public void deleteFolderSuccess() {
        refresh.setRefreshing(true);
        refreshVideo(false);
    }

    @Override
    public void showLoading() {

    }

    @Override
    public void hideLoading() {

    }

    @Override
    public void showError(String message) {
        ToastUtils.showShort(message);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (permissionDis != null)
            permissionDis.dispose();
    }

    @Override
    public void onResume() {
        CDELog.j(TAG, "on resume");
        super.onResume();
        refreshFolderData(UPDATE_SYSTEM_DATA);
    }

    @Override
    public void onStop() {
        CDELog.j(TAG, "on stop");
        super.onStop();
    }

    public void refreshFolderData(int updateType) {
        CDELog.j(TAG, "refreshFolderData");
        if (updateType == UPDATE_ADAPTER_DATA) {
            adapter.notifyDataSetChanged();
        } else {
            Intent intent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
            intent.setData(Uri.fromFile(Environment.getExternalStorageDirectory()));
            if (getContext() != null)
                getContext().sendBroadcast(intent);
            presenter.refreshVideo(getContext(), updateType == UPDATE_SYSTEM_DATA);
        }
    }

    public void initVideoData() {
        CDELog.j(TAG, "initVideoData");
        CDELog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
        if (presenter == null || refresh == null) {
            updateVideoFlag = true;
        } else {
            refresh.setRefreshing(true);
            presenter.refreshVideo(getContext(), true);
            updateVideoFlag = false;
        }
        CDELog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
    }


    @SuppressLint("CheckResult")
    private void refreshVideo(boolean reScan) {
        CDELog.j(TAG, "refreshVideo");
        new RxPermissions(this).
                request(Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                .subscribe(new Observer<Boolean>() {
                    @Override
                    public void onSubscribe(Disposable d) {
                        permissionDis = d;
                    }

                    @Override
                    public void onNext(Boolean granted) {
                        if (granted) {
                            presenter.refreshVideo(getContext(), reScan);
                        } else {
                            ToastUtils.showLong("未授予文件管理权限，无法扫描视频");
                            refresh.setRefreshing(false);
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                        e.printStackTrace();
                    }

                    @Override
                    public void onComplete() {
                        CDELog.j(TAG, "finish scan files");
                    }
                });
    }

    public static native int kantv_anti_tamper();
}
