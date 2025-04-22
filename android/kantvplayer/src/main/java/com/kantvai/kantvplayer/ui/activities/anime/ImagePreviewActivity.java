package com.kantvai.kantvplayer.ui.activities.anime;

import android.graphics.Bitmap;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.blankj.utilcode.util.StringUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.bitmap.BitmapTransitionOptions;
import com.bumptech.glide.load.resource.drawable.DrawableTransitionOptions;
import com.bumptech.glide.request.target.SimpleTarget;
import com.bumptech.glide.request.transition.Transition;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.base.BaseMvcActivity;
import com.kantvai.kantvplayer.ui.weight.dialog.CommonDialog;
import com.kantvai.kantvplayer.ui.weight.preview.ImageViewTouch;
import com.kantvai.kantvplayer.ui.weight.preview.ImageViewTouchBase;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.Constants;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import butterknife.BindView;
import butterknife.OnClick;


public class ImagePreviewActivity extends BaseMvcActivity {

    @BindView(R.id.preview_iv)
    ImageViewTouch previewIv;
    @BindView(R.id.back_iv)
    ImageView backIv;

    private String originUrl;

    private Runnable hideViewRunnable = () -> switchScreenStatus(true);

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_image_preview;
    }

    @Override
    public void initPageView() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);

        originUrl = getIntent().getStringExtra("image_url");

        if (!StringUtils.isEmpty(originUrl)) {
            previewIv.setDisplayType(ImageViewTouchBase.DisplayType.FIT_TO_SCREEN);
            Glide.with(this)
                    .load(originUrl)
                    .transition((DrawableTransitionOptions.withCrossFade()))
                    .into(previewIv);
        }

        switchScreenStatus(false);
        IApplication.getMainHandler().removeCallbacks(hideViewRunnable);
        IApplication.getMainHandler().postDelayed(hideViewRunnable, 2000);

        previewIv.setSingleTapListener(() -> {
            switchScreenStatus(false);
            IApplication.getMainHandler().removeCallbacks(hideViewRunnable);
            IApplication.getMainHandler().postDelayed(hideViewRunnable, 2000);
        });

        previewIv.setOnLongClickListener(v -> {
            new CommonDialog.Builder(ImagePreviewActivity.this)
                    .setAutoDismiss()
                    .setOkListener(dialog ->
                            Glide.with(this)
                               .asBitmap()
                               .load(originUrl)
                               .transition((BitmapTransitionOptions.withCrossFade()))
                               .into(new SimpleTarget<Bitmap>() {
                                   @Override
                                   public void onResourceReady(@NonNull Bitmap resource, @Nullable Transition<? super Bitmap> transition) {
                                       saveBitmap(resource);
                                   }
                               }))
                    .build()
                    .show("confirm download image file?", "OK", "Cancel");

            return true;
        });
    }

    @Override
    public void initPageViewListener() {

    }

    @OnClick({R.id.back_iv})
    public void onViewClicked(View view) {
        if (view.getId() == R.id.back_iv) {
            ImagePreviewActivity.this.finish();
        }
    }

    private void switchScreenStatus(boolean isFullScreen){
        if (backIv == null) return;
        if (isFullScreen){
            WindowManager.LayoutParams attrs = getWindow().getAttributes();
            attrs.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;
            getWindow().setAttributes(attrs);
            backIv.setVisibility(View.GONE);
        }else {
            WindowManager.LayoutParams attrs = getWindow().getAttributes();
            attrs.flags &= ~WindowManager.LayoutParams.FLAG_FULLSCREEN;
            getWindow().setAttributes(attrs);
            backIv.setVisibility(View.VISIBLE);
        }
    }

    private void saveBitmap(Bitmap bitmap){
        try {
            //make folder
            File folder = new File(Constants.DefaultConfig.imageFolder);
            if (!folder.exists())
                folder.mkdirs();

            //make file
            File coverFile = new File(folder,  CommonUtils.getCurrentFileName("COV", ".jpg"));
            if (coverFile.exists()) {
                coverFile.delete();
            }
            coverFile.createNewFile();

            //save
            FileOutputStream fos = new FileOutputStream(coverFile);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, fos);
            fos.flush();
            fos.close();
            ToastUtils.showLong("save succesful："+coverFile.getAbsolutePath());
        } catch (IOException e) {
            e.printStackTrace();
            ToastUtils.showShort("save failure");
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        IApplication.getMainHandler().removeCallbacks(hideViewRunnable);
    }
}
