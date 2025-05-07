package com.kantvai.kantvplayer.ui.activities.play;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import com.blankj.utilcode.util.FileUtils;
import com.blankj.utilcode.util.ToastUtils;
import com.kantvai.kantvplayer.R;
import com.kantvai.kantvplayer.base.BaseMvcActivity;
import com.kantvai.kantvplayer.bean.BindResourceBean;
import com.kantvai.kantvplayer.bean.params.BindResourceParam;
import com.kantvai.kantvplayer.bean.params.PlayParam;
import com.kantvai.kantvplayer.ui.fragment.TVGridFragment;
import com.kantvai.kantvplayer.utils.AppConfig;
import com.kantvai.kantvplayer.utils.CommonUtils;
import com.kantvai.kantvplayer.utils.database.DataBaseManager;

import java.io.File;

import butterknife.BindView;
import kantvai.media.player.KANTVLog;

public class PlayerManagerActivity extends BaseMvcActivity {
    private static final String TAG = PlayerManagerActivity.class.getName();
    private static final int SELECT_DANMU = 102;

    public static final int SOURCE_ORIGIN_OUTSIDE = 1000;
    public static final int SOURCE_ORIGIN_LOCAL = 1001;
    public static final int SOURCE_ORIGIN_STREAM = 1002;
    public static final int SOURCE_ORIGIN_SMB = 1003;
    public static final int SOURCE_ORIGIN_REMOTE = 1004;
    public static final int SOURCE_ONLINE_PREVIEW = 1005;

    private String videoPath;
    private String videoTitle;
    private String zimuPath;
    private long currentPosition;
    private int episodeId;
    private int sourceOrigin;
    private String searchWord;

    @BindView(R.id.error_tv)
    TextView errorTv;
    @BindView(R.id.back_iv)
    ImageView backIv;

    @Override
    protected int initPageLayoutID() {
        return R.layout.activity_player_manager;
    }

    @Override
    public void initPageView() {

    }

    @Override
    public void initPageViewListener() {
        View decorView = this.getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );
        this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

        errorTv.setVisibility(View.GONE);
        backIv.setOnClickListener(v -> PlayerManagerActivity.this.finish());

