package com.kantvai.kantvplayer.ui.activities.personal;

import android.database.Cursor;
import android.view.Menu;
import android.view.MenuItem;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.blankj.utilcode.util.ConvertUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvcActivity;
import com.kantvai.kantvplayer.base.BaseRvAdapter;
import com.kantvai.kantvplayer.bean.LocalPlayHistoryBean;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;
import com.kantvai.kantvplayer.ui.weight.ItemDecorationDivider;
import com.kantvai.kantvplayer.ui.weight.item.LocalPlayHistoryItem;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.database.callback.QueryAsyncResultCallback;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import butterknife.BindView;


public class LocalPlayHistoryActivity extends BaseMvcActivity {
    @BindView(R.id.rv)
    RecyclerView recyclerView;

    private MenuItem menuDeleteCheckedItem, menuDeleteCancelItem, menuDeleteAllItem;

    private BaseRvAdapter<LocalPlayHistoryBean> adapter;
    private List<LocalPlayHistoryBean> historyList;

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_local_play_history;
    }

    @Override
    public void initPageView() {
        setTitle("Playback history");

        historyList = new ArrayList<>();

        adapter = new BaseRvAdapter<LocalPlayHistoryBean>(historyList) {
            @NonNull
            @Override
            public AdapterItem<LocalPlayHistoryBean> onCreateItem(int viewType) {
                return new LocalPlayHistoryItem(new LocalPlayHistoryItem.OnLocalHistoryItemClickListener() {
                    @Override
                    public boolean onLongClick(int position) {
                        for (LocalPlayHistoryBean historyBean : historyList) {
                            historyBean.setDeleteMode(true);
                        }
                        setTitle("delete playback history");
                        menuDeleteCheckedItem.setVisible(true);
                        menuDeleteCancelItem.setVisible(true);
                        menuDeleteAllItem.setVisible(false);
                        historyList.get(position).setChecked(true);
                        adapter.notifyDataSetChanged();
                        return true;
                    }

                    @Override
                    public void onCheckedChanged(int position) {
                        boolean isChecked = historyList.get(position).isChecked();
                        historyList.get(position).setChecked(!isChecked);
                        adapter.notifyItemChanged(position);
                    }
                });
            }
        };

        recyclerView.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.VERTICAL, false));
        recyclerView.addItemDecoration(
                new ItemDecorationDivider(
                        ConvertUtils.dp2px(1),
                        CommonUtils.getResColor(R.color.layout_bg_color),
                        1)
        );
        recyclerView.setAdapter(adapter);

        queryHistory();
    }

    private void queryHistory(){
        //查询记录
        DataBaseManager.getInstance()
                .selectTable("local_play_history")
                .query()
                .setOrderByColumnDesc("play_time")
                .postExecute(new QueryAsyncResultCallback<List<LocalPlayHistoryBean>>(this) {

                    @Override
                    public List<LocalPlayHistoryBean> onQuery(Cursor cursor) {
                        if (cursor == null)
                            return new ArrayList<>();
                        List<LocalPlayHistoryBean> list = new ArrayList<>();
                        while (cursor.moveToNext()) {
                            LocalPlayHistoryBean historyBean = new LocalPlayHistoryBean();
                            historyBean.setVideoPath(cursor.getString(1));
                            historyBean.setVideoTitle(cursor.getString(2));
                            historyBean.setEpisodeId(cursor.getInt(3));
                            historyBean.setSourceOrigin(cursor.getInt(4));
                            historyBean.setPlayTime(cursor.getLong(5));
                            historyBean.setZimuPath(cursor.getString(6));
                            list.add(historyBean);
                        }
                        return list;
                    }

                    @Override
                    public void onResult(List<LocalPlayHistoryBean> result) {
                        historyList.clear();
                        if (result != null){
                            historyList.addAll(result);
                        }
                        adapter.notifyDataSetChanged();
                    }
                });
    }

    @Override
    public void initPageViewListener() {

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            case R.id.item_delete_all:
                DataBaseManager.getInstance()
                        .selectTable("local_play_history")
                        .delete()
                        .postExecute();
                historyList.clear();
                adapter.notifyDataSetChanged();
                break;
            case R.id.item_delete_cancel:
                for (LocalPlayHistoryBean historyBean : historyList) {
                    historyBean.setDeleteMode(false);
                    historyBean.setChecked(false);
                }
                setTitle("Playback history");
                menuDeleteCheckedItem.setVisible(false);
                menuDeleteCancelItem.setVisible(false);
                menuDeleteAllItem.setVisible(true);
                adapter.notifyDataSetChanged();
                break;
            case R.id.item_delete_checked:
                Iterator iterator = historyList.iterator();
                boolean isRemove = false;
                while (iterator.hasNext()) {
                    LocalPlayHistoryBean historyBean = (LocalPlayHistoryBean) iterator.next();
                    if (historyBean.isChecked()) {
                        DataBaseManager.getInstance()
                                .selectTable("local_play_history")
                                .delete()
                                .where("video_path", historyBean.getVideoPath())
                                .where("source_origin", String.valueOf(historyBean.getSourceOrigin()))
                                .postExecute();
                        iterator.remove();
                        isRemove = true;
                    }
                }
                if (isRemove) {
                    adapter.notifyDataSetChanged();
                } else {
                    ToastUtils.showShort("history item was not selected");
                }
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_local_play_history, menu);
        menuDeleteCheckedItem = menu.findItem(R.id.item_delete_checked);
        menuDeleteCancelItem = menu.findItem(R.id.item_delete_cancel);
        menuDeleteAllItem = menu.findItem(R.id.item_delete_all);
        menuDeleteCheckedItem.setVisible(false);
        menuDeleteCancelItem.setVisible(false);
        menuDeleteAllItem.setVisible(true);
        return super.onCreateOptionsMenu(menu);
    }
}
