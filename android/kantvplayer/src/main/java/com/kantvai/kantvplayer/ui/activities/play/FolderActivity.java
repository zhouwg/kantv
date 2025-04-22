package com.kantvai.kantvplayer.ui.activities.play;


import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvpActivity;
import com.kantvai.kantvplayer.base.BaseRvAdapter;
import com.kantvai.kantvplayer.bean.BindResourceBean;
import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.bean.event.OpenFolderEvent;
import com.kantvai.kantvplayer.bean.event.SaveCurrentEvent;
import com.kantvai.kantvplayer.bean.event.UpdateFragmentEvent;
import com.kantvai.kantvplayer.mvp.impl.FolderPresenterImpl;
import com.kantvai.kantvplayer.mvp.presenter.FolderPresenter;
import com.kantvai.kantvplayer.mvp.view.FolderView;
import com.kantvai.kantvplayer.ui.fragment.LocalMediaFragment;
import com.kantvai.kantvplayer.ui.weight.dialog.CommonDialog;
import com.kantvai.kantvplayer.ui.weight.item.VideoItem;
import com.kantvai.kantvplayer.utils.AppConfig;
import com.kantvai.kantvplayer.utils.Constants;
import com.kantvai.kantvplayer.utils.FileNameComparator;
import com.kantvai.kantvplayer.utils.Settings;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import butterknife.BindView;
import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;


public class FolderActivity extends BaseMvpActivity<FolderPresenter> implements FolderView {
    @BindView(R.id.rv)
    RecyclerView recyclerView;

    private BaseRvAdapter<VideoBean> adapter;
    private List<VideoBean> videoList;
    private VideoBean selectVideoBean;
    private int selectPosition;
    private String folderPath;
    private static final String TAG = FolderActivity.class.getName();
    @Override
    public void initView() {
        KANTVLog.j(TAG, "init view");
        EventBus.getDefault().register(this);
        videoList = new ArrayList<>();
        folderPath = getIntent().getStringExtra(OpenFolderEvent.FOLDERPATH);
        String folderTitle = FileUtils.getFileNameNoExtension(folderPath.substring(0, folderPath.length() - 1));
        setTitle(folderTitle);

        if (true) {
            Settings mSettings;
            Context mContext;
            SharedPreferences mSharedPreferences;

            mSettings = new Settings(this.getApplicationContext());
            mContext = this.getApplicationContext();
            mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);
            SharedPreferences.Editor editor = mSharedPreferences.edit();
            int mCurrentPlayEngine = mSettings.getPlayerEngine();
            KANTVLog.j(TAG, "current play engine is:" + mCurrentPlayEngine);
            String key = mContext.getString(R.string.pref_key_playerengine);
            String value = Integer.toString(KANTVUtils.PV_PLAYERENGINE__FFmpeg);
            editor.putString(key, value);//using FFmpeg for local media
            editor.commit();
            KANTVLog.j(TAG, "force play engine to:" + mSettings.getPlayerEngine());
        }

        adapter = new BaseRvAdapter<VideoBean>(videoList) {
            @NonNull
            @Override
            public AdapterItem<VideoBean> onCreateItem(int viewType) {
                return new VideoItem(new VideoItem.VideoItemEventListener() {
                    @Override
                    public void onVideoDelete(int position) {
                        if (position >= videoList.size()) return;
                        new CommonDialog.Builder(FolderActivity.this)
                                .setOkListener(dialog -> {
                                    String path = videoList.get(position).getVideoPath();
                                    KANTVLog.j(TAG, "file path:" + path);
                                    if (FileUtils.delete(path)) {
                                        KANTVLog.j(TAG, "delete file ok");
                                        adapter.removeItem(position);
                                        EventBus.getDefault().post(UpdateFragmentEvent.updatePlay(LocalMediaFragment.UPDATE_DATABASE_DATA));
                                    } else {
                                        ToastUtils.showShort("delete file failure");
                                        KANTVLog.j(TAG, "delete folders failure");
                                    }
                                })
                                .setAutoDismiss()
                                .build()
                                .show("confirm to delete file？");
                    }

                    @Override
                    public void onOpenVideo(int position) {
                        if (position >= videoList.size()) return;
                        selectVideoBean = videoList.get(position);
                        selectPosition = position;
                        openIntentVideo(selectVideoBean);

                    }
                });
            }
        };

        recyclerView.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        recyclerView.setNestedScrollingEnabled(false);
        recyclerView.setItemViewCacheSize(10);
        recyclerView.setAdapter(adapter);

