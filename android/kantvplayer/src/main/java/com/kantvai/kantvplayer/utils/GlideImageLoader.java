package com.kantvai.kantvplayer.utils;

import android.content.Context;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.drawable.DrawableTransitionOptions;
import com.kantvai.kantvplayer.ui.weight.RoundImageView;
import com.youth.banner.loader.ImageLoader;

public class GlideImageLoader extends ImageLoader {

    @Override
    public void displayImage(Context context, Object path, ImageView imageView) {
        Glide.with(context)
                .load(path)
                .transition((DrawableTransitionOptions.withCrossFade()))
                .into(imageView);
    }

    @Override
    public ImageView createImageView(Context context) {
        return new RoundImageView(context);
    }
}
