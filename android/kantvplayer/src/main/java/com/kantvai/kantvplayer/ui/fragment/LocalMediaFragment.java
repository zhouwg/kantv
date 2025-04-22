package com.kantvai.kantvplayer.ui.fragment;

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
import com.kantvai.kantvplayer.mvp.impl.LocalMediaFragmentPresenterImpl;
import com.kantvai.kantvplayer.utils.Settings;
import com.tbruyelle.rxpermissions2.RxPermissions;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvpFragment;
import com.kantvai.kantvplayer.base.BaseRvAdapter;
import com.kantvai.kantvplayer.bean.FolderBean;
import com.kantvai.kantvplayer.bean.event.OpenFolderEvent;
import com.kantvai.kantvplayer.mvp.presenter.LocalMediaFragmentPresenter;
import com.kantvai.kantvplayer.mvp.view.LocalMediaFragmentView;
import com.kantvai.kantvplayer.ui.activities.play.FolderActivity;
import com.kantvai.kantvplayer.ui.weight.dialog.CommonDialog;
import com.kantvai.kantvplayer.ui.weight.item.FolderItem;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.FileNameComparator;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import butterknife.BindView;
import kantvai.media.player.KANTVUtils;
import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import kantvai.media.player.KANTVLog;


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
    private int mCurrentPlayEngine = KANTVUtils.PV_PLAYERENGINE__Exoplayer;

    public static LocalMediaFragment newInstance() {
        return new LocalMediaFragment();
    }

    @NonNull
    @Override
    protected LocalMediaFragmentPresenter initPresenter() {
        KANTVLog.j(TAG, "initPresenter");
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

        KANTVLog.j(TAG, "init view");
        FolderItem.PlayFolderListener itemListener = new FolderItem.PlayFolderListener() {
            @Override
            public void onClick(String folderPath) {
                KANTVLog.j(TAG, "on click");
                Intent intent = new Intent(getContext(), FolderActivity.class);
                intent.putExtra(OpenFolderEvent.FOLDERPATH, folderPath);
                startActivity(intent);
            }

            @Override
            public void onDelete(String folderPath, String title) {
                KANTVLog.j(TAG, "on delete");
                new CommonDialog.Builder(getContext())
                        .setOkListener(dialog -> {
                            presenter.deleteFolderVideo(folderPath);
                            refresh.setRefreshing(true);
                            refreshVideo(false);
                        })
                        .setAutoDismiss()
                        .build()
                        .show("confirm to delete video files in [" + title + "]？");
            }

            @Override
            public void onShield(String folderPath, String title) {
                KANTVLog.j(TAG, "on shield");
                new CommonDialog.Builder(getContext())
                        .setOkListener(dialog -> {
                            presenter.filterFolder(folderPath);
                            refresh.setRefreshing(true);
                            refreshVideo(false);
                        })
                        .setAutoDismiss()
                        .build()
                        .show("confirm to shield video files in [" + title + "]？");
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

        KANTVLog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
        if (updateVideoFlag) {
            KANTVLog.j(TAG, "calling initVideoData");
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
            KANTVLog.j(TAG, "current play engine is:" + mCurrentPlayEngine);
            String key = mContext.getString(R.string.pref_key_playerengine);
            String value = Integer.toString(KANTVUtils.PV_PLAYERENGINE__FFmpeg);
            editor.putString(key, value);
            editor.commit();
            KANTVLog.j(TAG, "force play engine to:" + mSettings.getPlayerEngine());
        }

    }

    @Override
    public void initListener() {
        KANTVLog.j(TAG, "initListener");
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
        KANTVLog.j(TAG, "on resume");
        super.onResume();
        refreshFolderData(UPDATE_SYSTEM_DATA);
    }

    @Override
    public void onStop() {
        KANTVLog.j(TAG, "on stop");
        super.onStop();
    }

    public void refreshFolderData(int updateType) {
        KANTVLog.j(TAG, "refreshFolderData");
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
        KANTVLog.j(TAG, "initVideoData");
        KANTVLog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
        if (presenter == null || refresh == null) {
            updateVideoFlag = true;
        } else {
            refresh.setRefreshing(true);
            presenter.refreshVideo(getContext(), true);
            updateVideoFlag = false;
        }
        KANTVLog.j(TAG, "updateVideoFlag=" + updateVideoFlag);
    }


    @SuppressLint("CheckResult")
    private void refreshVideo(boolean reScan) {
        KANTVLog.j(TAG, "refreshVideo");
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
                            ToastUtils.showLong("can't scan video files because required permission was not granted");
                            refresh.setRefreshing(false);
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                        e.printStackTrace();
                    }

                    @Override
                    public void onComplete() {
                        KANTVLog.j(TAG, "finish scan files");
                    }
                });
    }

    public static native int kantv_anti_remove_rename_this_file();
}