        initIntent();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == SELECT_DANMU) {
            if (resultCode == RESULT_OK) {
                BindResourceBean bindResourceBean = data.getParcelableExtra("bind_data");
                if (bindResourceBean != null) {
                    episodeId = bindResourceBean.getEpisodeId();
                }
            }
            if (TextUtils.isEmpty(videoPath)) {
                ToastUtils.showShort("parse url of video failure");
                errorTv.setVisibility(View.VISIBLE);
            } else {
                launchPlayerActivity();
            }
        }
    }

    private void initIntent() {
        Intent openIntent = getIntent();
        videoTitle = openIntent.getStringExtra("video_title");
        videoPath = openIntent.getStringExtra("video_path");
        zimuPath = openIntent.getStringExtra("zimu_path");
        currentPosition = openIntent.getLongExtra("current_position", 0L);
        episodeId = openIntent.getIntExtra("episode_id", 0);
        searchWord = openIntent.getStringExtra("search_word");

        sourceOrigin = openIntent.getIntExtra("source_origin", SOURCE_ORIGIN_LOCAL);

        KANTVLog.d(TAG, "video title:" + videoTitle);
        KANTVLog.d(TAG, "video url:" + videoPath);


        if (Intent.ACTION_VIEW.equals(openIntent.getAction())) {
            sourceOrigin = SOURCE_ORIGIN_OUTSIDE;
            Uri data = getIntent().getData();
            if (data != null) {
                videoPath = CommonUtils.getRealFilePath(PlayerManagerActivity.this, data);
            }
        }

        if (TextUtils.isEmpty(videoPath)) {
            ToastUtils.showShort("parse url of video failure:empty url");
            errorTv.setVisibility(View.VISIBLE);
            return;
        }


        if (!TextUtils.isEmpty(zimuPath)) {
            File zimuFile = new File(zimuPath);
            if (!zimuFile.exists() || !zimuFile.isFile()) {
                zimuPath = "";
            }
        }

        videoTitle = TextUtils.isEmpty(videoTitle) ? FileUtils.getFileName(videoPath) : videoTitle;
        searchWord = TextUtils.isEmpty(searchWord) ? FileUtils.getFileNameNoExtension(videoPath) : searchWord;

        launchPlayerActivity();
    }


    private void launchPlayerActivity() {
        saveDatabase();

        PlayParam playParam = new PlayParam();
        playParam.setVideoTitle(videoTitle);
        playParam.setVideoPath(videoPath);
        playParam.setZimuPath(zimuPath);
        playParam.setEpisodeId(episodeId);
        playParam.setCurrentPosition(currentPosition);
        playParam.setSourceOrigin(sourceOrigin);


        KANTVLog.d(TAG, "start player activity");
        Intent intent = new Intent(this, PlayerActivity.class);
        intent.putExtra("video_data", playParam);
        this.startActivity(intent);
        PlayerManagerActivity.this.finish();
    }


    public void saveDatabase() {
        DataBaseManager.getInstance()
                .selectTable("local_play_history")
                .query()
                .where("video_path", videoPath)
                .where("source_origin", String.valueOf(sourceOrigin))
                .postExecute(cursor -> {
                    if (cursor.getCount() > 0) {
                        DataBaseManager.getInstance()
                                .selectTable("local_play_history")
                                .update()
                                .param("video_title", videoTitle)
                                .param("zimu_path", zimuPath)
                                .param("episode_id", episodeId)
                                .param("play_time", System.currentTimeMillis())
                                .where("video_path", videoPath)
                                .where("source_origin", String.valueOf(sourceOrigin))
                                .executeAsync();
                    } else {
                        DataBaseManager.getInstance()
                                .selectTable("local_play_history")
                                .insert()
                                .param("video_path", videoPath)
                                .param("video_title", videoTitle)
                                .param("zimu_path", zimuPath)
                                .param("episode_id", episodeId)
                                .param("source_origin", sourceOrigin)
                                .param("play_time", System.currentTimeMillis())
                                .executeAsync();
                    }
                });
    }


    public static void launchPlayerLocal(Context context, String title, String path, String zimu, long position, int episodeId) {
        Intent intent = new Intent(context, PlayerManagerActivity.class);
        intent.putExtra("video_title", title);
        intent.putExtra("video_path", path);
        intent.putExtra("zimu_path", zimu);
        intent.putExtra("current_position", position);
        intent.putExtra("episode_id", episodeId);
        intent.putExtra("source_origin", SOURCE_ORIGIN_LOCAL);
        context.startActivity(intent);
    }


    public static void launchPlayerStream(Context context, String title, String path, long position, int episodeId) {
        Intent intent = new Intent(context, PlayerManagerActivity.class);
        intent.putExtra("video_title", title);
        intent.putExtra("video_path", path);
        intent.putExtra("current_position", position);
        intent.putExtra("episode_id", episodeId);
        intent.putExtra("source_origin", SOURCE_ORIGIN_STREAM);
        context.startActivity(intent);
    }


    public static void launchPlayerRemote(Context context, String title, String path, long position, int episodeId) {
        Intent intent = new Intent(context, PlayerManagerActivity.class);
        intent.putExtra("video_title", title);
        intent.putExtra("video_path", path);
        intent.putExtra("current_position", position);
        intent.putExtra("episode_id", episodeId);
        intent.putExtra("source_origin", SOURCE_ORIGIN_REMOTE);
        context.startActivity(intent);
    }


    public static void launchPlayerOnline(Context context, String title, String path, long position, int episodeId,  String searchWord) {
        Intent intent = new Intent(context, PlayerManagerActivity.class);
        intent.putExtra("video_title", title);
        intent.putExtra("video_path", path);
        intent.putExtra("current_position", position);
        intent.putExtra("episode_id", episodeId);
        intent.putExtra("search_word", searchWord);
        intent.putExtra("source_origin", SOURCE_ONLINE_PREVIEW);
        context.startActivity(intent);
    }


    public static void launchPlayerHistory(Context context, String title, String path,String zimu, long position, int episodeId, int sourceOrigin) {
        Intent intent = new Intent(context, PlayerManagerActivity.class);
        intent.putExtra("video_title", title);
        intent.putExtra("video_path", path);
        intent.putExtra("zimu_path", zimu);
        intent.putExtra("current_position", position);
        intent.putExtra("episode_id", episodeId);
        intent.putExtra("source_origin", sourceOrigin);
        context.startActivity(intent);
    }

}
