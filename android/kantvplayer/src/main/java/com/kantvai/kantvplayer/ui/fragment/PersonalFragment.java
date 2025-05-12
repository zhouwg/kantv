 /*
  * Copyright (c) Project KanTV. 2021-2023
  * Copyright (c) 2024- KanTV Authors
  */
 package com.kantvai.kantvplayer.ui.fragment;

 import android.annotation.SuppressLint;
 import android.app.Activity;
 import android.content.Context;
 import android.graphics.Color;
 import android.os.Bundle;
 import android.view.View;
 import android.view.ViewGroup;
 import android.webkit.WebChromeClient;
 import android.webkit.WebSettings;
 import android.webkit.WebView;
 import android.webkit.WebViewClient;

 import androidx.annotation.NonNull;
 import androidx.appcompat.app.AppCompatActivity;

 import com.blankj.utilcode.util.ConvertUtils;
 import com.kantvai.kantvplayer.ui.activities.personal.CustomEPGListActivity;
 import com.kantvai.kantvplayer.ui.fragment.settings.LLMSettingFragment;
 import com.kantvai.kantvplayer.ui.fragment.settings.SystemSettingFragment;
 import com.kantvai.kantvplayer.R;
 import com.kantvai.kantvplayer.base.BaseMvpFragment;
 import com.kantvai.kantvplayer.mvp.impl.PersonalFragmentPresenterImpl;
 import com.kantvai.kantvplayer.mvp.presenter.PersonalFragmentPresenter;
 import com.kantvai.kantvplayer.mvp.view.PersonalFragmentView;
 import com.kantvai.kantvplayer.ui.activities.ShellActivity;

 import com.kantvai.kantvplayer.ui.activities.personal.BenchmarkActivity;
 import com.kantvai.kantvplayer.ui.activities.personal.LocalPlayHistoryActivity;
 import com.kantvai.kantvplayer.ui.fragment.settings.PlaySettingFragment;
 import com.kantvai.kantvplayer.ui.fragment.settings.RecordSettingFragment;
 import com.kantvai.kantvplayer.ui.fragment.settings.ASRSettingFragment;
 import com.kantvai.kantvplayer.ui.weight.ProgressView;
 import com.kantvai.kantvplayer.utils.Settings;


 import butterknife.OnClick;
 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;


 public class PersonalFragment extends BaseMvpFragment<PersonalFragmentPresenter> implements PersonalFragmentView {
     private Activity mActivity;
     private Settings mSettings;
     private Context mContext;
     WebView mWebView;
     private ProgressView progressView;

     private static final String TAG = PersonalFragment.class.getName();

     public static PersonalFragment newInstance() {
         return new PersonalFragment();
     }

     @NonNull
     @Override
     protected PersonalFragmentPresenter initPresenter() {
         return new PersonalFragmentPresenterImpl(this, this);
     }

     @Override
     protected int initPageLayoutId() {
         return R.layout.fragment_personal;
     }

     @SuppressLint("ClickableViewAccessibility")
     @Override
     public void initView() {
         long beginTime = 0;
         long endTime = 0;
         beginTime = System.currentTimeMillis();

         mActivity = getActivity();
         mSettings = new Settings(mActivity.getBaseContext());
         mContext = mActivity.getBaseContext();
         mSettings.updateUILang((AppCompatActivity) getActivity());

         mWebView = (WebView) mActivity.findViewById(R.id.webview_ad);
         if (mWebView != null) {
             progressView = new ProgressView(mContext);
             progressView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ConvertUtils.dp2px(4)));
             progressView.setColor(Color.BLUE);
             progressView.setProgress(10);
             mWebView.addView(progressView);
             WebSettings webSetting = mWebView.getSettings();
             webSetting.setJavaScriptEnabled(true);
             webSetting.setCacheMode(android.webkit.WebSettings.LOAD_NO_CACHE);
             //webSetting.setAppCacheEnabled(true);
             String appCachePath = mActivity.getApplicationContext().getCacheDir().getAbsolutePath();
             //webSetting.setAppCachePath(appCachePath);
             webSetting.setDomStorageEnabled(true);
             //webSetting.setAppCacheEnabled(true);
             webSetting.setJavaScriptCanOpenWindowsAutomatically(true);
             webSetting.setLayoutAlgorithm(WebSettings.LayoutAlgorithm.SINGLE_COLUMN);
             webSetting.setAllowFileAccess(true);
             webSetting.setAllowUniversalAccessFromFileURLs(true);
             webSetting.setAllowFileAccessFromFileURLs(true);
             webSetting.setJavaScriptEnabled(true);
             webSetting.setJavaScriptCanOpenWindowsAutomatically(true);
             mWebView.setWebViewClient(mWebViewClient);
             mWebView.setWebChromeClient(mWebChromeClient);

             String aboutkantvUrl = "http://" + KANTVUtils.getKANTVMasterServer() + "/aboutkantv.html";
             if (mSettings.getUILang() == Settings.KANTV_UILANG_CN) {
                 aboutkantvUrl = "http://" + KANTVUtils.getKANTVMasterServer() + "/aboutkantv_zh.html";
             }
             mWebView.loadUrl(aboutkantvUrl);
         }
         endTime = System.currentTimeMillis();
         KANTVLog.j(TAG, "initView cost: " + (endTime - beginTime) + " milliseconds");
     }

     WebViewClient mWebViewClient = new WebViewClient() {

         @Override
         public boolean shouldOverrideUrlLoading(WebView view, String url) {
             view.loadUrl(url);
             return true;
         }
     };
     WebChromeClient mWebChromeClient = new WebChromeClient() {
         @Override
         public void onProgressChanged(WebView view, int newProgress) {
             if (newProgress == 100) {
                 progressView.setVisibility(View.GONE);
             } else {
                 progressView.setProgress(newProgress);
             }
             super.onProgressChanged(view, newProgress);
         }
     };


     @Override
     public void initListener() {

     }

     @OnClick({
             R.id.system_setting_ll, R.id.play_setting_ll, R.id.record_setting_ll,
             R.id.local_history_ll, R.id.llm_setting_ll, R.id.asr_setting_ll,
             R.id.peformance_benchmark_ll, R.id.custom_playlist_ll, R.id.exit_app_ll
     })
     public void onViewClicked(View view) {
         switch (view.getId()) {
             case R.id.system_setting_ll:
                 Bundle app = new Bundle();
                 app.putString("fragment", SystemSettingFragment.class.getName());
                 launchActivity(ShellActivity.class, app);
                 break;

             case R.id.play_setting_ll:
                 Bundle player = new Bundle();
                 player.putString("fragment", PlaySettingFragment.class.getName());
                 launchActivity(ShellActivity.class, player);
                 break;

             case R.id.local_history_ll:
                 launchActivity(LocalPlayHistoryActivity.class);
                 break;

             case R.id.record_setting_ll:
                 Bundle download = new Bundle();
                 download.putString("fragment", RecordSettingFragment.class.getName());
                 launchActivity(ShellActivity.class, download);
                 break;

             case R.id.llm_setting_ll:
                 Bundle llm = new Bundle();
                 llm.putString("fragment", LLMSettingFragment.class.getName());
                 launchActivity(ShellActivity.class, llm);
                 break;

             case R.id.asr_setting_ll:
                 Bundle asr = new Bundle();
                 asr.putString("fragment", ASRSettingFragment.class.getName());
                 launchActivity(ShellActivity.class, asr);
                 break;

             case R.id.peformance_benchmark_ll:
                 launchActivity(BenchmarkActivity.class);
                 break;

             case R.id.custom_playlist_ll:
                 launchActivity(CustomEPGListActivity.class);
                 break;

             case R.id.exit_app_ll:
                 KANTVUtils.exitAPK(mActivity);
                 break;
         }
     }


     @Override
     public void onDestroy() {
         super.onDestroy();
         KANTVLog.d(TAG, "on destroy");
         if (mWebView != null) {
             mWebView.loadDataWithBaseURL(null, "", "text/html", "utf-8", null);
             mWebView.clearHistory();
             ((ViewGroup) mWebView.getParent()).removeView(mWebView);
             mWebView.destroy();
             mWebView = null;
         }
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
