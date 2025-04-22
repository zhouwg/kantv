package com.kantvai.kantvplayer.ui.weight.item;

import android.annotation.SuppressLint;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.StringUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.app.IApplication;
import com.kantvai.kantvplayer.bean.VideoBean;
import com.kantvai.kantvplayer.ui.fragment.settings.PlaySettingFragment;
import com.kantvai.kantvplayer.ui.weight.swipe_menu.EasySwipeMenuLayout;
import com.kantvai.kantvplayer.utils.AppConfig;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.interf.AdapterItem;

import java.io.File;

import butterknife.BindView;
import kantvai.media.player.KANTVLog;
import io.reactivex.Observable;
import io.reactivex.ObservableOnSubscribe;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;

public class VideoItem implements AdapterItem<VideoBean> {

    @BindView(R.id.cover_iv)
    ImageView coverIv;
    @BindView(R.id.duration_tv)
    TextView durationTv;
    @BindView(R.id.title_tv)
    TextView titleTv;
    @BindView(R.id.item_layout)
    ConstraintLayout itemLayout;

    @BindView(R.id.delete_video_tv)
    TextView deleteVideoTv;

    @BindView(R.id.zimu_tips_tv)
    TextView zimuTipsTv;
    @BindView(R.id.swipe_menu_layout)
    EasySwipeMenuLayout swipeMenuLayout;
    @BindView(R.id.remove_zimu_tv)
    TextView removeZimuTv;

    private View mView;
    private VideoItemEventListener listener;
    private static final String TAG = VideoItem.class.getName();
    public VideoItem(VideoItemEventListener listener) {
        this.listener = listener;
    }

    @Override
    public int getLayoutResId() {
        return R.layout.item_video;
    }

    @Override
    public void initItemViews(View itemView) {
        mView = itemView;
    }

    @Override
    public void onSetViews() {

    }

    @SuppressLint("CheckResult")
    @Override
    public void onUpdateViews(final VideoBean model, final int position) {
        coverIv.setScaleType(ImageView.ScaleType.FIT_XY);
        if (!model.isNotCover()) {
            Observable
                    .create((ObservableOnSubscribe<Bitmap>) emitter ->
                            emitter.onNext(getBitmap(model.get_id())))
                    .subscribeOn(Schedulers.io())
                    .observeOn(AndroidSchedulers.mainThread())
                    .subscribe(bitmap -> coverIv.setImageBitmap(bitmap));
        } else {
            coverIv.setImageBitmap(BitmapFactory.decodeResource(mView.getResources(), R.mipmap.ic_smb_video));
        }


        boolean isLastPlayVideo = false;
        String lastVideoPath = AppConfig.getInstance().getLastPlayVideo();
        if (!StringUtils.isEmpty(lastVideoPath)) {
            isLastPlayVideo = lastVideoPath.equals(model.getVideoPath());
        }

        boolean isBoundZimu = isBoundZimu(model.getZimuPath());

        zimuTipsTv.setVisibility(isBoundZimu ? View.VISIBLE : View.GONE);
        removeZimuTv.setVisibility(isBoundZimu ? View.VISIBLE : View.GONE);
        swipeMenuLayout.setRightSwipeEnable(isBoundZimu);
        //swipeMenuLayout.setRightSwipeEnable(false);

        titleTv.setText(FileUtils.getFileNameNoExtension(model.getVideoPath()));
        titleTv.setTextColor(isLastPlayVideo
                ? CommonUtils.getResColor(R.color.immutable_text_theme)
                : CommonUtils.getResColor(R.color.text_black));

        durationTv.setText(CommonUtils.formatDuring(model.getVideoDuration()));
        if (model.getVideoDuration() == 0) durationTv.setVisibility(View.GONE);

        /*
        bindZimuTv.setOnClickListener(v -> listener.onBindZimu(position));
        removeZimuTv.setOnClickListener(v -> listener.onRemoveZimu(position));
         */
        deleteVideoTv.setOnClickListener(v -> listener.onVideoDelete(position));

        itemLayout.setOnClickListener(v -> listener.onOpenVideo(position));


    }

    private Bitmap getBitmap(int _id) {
        Bitmap bitmap;
        KANTVLog.j(TAG, "video cover id:" + _id);
        if (_id == 0) {
            bitmap = Bitmap.createBitmap(200, 200, Bitmap.Config.RGB_565);
            bitmap.eraseColor(CommonUtils.getResColor(R.color.video_item_image_default_color));
        } else {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inDither = false;
            options.inPreferredConfig = Bitmap.Config.RGB_565;
            bitmap = MediaStore.Video.Thumbnails.getThumbnail(IApplication.get_context().getContentResolver(), _id, MediaStore.Images.Thumbnails.MICRO_KIND, options);
        }
        if (bitmap == null) {
            bitmap = Bitmap.createBitmap(200, 200, Bitmap.Config.RGB_565);
            bitmap.eraseColor(CommonUtils.getResColor(R.color.video_item_image_default_color));
        }
        return bitmap;
    }

    private boolean isBoundDanmu(String danmuPath) {
        if (!TextUtils.isEmpty(danmuPath)) {
            File danmuFile = new File(danmuPath);
            return danmuFile.exists();
        }
        return false;
    }

    private boolean isBoundZimu(String zimuPath) {
        if (!TextUtils.isEmpty(zimuPath)) {
            File zimuFile = new File(zimuPath);
            return zimuFile.exists();
        }
        return false;
    }

    public interface VideoItemEventListener {
        void onVideoDelete(int position);

        void onOpenVideo(int position);
    }
}
