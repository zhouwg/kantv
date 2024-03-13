package com.cdeos.kantv.ui.activities.personal;

import android.content.Context;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.flyco.tablayout.CommonTabLayout;
import com.flyco.tablayout.listener.CustomTabEntity;
import com.flyco.tablayout.listener.OnTabSelectListener;
import com.cdeos.kantv.R;
import com.cdeos.kantv.base.BaseMvpActivity;
import com.cdeos.kantv.mvp.impl.ScanManagerPresenterImpl;
import com.cdeos.kantv.mvp.presenter.ScanManagerPresenter;
import com.cdeos.kantv.mvp.view.ScanManagerView;
import com.cdeos.kantv.ui.fragment.VideoScanFragment;
import com.cdeos.kantv.ui.weight.dialog.FileManagerDialog;
import com.cdeos.kantv.utils.CommonUtils;
import com.cdeos.kantv.utils.TabEntity;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.OnClick;



public class ScanManagerActivity extends BaseMvpActivity<ScanManagerPresenter> implements ScanManagerView {

    @BindView(R.id.tab_layout)
    CommonTabLayout tabLayout;
    @BindView(R.id.viewpager)
    ViewPager viewPager;
    @BindView(R.id.delete_tv)
    TextView deleteTv;

    private List<VideoScanFragment> fragmentList;
    private int selectedPosition = 0;

    @Override
    public void initView() {
        setTitle("本地视频扫描管理");
        fragmentList = new ArrayList<>();

        ScanManagerActivity.OnFragmentItemCheckListener itemCheckListener = hasChecked -> {
            if (hasChecked) {
                deleteTv.setTextColor(CommonUtils.getResColor(R.color.immutable_text_theme));
                deleteTv.setClickable(true);
            } else {
                deleteTv.setTextColor(CommonUtils.getResColor(R.color.text_gray));
                deleteTv.setClickable(false);
            }
        };

        VideoScanFragment scanFragment = VideoScanFragment.newInstance(true);
        VideoScanFragment blockFragment = VideoScanFragment.newInstance(false);
        scanFragment.setOnItemCheckListener(itemCheckListener);
        blockFragment.setOnItemCheckListener(itemCheckListener);
        fragmentList.add(scanFragment);
        fragmentList.add(blockFragment);

        initTabLayout();

        initViewPager();
    }

    @Override
    public void initListener() {

    }

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_video_scan;
    }

    @NonNull
    @Override
    protected ScanManagerPresenter initPresenter() {
        return new ScanManagerPresenterImpl(this, this);
    }

    private void initTabLayout() {
        ArrayList<CustomTabEntity> mTabEntities = new ArrayList<>();
        mTabEntities.add(new TabEntity("扫描目录", 0, 0));
        mTabEntities.add(new TabEntity("屏蔽目录", 0, 0));
        tabLayout.setTabData(mTabEntities);
        tabLayout.setCurrentTab(selectedPosition);

        tabLayout.setOnTabSelectListener(new OnTabSelectListener() {
            @Override
            public void onTabSelect(int position) {
                viewPager.setCurrentItem(position);
            }

            @Override
            public void onTabReselect(int position) {

            }
        });
    }

    private void initViewPager() {
        VideoScanFragmentAdapter fragmentAdapter = new VideoScanFragmentAdapter(getSupportFragmentManager(), fragmentList);
        viewPager.setAdapter(fragmentAdapter);
        viewPager.setCurrentItem(selectedPosition);
        viewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {

            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
            }

            @Override
            public void onPageSelected(int position) {
                tabLayout.setCurrentTab(position);
                selectedPosition = position;
                fragmentList.get(position).updateFolderList();
                resetButtonStatus();
            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });
    }

    @OnClick({R.id.scan_folder_tv, R.id.scan_file_tv, R.id.delete_tv, R.id.add_tv})
    public void onViewClicked(View view) {
        switch (view.getId()) {
            case R.id.scan_folder_tv:
                new FileManagerDialog(ScanManagerActivity.this, FileManagerDialog.SELECT_FOLDER,
                        path -> presenter.listFolder(path)
                ).show();
                break;
            case R.id.scan_file_tv:
                new FileManagerDialog(ScanManagerActivity.this, FileManagerDialog.SELECT_VIDEO, path -> {
                    List<String> pathList = new ArrayList<>();
                    pathList.add(path);
                    presenter.saveNewVideo(pathList);
                }).show();
                break;
            case R.id.delete_tv:
                fragmentList.get(selectedPosition).deleteChecked();
                resetButtonStatus();
                break;
            case R.id.add_tv:
                new FileManagerDialog(ScanManagerActivity.this, FileManagerDialog.SELECT_FOLDER, path ->
                        fragmentList.get(selectedPosition).addPath(path)
                ).show();
                break;
        }
    }

    private void resetButtonStatus() {
        VideoScanFragment videoScanFragment = fragmentList.get(selectedPosition);
        if (videoScanFragment.hasChecked()) {
            deleteTv.setTextColor(CommonUtils.getResColor(R.color.immutable_text_theme));
            deleteTv.setClickable(true);
        } else {
            deleteTv.setTextColor(CommonUtils.getResColor(R.color.text_gray));
            deleteTv.setClickable(false);
        }
    }

    @Override
    public Context getContext() {
        return this;
    }

    public interface OnFragmentItemCheckListener {
        void onChecked(boolean hasChecked);
    }

    private class VideoScanFragmentAdapter extends FragmentPagerAdapter {
        private List<VideoScanFragment> list;

        private VideoScanFragmentAdapter(FragmentManager supportFragmentManager, List<VideoScanFragment> list) {
            super(supportFragmentManager);
            this.list = list;
        }

        @Override
        public Fragment getItem(int position) {
            return list.get(position);
        }

        @Override
        public int getCount() {
            return list.size();
        }
    }
}
