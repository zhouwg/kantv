package com.kantvai.kantvplayer.base;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.kantvai.kantvplayer.utils.interf.IBaseView;

import butterknife.ButterKnife;
import butterknife.Unbinder;

/**
 * Fragment ---------------- Activity
 * ----------------------------------
 * onInflate()              | before Create
 * ----------------------------------
 * onAttach()               |
 * onCreate()               |
 * onCreateView()           | Created
 * onViewCreated()          |
 * onActivityCreated()      |
 * ----------------------------------
 * onStart()                | Started
 * ----------------------------------
 * onResume()               | Resumed
 * ----------------------------------
 * onPause()                | Paused
 * ----------------------------------
 * onStop()                 | Stopped
 * ----------------------------------
 * onDestroyView()          |
 * onDestroy()              | Destroyed
 * onDetach()               |
 * ----------------------------------
 */
public abstract class BaseAppFragment extends Fragment implements IBaseView {
    protected boolean isActivityCreated = false;

    public boolean isFirstVisible = true;

    protected View mFragmentView;

    protected int position;

    protected boolean mIsVisibleToUser = false;

    protected BaseAppCompatActivity mContext;

    private Unbinder unbind;

    public void setPosition(int position) {
        this.position = position;
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        mIsVisibleToUser = isVisibleToUser;
        if (isActivityCreated) {
            if (isVisibleToUser) {
                if (isFirstVisible) {
                    isFirstVisible = false;
                    onPageFirstVisible();
                }
                onPageStart();
            } else {
                onPageEnd();
            }
        }
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        init();
    }

    protected void init() {
        mContext = (BaseAppCompatActivity) getActivity();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(initPageLayoutId(), container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        mFragmentView = view;
        unbind = ButterKnife.bind(this, mFragmentView);
        initPageView();
        initPageViewListener();
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        isActivityCreated = true;
        if (getUserVisibleHint() || mIsVisibleToUser) {
            if (isFirstVisible) {
                isFirstVisible = false;
                onPageFirstVisible();
                onPageStart();
            }
        }
        process(savedInstanceState);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        unbind.unbind();
        mFragmentView = null;
    }

    @Override
    public void onDetach() {
        super.onDetach();
    }

    @LayoutRes
    protected abstract int initPageLayoutId();

    @Override
    public boolean getUserVisibleHint() {
        return super.getUserVisibleHint();
    }

    protected void onPageFirstVisible() {
    }

    protected void process(Bundle savedInstanceState) {
    }

    protected void onPageStart() {
        lazyLoad();
    }

    protected void onPageEnd() {
    }

    protected void lazyLoad() {

    }

    protected void launchActivity(Class<? extends Activity> cls) {
        launchActivity(cls, null);
    }

    protected void launchActivity(Class<? extends Activity> cls, @Nullable Bundle bundle) {
        Intent intent = new Intent(this.getActivity(), cls);
        if (bundle != null)
            intent.putExtras(bundle);
        startActivity(intent);
    }

    protected void launchActivity(Class<? extends Activity> cls, @Nullable Bundle bundle, Integer flag) {
        Intent intent = new Intent(this.getActivity(), cls);
        if (flag != null) {
            intent.setFlags(flag);
        }
        if (bundle != null)
            intent.putExtras(bundle);
        startActivity(intent);
    }

    @Override
    public void initData() {

    }

    public BaseAppCompatActivity getBaseActivity() {
        return mContext != null ? mContext : (BaseAppCompatActivity) this.getContext();
    }

    public boolean activityIsAlive() {
        Activity currentActivity = getActivity();
        if (currentActivity == null) return false;
        return !(currentActivity.isDestroyed() || currentActivity.isFinishing());
    }
}
