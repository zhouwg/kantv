package com.cdeos.kantv.ui.fragment;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.blankj.utilcode.util.ToastUtils;
import com.cdeos.kantv.R;
import com.cdeos.kantv.base.BaseMvpFragment;
import com.cdeos.kantv.base.BaseRvAdapter;
import com.cdeos.kantv.bean.ScanFolderBean;
import com.cdeos.kantv.mvp.impl.VideoScanFragmentPresenterImpl;
import com.cdeos.kantv.mvp.presenter.VideoScanFragmentPresenter;
import com.cdeos.kantv.mvp.view.VideoScanFragmentView;
import com.cdeos.kantv.ui.activities.setting.ScanManagerActivity;
import com.cdeos.kantv.ui.weight.item.VideoScanItem;
import com.cdeos.kantv.utils.interf.AdapterItem;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import butterknife.BindView;

//FIXME
public class VideoScanFragment extends BaseMvpFragment<VideoScanFragmentPresenter> implements VideoScanFragmentView {
    @BindView(R.id.folder_rv)
    RecyclerView folderRv;

    private boolean isScanType;

    private BaseRvAdapter<ScanFolderBean> adapter;
    private List<ScanFolderBean> folderList;
    private ScanManagerActivity.OnFragmentItemCheckListener itemCheckListener;
    private static final String TAG = VideoScanFragment.class.getName();

    public static VideoScanFragment newInstance(boolean isScanType){
        VideoScanFragment videoScanFragment = new VideoScanFragment();
        Bundle args = new Bundle();
        args.putSerializable("is_scan_type", isScanType);
        videoScanFragment.setArguments(args);
        return videoScanFragment;
    }

    public VideoScanFragment(){
        folderList = new ArrayList<>();
    }

    @NonNull
    @Override
    protected VideoScanFragmentPresenter initPresenter() {
        return new VideoScanFragmentPresenterImpl(this, this);
    }

    @Override
    protected int initPageLayoutId() {
        return R.layout.fragment_video_scan;
    }

    @Override
    public void initView() {
        Bundle args = getArguments();
        if (args == null) return;
        isScanType = getArguments().getBoolean("is_scan_type");

        adapter = new BaseRvAdapter<ScanFolderBean>(folderList) {
            @NonNull
            @Override
            public AdapterItem<ScanFolderBean> onCreateItem(int viewType) {
                return new VideoScanItem((isCheck, position) -> {
                    if (position < 0 || position >= folderList.size())
                        return;
                    folderList.get(position).setCheck(isCheck);
                    if (isCheck){
                        itemCheckListener.onChecked(true);
                    }else {
                        for (ScanFolderBean bean : folderList){
                            if (bean.isCheck()){
                                itemCheckListener.onChecked(true);
                                return;
                            }
                        }
                        itemCheckListener.onChecked(false);
                    }
                });
            }
        };
        folderRv.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
        folderRv.setNestedScrollingEnabled(false);
        folderRv.setItemViewCacheSize(10);
        folderRv.setAdapter(adapter);

        presenter.queryScanFolderList(isScanType);
    }

    public void updateFolderList(){
        presenter.queryScanFolderList(isScanType);
    }

    @Override
    public void initListener() {

    }

    public void addPath(String path){
        for (ScanFolderBean bean : folderList){
            if (path.contains(bean.getFolder())){
                ToastUtils.showShort("already in scanning");
                return;
            }
        }
        presenter.addScanFolder(path, isScanType);
    }

    public void deleteChecked(){
        Iterator iterator = folderList.iterator();
        while (iterator.hasNext()){
            ScanFolderBean bean = (ScanFolderBean) iterator.next();
            if (bean.isCheck()){
                presenter.deleteScanFolder(bean.getFolder(), isScanType);
                iterator.remove();
            }
        }
        adapter.notifyDataSetChanged();
    }

    public boolean hasChecked(){
        for (ScanFolderBean bean : folderList) {
            if (bean.isCheck()) {
                return true;
            }
        }
        return false;
    }

    public void setOnItemCheckListener(ScanManagerActivity.OnFragmentItemCheckListener itemCheckListener){
        this.itemCheckListener = itemCheckListener;
    }

    @Override
    public void updateFolderList(List<ScanFolderBean> folderList) {
        this.folderList.clear();
        this.folderList.addAll(folderList);
        adapter.notifyDataSetChanged();
    }
}
