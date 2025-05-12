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
package com.kantvai.kantvplayer.ui.activities;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.graphics.Color;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.appcompat.widget.Toolbar;

import com.blankj.utilcode.util.ConvertUtils;
import com.blankj.utilcode.util.StringUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvcActivity;
import com.kantvai.kantvplayer.ui.weight.ProgressView;

import butterknife.BindView;


public class WebViewActivity extends BaseMvcActivity {
    @BindView(R.id.web_view)
    WebView mWebView;
    @BindView(R.id.toolbar)
    Toolbar toolbar;

    private ProgressView progressView;
    private boolean isSelectUrl = false;

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_webview;
    }

    @Override
    public void initPageView() {
        Intent intent = getIntent();
        String title = intent.getStringExtra("title");
        String link = intent.getStringExtra("link");
        isSelectUrl = intent.getBooleanExtra("isSelect", false);

        setTitle(title);

        initView();

        initWebSetting();

        mWebView.setWebViewClient(mWebViewClient);
        mWebView.setWebChromeClient(mWebChromeClient);
        mWebView.loadUrl(link);
    }

    @Override
    public void initPageViewListener() {

    }

    private void initView() {
        progressView = new ProgressView(this);
        progressView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ConvertUtils.dp2px(4)));
        progressView.setColor(Color.BLUE);
        progressView.setProgress(10);
        mWebView.addView(progressView);
    }

    @SuppressLint("SetJavaScriptEnabled")
    private void initWebSetting(){
        WebSettings webSetting = mWebView.getSettings();
        webSetting.setJavaScriptEnabled(true);
        webSetting.setCacheMode(android.webkit.WebSettings.LOAD_NO_CACHE);
        //webSetting.setAppCacheEnabled(true);
        String appCachePath = this.getApplicationContext().getCacheDir().getAbsolutePath();
        //webSetting.setAppCachePath(appCachePath);
        webSetting.setDomStorageEnabled(true);
        //webSetting.setAppCacheEnabled(true);
        webSetting.setJavaScriptCanOpenWindowsAutomatically(true);
        webSetting.setLayoutAlgorithm(WebSettings.LayoutAlgorithm.SINGLE_COLUMN);
        webSetting.setAllowFileAccess(true);
        webSetting.setAllowUniversalAccessFromFileURLs(true);
        webSetting.setAllowFileAccessFromFileURLs(true);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mWebView.canGoBack()) {
                mWebView.goBack();
                return true;
            } else {
                finish();
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                WebViewActivity.this.finish();
                break;
            case R.id.select_url:
                if (!StringUtils.isEmpty(mWebView.getUrl())) {
                    Intent intent = getIntent();
                    intent.putExtra("selectUrl", mWebView.getUrl());
                    setResult(RESULT_OK, intent);
                    WebViewActivity.this.finish();
                } else {
                    ToastUtils.showShort("url is not empty");
                }
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (isSelectUrl)
            getMenuInflater().inflate(R.menu.menu_url_select, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mWebView != null) {
            mWebView.loadDataWithBaseURL(null, "", "text/html", "utf-8", null);
            mWebView.clearHistory();
            ((ViewGroup) mWebView.getParent()).removeView(mWebView);
            mWebView.destroy();
            mWebView = null;
        }
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

    public static native int kantv_anti_remove_rename_this_file();
}