        presenter.getVideoList(folderPath);
    }

    @Override
    public void initListener() {
    }

    @Override
    public void refreshAdapter(List<VideoBean> beans) {
        videoList.clear();
        videoList.addAll(beans);
        sort(AppConfig.getInstance().getFolderSortType());
        adapter.notifyDataSetChanged();
    }

    @NonNull
    @Override
    protected FolderPresenter initPresenter() {
        return new FolderPresenterImpl(this, this);
    }

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_folder;
    }

    @Override
    public void showLoading() {
        showLoadingDialog("");
    }

    @Override
    public void hideLoading() {
        dismissLoadingDialog();
    }

    @Override
    public void showError(String message) {
        KANTVLog.j(TAG, "error:" + message);
        ToastUtils.showShort(message);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        if (item.getItemId() == R.id.item_video_edit) {
            if (videoList != null && videoList.size() > 0) {
                //showVideoEditDialog();
                showVideoSortDialog();
            } else {
                ToastUtils.showShort("video list is empty");
            }
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_folder, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (adapter != null) {
            adapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void saveCurrent(SaveCurrentEvent event) {
        adapter.getData().get(selectPosition).setCurrentPosition(event.getCurrentPosition());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            BindResourceBean bindResourceBean = data.getParcelableExtra("bind_data");
            if (bindResourceBean == null)
                return;

            int position = bindResourceBean.getItemPosition();
            if (position < 0 || position > videoList.size() || videoList.size() == 0)
                return;
            String videoPath = videoList.get(position).getVideoPath();
        }
    }


    @Override
    public void openIntentVideo(VideoBean videoBean) {
        launchPlay(videoBean);

    }


    private void launchPlay(VideoBean videoBean) {
        AppConfig.getInstance().setLastPlayVideo(videoBean.getVideoPath());
        EventBus.getDefault().post(UpdateFragmentEvent.updatePlay(LocalMediaFragment.UPDATE_ADAPTER_DATA));

        PlayerManagerActivity.launchPlayerLocal(
                this,
                FileUtils.getFileNameNoExtension(videoBean.getVideoPath()),
                videoBean.getVideoPath(),
                videoBean.getZimuPath(),
                videoBean.getCurrentPosition(),
                videoBean.getEpisodeId());
    }

    private void showVideoEditDialog() {
        final String[] playTypes = { "video sort"};

        new AlertDialog.Builder(this)
                .setTitle("edit")
                .setItems(playTypes, (dialog, which) -> {
                    switch (which) {
                        case 0:
                            showVideoSortDialog();
                            break;
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }


    private void showVideoSortDialog() {
        final String[] playTypes = {"sort by filename", "sort by duration", "sort by create time"};

        new AlertDialog.Builder(this)
                .setTitle("video sort")
                .setItems(playTypes, (dialog, which) -> {
                    switch (which) {
                        case 0:
                            int nameType = AppConfig.getInstance().getFolderSortType();
                            KANTVLog.j(TAG, "nameType:" + nameType);
                            if (nameType == Constants.FolderSort.NAME_ASC)
                                sort(Constants.FolderSort.NAME_DESC);
                            else if (nameType == Constants.FolderSort.NAME_DESC)
                                sort(Constants.FolderSort.NAME_ASC);
                            else
                                sort(Constants.FolderSort.NAME_ASC);
                            adapter.notifyDataSetChanged();
                            break;
                        case 1:
                            int durationType = AppConfig.getInstance().getFolderSortType();
                            KANTVLog.j(TAG, "durationType:" + durationType);
                            if (durationType == Constants.FolderSort.DURATION_ASC)
                                sort(Constants.FolderSort.DURATION_DESC);
                            else if (durationType == Constants.FolderSort.DURATION_DESC)
                                sort(Constants.FolderSort.DURATION_ASC);
                            else
                                sort(Constants.FolderSort.DURATION_ASC);
                            adapter.notifyDataSetChanged();
                            break;
                        default:
                            KANTVLog.j(TAG, "which is :" + which);
                            int generateTimeType = AppConfig.getInstance().getFolderSortType();
                            KANTVLog.j(TAG, "generateTimeType:" + generateTimeType);
                            if (generateTimeType == Constants.FolderSort.GENERATETIME_ASC)
                                sort(Constants.FolderSort.GENERATETIME_DESC);
                            else if (generateTimeType == Constants.FolderSort.GENERATETIME_DESC)
                                sort(Constants.FolderSort.GENERATETIME_ASC);
                            else
                                sort(Constants.FolderSort.GENERATETIME_ASC);
                            adapter.notifyDataSetChanged();
                            break;
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private void sort(int type) {
        if (type == Constants.FolderSort.NAME_ASC) {
            Collections.sort(videoList, new FileNameComparator<VideoBean>() {
                @Override
                public String getCompareValue(VideoBean videoBean) {
                    return FileUtils.getFileNameNoExtension(videoBean.getVideoPath());
                }
            });
        } else if (type == Constants.FolderSort.NAME_DESC) {
            Collections.sort(videoList, new FileNameComparator<VideoBean>(true) {
                @Override
                public String getCompareValue(VideoBean videoBean) {
                    return FileUtils.getFileNameNoExtension(videoBean.getVideoPath());
                }
            });
        } else if (type == Constants.FolderSort.DURATION_ASC) {
            Collections.sort(videoList,
                    (o1, o2) -> Long.compare(o1.getVideoDuration(), o2.getVideoDuration()));
        } else if (type == Constants.FolderSort.DURATION_DESC) {
            Collections.sort(videoList,
                    (o1, o2) -> Long.compare(o2.getVideoDuration(), o1.getVideoDuration()));
        } else if (type == Constants.FolderSort.GENERATETIME_ASC) {
            Collections.sort(videoList,
                    (o1, o2) -> Long.compare(o1.getVideoGenerateTime(), o2.getVideoGenerateTime()));
        } else if (type == Constants.FolderSort.GENERATETIME_DESC) {
            Collections.sort(videoList,
                (o1, o2) -> Long.compare(o2.getVideoGenerateTime(), o1.getVideoGenerateTime()));
        }
        AppConfig.getInstance().saveFolderSortType(type);
    }
}
