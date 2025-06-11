/*
 * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
 *
 * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
 */
 package kantvai.media.exo2;


 import android.content.Context;

 import android.media.AudioTrack;
 import android.media.MediaCodec;

 import android.net.Uri;
 import android.os.Build;
 import android.util.Pair;

 import android.view.Surface;
 import android.widget.Toast;

 import androidx.annotation.NonNull;
 import androidx.annotation.RequiresApi;
 import androidx.appcompat.app.AppCompatActivity;

 import com.google.android.exoplayer2.C;
 import com.google.android.exoplayer2.DefaultLivePlaybackSpeedControl;
 import com.google.android.exoplayer2.DefaultRenderersFactory;
 import com.google.android.exoplayer2.ExoPlaybackException;
 import com.google.android.exoplayer2.ExoPlayer;
 import com.google.android.exoplayer2.Format;
 import com.google.android.exoplayer2.LivePlaybackSpeedControl;
 import com.google.android.exoplayer2.MediaItem;
 import com.google.android.exoplayer2.MediaMetadata;
 import com.google.android.exoplayer2.PlaybackException;
 import com.google.android.exoplayer2.PlaybackParameters;
 import com.google.android.exoplayer2.Player;
 import com.google.android.exoplayer2.RenderersFactory;
 import com.google.android.exoplayer2.SimpleExoPlayer;
 import com.google.android.exoplayer2.Timeline;
 import com.google.android.exoplayer2.analytics.AnalyticsListener;
 import com.google.android.exoplayer2.audio.AudioAttributes;
 import com.google.android.exoplayer2.audio.AudioSink;
 import com.google.android.exoplayer2.drm.DefaultDrmSessionManager;
 import com.google.android.exoplayer2.drm.DrmSession;
 import com.google.android.exoplayer2.drm.DrmSessionEventListener;
 import com.google.android.exoplayer2.drm.DrmSessionManager;
 import com.google.android.exoplayer2.drm.FrameworkMediaDrm;
 import com.google.android.exoplayer2.drm.HttpMediaDrmCallback;
 import com.google.android.exoplayer2.mediacodec.MediaCodecRenderer.DecoderInitializationException;
 import com.google.android.exoplayer2.mediacodec.MediaCodecUtil.DecoderQueryException;
 import com.google.android.exoplayer2.metadata.id3.Id3Frame;
 import com.google.android.exoplayer2.offline.DownloadRequest;
 import com.google.android.exoplayer2.source.BehindLiveWindowException;
 import com.google.android.exoplayer2.source.DefaultMediaSourceFactory;
 import com.google.android.exoplayer2.source.MediaSource;
 import com.google.android.exoplayer2.source.MediaSourceFactory;
 import com.google.android.exoplayer2.source.TrackGroupArray;
 import com.google.android.exoplayer2.source.ads.AdsLoader;
 import com.google.android.exoplayer2.text.Cue;
 import com.google.android.exoplayer2.trackselection.DefaultTrackSelector;
 import com.google.android.exoplayer2.trackselection.MappingTrackSelector;
 import com.google.android.exoplayer2.trackselection.MappingTrackSelector.MappedTrackInfo;
 import com.google.android.exoplayer2.trackselection.TrackSelectionArray;
 import com.google.android.exoplayer2.upstream.BandwidthMeter;
 import com.google.android.exoplayer2.upstream.DataSource;
 import com.google.android.exoplayer2.upstream.DefaultDataSourceFactory;
 import com.google.android.exoplayer2.util.DebugTextViewHelper;
 import com.google.android.exoplayer2.util.ErrorMessageProvider;
 import com.google.android.exoplayer2.util.EventLogger;
 import com.google.android.exoplayer2.util.Util;
 import com.google.android.exoplayer2.video.VideoSize;

 import org.jetbrains.annotations.Nullable;

 import java.io.IOException;
 import java.util.ArrayList;
 import java.util.Collections;
 import java.util.List;
 import java.util.Locale;
 import java.util.Map;
 import java.util.UUID;
 import java.util.concurrent.CopyOnWriteArrayList;

 import kantvai.media.player.KANTVLog;
 import kantvai.media.player.KANTVUtils;
 import kantvai.media.player.KANTVDRMManager;
 import kantvai.media.player.bean.ExoTrackInfoBean;
 import kantvai.media.player.bean.TrackInfoBean;

 import static com.google.android.exoplayer2.ExoPlaybackException.TYPE_SOURCE;
 import static com.google.android.exoplayer2.PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW;
 import static com.google.android.exoplayer2.PlaybackException.ERROR_CODE_UNSPECIFIED;
 import static com.google.android.exoplayer2.util.Assertions.checkNotNull;
 import static com.google.android.exoplayer2.util.MimeTypes.APPLICATION_M3U8;
 import static com.google.android.exoplayer2.util.Util.getAdaptiveMimeTypeForContentType;


 public class DemoPlayer {
     private final static String TAG = DemoPlayer.class.getName();
     private DataSource.Factory dataSourceFactory;
     private DefaultTrackSelector trackSelector;
     private DefaultTrackSelector.Parameters trackSelectorParameters;

     private TrackGroupArray lastSeenTrackGroupArray;
     private boolean startAutoPlay;
     private int startWindow;
     private long startPosition;
     private List<MediaItem> mediaItems;

     public static final int RENDERER_COUNT = 4;
     public static final int TYPE_VIDEO = 0;
     public static final int TYPE_AUDIO = 1;
     public static final int TYPE_TEXT = 2;
     public static final int TYPE_METADATA = 3;

     protected @Nullable
     SimpleExoPlayer player;

     private Surface surface;
     //List<MediaItem> mediaItems;
     private Format videoFormat;
     private int videoTrackToRestore;

     private BandwidthMeter bandwidthMeter;
     private boolean backgrounded;

     private CaptionListener captionListener;
     private Id3MetadataListener id3MetadataListener;
     private InternalErrorListener internalErrorListener;
     private InfoListener infoListener;
     private final CopyOnWriteArrayList<Listener> listeners;

     private String mVideoUrl;
     private String mVideoTitle;
     private Context mContext;
     private AppCompatActivity mActivity;
     private boolean mDisableAudioTrack = false;
     private boolean mDisableVideoTrack = false;
     private boolean mUsingTEE = false;
     private boolean mDRMEnabled  = false;
     private String  mVideoInfo = "unknown";
     private String  mAudioInfo = "unknown";
     private String  mVideoDecoder = "MediaCodec";
     private String  mAudioDecoder = "unknown";
     private long    mBitrate   = 0;

     private long    mStartTimes = 0;

     /**
      * A listener for receiving notifications of timed text.
      */
     public interface CaptionListener {
         void onCues(List<Cue> cues);
     }

     /**
      * A listener for receiving ID3 metadata parsed from the media stream.
      */
     public interface Id3MetadataListener {
         void onId3Metadata(List<Id3Frame> id3Frames);
     }

     public void addListener(Listener listener) {
         listeners.add(listener);
     }


     public void removeListener(Listener listener) {
         listeners.remove(listener);
     }

     public void setInternalErrorListener(InternalErrorListener listener) {
         internalErrorListener = listener;
     }

     public void setInfoListener(InfoListener listener) {
         infoListener = listener;
     }



     /**
      * @return Whether initialization was successful.
      */
     protected boolean initializePlayer() {
         KANTVLog.d(TAG, "enter initializePlayer");
         if (player == null) {
             boolean usingComplexExo2Instance = true;

             KANTVLog.d(TAG, "initializePlayer1");
             trackSelector = new DefaultTrackSelector(/* context= */ mActivity);
             KANTVLog.d(TAG, "initializePlayer2");

             if (usingComplexExo2Instance) {
                 boolean preferExtensionDecoders = false;
                 boolean preferExtensionRenderer = true;
                 KANTVLog.d(TAG, "initializePlayer3");
                 dataSourceFactory = DemoUtil.getDataSourceFactory(/* context= */ mActivity.getApplicationContext());
                 KANTVLog.d(TAG, "initializePlayer4");
                 DefaultTrackSelector.ParametersBuilder builder =
                         new DefaultTrackSelector.ParametersBuilder(/* context= */ mActivity.getApplicationContext());
                 trackSelectorParameters = builder.build();
                 KANTVLog.d(TAG, "initializePlayer5");

                 RenderersFactory renderersFactory = null;
                 if (!KANTVUtils.getUsingFFmpegCodec()) {
                     int extensionRendererMode =
                             preferExtensionDecoders
                                     ? (preferExtensionRenderer
                                     ? DefaultRenderersFactory.EXTENSION_RENDERER_MODE_PREFER
                                     : DefaultRenderersFactory.EXTENSION_RENDERER_MODE_ON)
                                     : DefaultRenderersFactory.EXTENSION_RENDERER_MODE_OFF;
                     renderersFactory = DemoUtil.buildRenderersFactory(mActivity.getApplicationContext(), preferExtensionDecoders);
                     KANTVLog.g(TAG, "using system decoder for audio");
                 } else {
                     int extensionRendererMode = DefaultRenderersFactory.EXTENSION_RENDERER_MODE_PREFER;
                     renderersFactory = new DefaultRenderersFactory(mActivity.getApplicationContext()).setExtensionRendererMode(extensionRendererMode);
                     KANTVLog.g(TAG, "using ffmpeg decoder for audio");
                 }

                 KANTVLog.d(TAG, "initializePlayer6");
                 MediaSourceFactory mediaSourceFactory =
                         new DefaultMediaSourceFactory(dataSourceFactory);

                 KANTVLog.d(TAG, "initializePlayer7");
                 trackSelector = new DefaultTrackSelector(/* context= */ mActivity.getApplicationContext());
                 trackSelector.setParameters(trackSelectorParameters);
                 lastSeenTrackGroupArray = null;
                 KANTVLog.d(TAG, "initializePlayer8");
                  /*
                  re-buffering issue with live content

                  Main symptoms:
                  some seconds of buffering occurs when playing 4k encrypted HLS live content
                  Repeated buffy worms occur after video loading and playback

                  https://github.com/google/ExoPlayer/issues/9531
                  https://github.com/google/ExoPlayer/issues/9553
                  https://github.com/google/ExoPlayer/issues/9059
                  https://github.com/google/ExoPlayer/issues/7256
                  https://github.com/google/ExoPlayer/issues/4023
                  https://github.com/google/ExoPlayer/issues/9820
                  https://github.com/google/ExoPlayer/issues/7427
                  https://github.com/google/ExoPlayer/issues/7351
                  https://github.com/google/ExoPlayer/issues/2438
                  https://github.com/google/ExoPlayer/issues/2083
                  https://github.com/google/ExoPlayer/issues/5978
                  https://github.com/google/ExoPlayer/issues/5932
                  https://github.com/google/ExoPlayer/issues/3040
                  https://github.com/google/ExoPlayer/commit/9463c31cded5cd6523769c4deab04cc4204eeb3c
                  https://github.com/google/ExoPlayer/commit/9725132e3c09280c2532f2de3077e9cd6ba28b1d
                  https://github.com/google/ExoPlayer/issues/9578


                 MediaSource videoSource = new ProgressiveMediaSource.Factory(new DefaultDataSourceFactory(context)).createMediaSource(MediaItem.fromUri(video));
                 MediaSource audioSource = new ProgressiveMediaSource.Factory(new DefaultDataSourceFactory(context)).createMediaSource(MediaItem.fromUri(audio));
                 return new MergingMediaSource(videoSource, audioSource);

                 And the settings below are the settings created when loading a video through exoplayer.
                 Is there something wrong with the settings?

                 DefaultLoadControl loadControl = new DefaultLoadControl.Builder()
                .setAllocator(new DefaultAllocator(true, C.DEFAULT_BUFFER_SEGMENT_SIZE))
                .setBufferDurationsMs(30_000, 60_000, 1000,1000)
                .setTargetBufferBytes(C.LENGTH_UNSET)
                .setPrioritizeTimeOverSizeThresholds(true)
                .setBackBuffer(1000 * 15,true)
                .build();

                 DataSource.Factory cacheDataSourceFactory = new CacheDataSource.Factory()
                .setCache(VideoCache.getInstance(context))
                .setUpstreamDataSourceFactory(dataSourceFactory);

                 DefaultBandwidthMeter defaultBandwidthMeter = new DefaultBandwidthMeter
                .Builder(this)
                .setResetOnNetworkTypeChange(true)
                .setSlidingWindowMaxWeight(2000)
                .setInitialBitrateEstimate(Integer.MAX_VALUE)
                .build();

                 DefaultTrackSelector trackSelector = new DefaultTrackSelector(this);
                 trackSelector.setParameters(trackSelector
                .buildUponParameters()
                .setForceLowestBitrate(true)
                .setForceHighestSupportedBitrate(true)
                .setMaxVideoSizeSd());

                 exoPlayerVideo = new SimpleExoPlayer.Builder(this)
                .setLoadControl(loadControl)
                .setMediaSourceFactory(new DefaultMediaSourceFactory(cacheDataSourceFactory))
                .setBandwidthMeter(defaultBandwidthMeter)
                .setTrackSelector(trackSelector)
                .setUseLazyPreparation(true)
                .build();


                DefaultLoadControl loadControl = new DefaultLoadControl.Builder()
                    .setAllocator(new DefaultAllocator(true, C.DEFAULT_BUFFER_SEGMENT_SIZE))
                //  .setBufferDurationsMs(24000, 24000, 1500, 2000)
                    .setPrioritizeTimeOverSizeThresholds(true)
                    .build();

               Also I have tested that downgrading ExoPlayer does not solve the issue (at least down to 2.11.7)

               Increasing buffer size to 32000 resolves the issue:
               .setBufferDurationsMs(32000, 32000, 1500, 2000)


               The problem is even easier to reproduce when the buffer durations are set to a
               really low value (e.g. setBufferDurationsMs(2000, 2000, 1500, 2000)).
               I can reproduce after ~1 minute of playback with these values.

               The underlying problem seems to be that the HLS source reports a buffered duration that
               doesn't match the actually loaded samples. So the player thinks the source has sufficient
               buffers loaded for playback (and doesn't continue loading), but actually there are no samples
               available for playback and the player gets stuck. Our stuck buffering detection doesn't detect
               this either because it only treats the player as stuck if the reported buffered duration is almost zero (which isn't the case here).

               The reason this is happening as described above is that the declared segment durations of some
               segments in the HLS playlist don't really match the actual duration of the media. Over time
               we accumulate an error between the calculated position of a chunk and the actual media timestamps.
               Once the accumulated error is above the maximum buffer configured in the LoadControl, the player gets stuck.

               I'll close this as bad media because the HLS spec clearly states that "Durations SHOULD be decimal-floating-point,
               with enough accuracy to avoid perceptible error when segment durations are accumulated. "

               That is probably working as intended.

               Note that we can't keep arbitrary amounts of buffer in memory. That's why we only buffer a few
               seconds ahead as configured in the LoadControl (default is 50 seconds maximum).

               We also buffer ahead into the next playlist item if we already finished buffering the current one.
               The buffer is kept if you seek to a position within the current buffer (no matter if it's in the
               current or the next item). However, we do throw away the buffer of other playlist items you didn't seek into.
               As soon as you seek somewhere outside the current buffer (that could be before the current position
               or behind the buffered position), we throw away the already buffered data and start buffering from the new position.

               I wasn't able to able to open the traffic log as I don't have access to the Charles application. But does
               the description above fit what you are seeing?

               We should try to keep the buffer of other upcoming playlist items if we were able to keep the buffer of
               the target item. Marking as an enhancement to do that. But it's low-priority for now.

               (a) the server cannot transfer media at faster than 1x speed, and
               (b) this means that the client will need to re-buffer.
               This is correct, and (b) is fundamentally unavoidable if (a) is true.
               If you wish to avoid re-buffering, you'll need to make it so that (a) is false.
               For example by re-encoding the content at a lower bitrate

               Of course the issue should be resolve on the server side but I don't have any control on it.
               I try to play a live 4k content but if the link between the client and the server is not
               optimal the player need to re-buffer or play at lower resolution. After I have read the parallelize
               downloads implementation in the version 1.12 I have thinking that maybe I could solve my issue but I could not found how.
               Feel free to close this "issue" if you think this case should not be resolve on client side.


               Internally ExoPlayer builds a queue of media periods [p,p,p,p,...] to be sequentially played and hence to be buffered.
               The sequence is determined by the current window index, the timeline, the repeat mode and the shuffle mode.
               The queue is rebuilt each time when either of these determinants changes.
               Regarding question 1) The player doesn't start rendering when in STATE_BUFFERING. The player starts playing when in STATE_READY.

               You are correct for question 2) in saying that the player downloads data even when playWhenReady==false.
               When the player is prepared, it transitions from STATE_IDLE to STATE_BUFFERING until enough data is available
               to transition to STATE_READY. This happens regardless of playWhenReady being true or false.
               In general the player state (IDLE|BUFFERING|READY|ENDED) and playWhenReady are independent.
               As an aside: There is a method player.isPlaying and a callback Player.EventListener.onIsPlayingChanged().
               This gives you a boolean which reports whether the player is actually playing media.

               It is working as intended. STATE_READY is defined as:
               The player is able to immediately play from its current position.
               This would include both the audio and video parts of the stream being
               ready for playback. Independently to that, the first video frame will always be rendered as soon as it's possible.
               So there's no guaranteed ordering between the two. I don't think you should ever need to rely on there being a particular order,
               so if you're trying to, it's likely you're doing something wrong or unnecessary.
               Regarding the original question, the Javadoc (linked above) defines STATE_READY and STATE_BUFFERING very unambiguously.
               Please always check the documentation before filing issues.
               The Javadoc for the states is defined clearly:
               STATE_READY - The player is able to immediately play from its current position.

               It should be clear that the ability to play is not tied to the buffer reaching its maximum possible size.
               So you'd expect STATE_READY to come before the maximum buffer duration is reached in most cases.
               The ability to play is tied to DEFAULT_BUFFER_FOR_PLAYBACK_MS:

               DEFAULT_BUFFER_FOR_PLAYBACK_MS - The default duration of media that must be buffered for playback to start or
               resume following a user action such as a seek, in milliseconds.

               So you'd expect STATE_READY to happen around the point at DEFAULT_BUFFER_FOR_PLAYBACK_MS of media has
               been buffered (or DEFAULT_BUFFER_FOR_PLAYBACK_AFTER_REBUFFER_MS after a re-buffer).

               There is no guarantee on the ordering of these events though, and I can't really envisage a valid
               case where you'd ever need to rely on one.


               example
               https://github.com/google/ExoPlayer/blob/release-v2/library/core/src/main/java/com/google/android/exoplayer2/trackselection/BufferSizeAdaptationBuilder.java#L245
               and set minBufferMs and bufferForPlaybackMs to a minimum of 30000
               remember maxBufferMs >= minBufferMs
               check https://exoplayer.dev/doc/reference/com/google/android/exoplayer2/DefaultLoadControl.html#DEFAULT_MIN_BUFFER_MS
               for more detailed description of it value
               Maybe you have a use for it, but I think 5s is more than enough for most playback, you can even set a lower value for
               bufferForPlaybackMs let say 1s and let minBufferMs be 5s the playback will start after 1s has be buffered
               (not the time that will take but the size in time of the buffer) and the buffer will fill up with up to 5s,
               but you can't do that for the 30s if 30s isn't available you must set as I wrote before and wait 25s buffering.
               use player.getTotalBufferedDuration() to see how long in time is the buffer

               You can implement a custom LoadControl and return true from shouldContinueLoading() whenever you want the buffering to continue.

               Configure minBuffer < maxBuffer (by default exoplayer use 50s for min and max, that leads to "constant" loading) smth
               like minBuffer = 1min, maxBuffer=2min. In this case, I'll often have a (maxBuffer - minBuffer) window for preloading


               We considered parallel chunk fetching and ultimately concluded that we're better off with the sequential approach.

               Sequential fetching has the obvious benefit of always requesting bytes in the exact order in which they're required.
               With parallel fetching this is not true, and so it's possible that fetching bytes that are required further into
               the future will introduce additional startup latency and/or re-buffering events by impeding transfer of bytes that
               you need more urgently. Sequential fetching is also simpler to manage from an implementation and correctness point of view.
               Implementing responsive adaptive bit-rate logic is also more straightforward with a sequential approach.

               Parallel fetching is beneficial primarily when (a) round trip time is large (so the overhead in starting each request
               is large, even with socket re-use) (b) chunks are short (meaning the overhead is incurred often) and (c) throughput is
               still good enough for streaming. In practice it's doubtful how often (a) and (c) would both be true at once.
               Media providers should be making chunks a sensible size, also.

               If you define robustness to mean that the playback doesn't fail, then it's always possible to be equally robust using
               either approach (in the extreme case you can design the player to retry indefinitely in both cases, and hence achieve
               0% failures). If you define robustness to mean the playback not re-buffering, then it can be debated which is better or worse.

               As above, we believe that parallel loading is better only under network conditions that occur rarely in practice.
               Or at least, we believe that sequential loading will perform better in the majority of cases. We have no plans to
               support parallel loading. Doing so would significantly increase complexity throughout a large part of the code-base for marginal gain.


               (1)Once the buffer is full we'll utilize the network at a lower rate, because at that point we can only fetch media at approximately
               the same rate as it's being consumed. So you're probably seeing a utilization drop simply because the buffer is as full as it can be (which is a good thing!).
               (2)Once the buffer becomes full, we do implement some fill/drain logic that allows the buffer to drain out for a while before it starts re-filling.
                We start start re-filling again well before it's nearly empty though. I believe there are some battery and mobile-carrier-network related reasons for doing this.
               (3)Both the buffer size and fill/drain logic can be controlled when creating the player. Buffer size can be controlled by changing
               the buffer contribution values passed to HlsSampleSource. The fill/drain logic can be controlled by using a different constructor when
               instantiating the DefaultLoadControl, which allows passing of additional control parameters.
               You can also replace DefaultLoadControl with your own implementation.
               (4)To allow maximal usage of network until the entire stream is downloaded would require us to extend the buffer from memory into
               flash storage. We've opted not to do this for the time-being. It's definitely not always ideal behaviour. If you're operating at
               scale then there are negative cost and network load implications of doing this. Flash storage is also a tricky area in general to
               get right (e.g. not using too much of it, handling read/write failures etc).

               DefaultLoadControl uses a combination of buffered media duration and size to determine when it considers the buffer full.
               You can control the parameters for each dimension when you create the DefaultLoadControl,
               by using the constructor that takes the most arguments.


               Just to confirm the core changes:

               Reduce minBuffer to 6 seconds and maxBuffer to 10 seconds in DefaultLoadControl
               Throttle network speed to simulate a poor connection.
               Play a stream with a single non-adaptive 6.1Mbps track.

               Observed effect:

               Player state changes between BUFFERING and READY

               Expected effect:

               Player is always READY (?)

               If this summary is correct, then everything is probably working as intended. The throttling
               likely means the player can't buffer fast enough, so will run into rebuffers from time to time.

               To avoid this situation, you can do multiple things:

               Provide adaptive content with multiple bitrates. Just having a stream with 6.1Mbps is likely to cause issues in many network conditions.
               Increase the maximum buffer of DefaultLoadControl so that more buffer is available when the network speed falls too low.
               Provide download functionality so that the stream can be downloaded by the user before playback starts (see https://exoplayer.dev/downloading-media.html).

               There are four parameters within DefaultLoadControl; two of them control the load timing;
               the other twos control when the playback starts by having sufficient buffering.

               The maxBufferUs & minBufferUs are about the load timing but bufferForPlaybackAfterRebufferUs
                & bufferForPlaybackUs are about when the playback starts by having sufficient buffering.

               We'll update our default for playbacks with video* to have the same min and max buffer value.
               That means it will follow the drip-feeding pattern described above. Our experiments showed that
               rebuffers and the number of format changes went down significantly. At the same time, battery usage increased only very slightly.
               */

                 LivePlaybackSpeedControl speedControl = new DefaultLivePlaybackSpeedControl.Builder()
                         .setFallbackMinPlaybackSpeed(1f)
                         .setFallbackMaxPlaybackSpeed(1f)
                         .build();
                 if (KANTVUtils.getHLSPlaylistType() == KANTVUtils.PLAYLIST_TYPE_LIVE) {
                     player =
                             new SimpleExoPlayer.Builder(/* context= */ mActivity, renderersFactory)
                                     .setMediaSourceFactory(mediaSourceFactory)
                                     .setTrackSelector(trackSelector)
                                     .setLivePlaybackSpeedControl(speedControl)
                                     .build();
                 } else {
                     player =
                             new SimpleExoPlayer.Builder(/* context= */ mActivity, renderersFactory)
                                     .setMediaSourceFactory(mediaSourceFactory)
                                     .setTrackSelector(trackSelector)
                                     .build();
                 }
                 KANTVLog.d(TAG, "initializePlayer9");
             } else {
                 KANTVLog.d(TAG, "initializePlayer3");
                 player = new SimpleExoPlayer.Builder(mActivity).build();
                 KANTVLog.d(TAG, "initializePlayer4");
             }
             KANTVLog.d(TAG, "after create SimpleExoPlayer");
             player.addListener(new PlayerEventListener());
             player.addAnalyticsListener(new EventLogger(trackSelector));
             player.addAnalyticsListener(mAnalyticsListener);
             player.setAudioAttributes(AudioAttributes.DEFAULT, /* handleAudioFocus= */ true);
         }
         KANTVLog.j(TAG, "leave initializePlayer");

         return true;
     }


     public DemoPlayer(AppCompatActivity activity, Context context) {
         KANTVLog.d(TAG, "enter DemoPlayer");
         mActivity = activity;
         mContext = context;
         listeners = new CopyOnWriteArrayList<>();
         initializePlayer();
         KANTVLog.d(TAG, "leave DemoPlayer");
     }


     public void setSurface(Surface surface) {
         KANTVLog.d(TAG, "enter setSurface");
         if (player == null) {
             KANTVLog.d(TAG, "it shouldn't happend, pls check");
             return;
         }
         this.surface = surface;
         player.setVideoSurface(surface);
         KANTVLog.d(TAG, "leave setSurface");
     }

     public void setDisableAudioTrack(boolean bDisable) {
         mDisableAudioTrack = bDisable;
     }

     public void setDisableVideoTrack(boolean bDisable) {
         mDisableVideoTrack = bDisable;
     }

     public void setUsingTEE(boolean bUsingTEE) {
         mUsingTEE = bUsingTEE;
     }

     public boolean isDRMEnabled() {
         if (mUsingTEE) {
             return mDRMEnabled;
         }
         return false;
     }

     public Surface getSurface() {
         return surface;
     }

     private void showToast(String message) {
         Toast.makeText(mContext, message, Toast.LENGTH_LONG).show();
     }

     public void prepare() {
         if (player == null || mVideoUrl == null) {
             KANTVLog.j(TAG, "it shouldn't happen, pls check");
             return;
         }
         KANTVLog.g(TAG, "using TEE: " + mUsingTEE);

         MediaItem.Builder mediaItem = new MediaItem.Builder();
         String title = "multidrm client via exoplayer engine";
         if ((mVideoTitle != null) && (!mVideoTitle.isEmpty()))
            title = mVideoTitle.trim();
         else {
             KANTVLog.j(TAG, "video title is empty, pls check");
             return;
         }
         Uri uri = Uri.parse(mVideoUrl.trim());
         KANTVLog.g(TAG, "url:" + mVideoUrl);
         String drmScheme = KANTVUtils.getDrmSchemeName();
         String drmLicenseURL = KANTVUtils.getDrmLicenseURL();
         KANTVLog.g(TAG, "name:" + title);
         KANTVLog.g(TAG, "uri:" + uri.toString());
         KANTVLog.g(TAG, "drm scheme:" + drmScheme);
         KANTVLog.g(TAG, "drm license url:" + drmLicenseURL);
         if (mUsingTEE) {
             KANTVLog.j(TAG, "using TEE");
             if (drmScheme != null && !drmScheme.isEmpty()) {
                 if (drmScheme.equals("chinadrm")) {
                     if (KANTVUtils.getEnableWisePlay()) {
                         mediaItem.setDrmUuid(Util.getDrmUuid("wiseplay")); //wiseplay drm
                         KANTVDRMManager.setMultiDrmInfo(KANTVUtils.DRM_SCHEME_WISEPLAY);
                     } else {
                         mediaItem.setDrmUuid(Util.getDrmUuid("chinadrm")); //chinadrm
                     }
                 } else if (drmScheme.equals("widevine")) {
                     mediaItem.setDrmUuid(Util.getDrmUuid("widevine")); //widevine drm
                 } else if (drmScheme.equals("wiseplay")) {
                     KANTVLog.j(TAG, "this is wiseplay content");
                     mediaItem.setDrmUuid(Util.getDrmUuid("wiseplay")); //wiseplay drm
                     KANTVDRMManager.setMultiDrmInfo(KANTVUtils.DRM_SCHEME_WISEPLAY);
                 }
             } else {
                 KANTVLog.j(TAG, "can't find valid drm scheme info, assume it's chinadrm");
                 mediaItem.setDrmUuid(Util.getDrmUuid("chinadrm"));
             }

             if (drmLicenseURL != null && !drmLicenseURL.isEmpty()) {
                 mediaItem.setDrmLicenseUri(drmLicenseURL);
             }
         }
         if (false) {
             //troubleshooting WisePlay DRM
             int encryptKeyType = KANTVUtils.KEY_AES_CBC;
             int encryptESType  = KANTVUtils.ES_H264_AES_CBC;
             String offlineDRMIVString  = "pby2hicdq57qp59h";
             String offlineDRMKeyString = "aeia7akp9ac5ikpa";
             byte[] offlineDRMIV  = offlineDRMIVString.getBytes();
             byte[] offlineDRMkey = offlineDRMKeyString.getBytes();
             KANTVLog.j(TAG, "iv:" + KANTVUtils.bytesToHexString(offlineDRMIV));
             KANTVLog.j(TAG, "key:" + KANTVUtils.bytesToHexString(offlineDRMkey));
             KANTVDRMManager.setOfflineDrmInfo(encryptKeyType, encryptESType, offlineDRMkey, offlineDRMIV);
         }

         KANTVLog.g(TAG, "uri " + uri);
         String adaptiveMimeType =
                 Util.getAdaptiveMimeTypeForContentType(Util.inferContentType(uri, null));
         KANTVLog.g(TAG, "adaptiveMimeType:" + adaptiveMimeType);
         if (KANTVUtils.getHLSPlaylistType() == KANTVUtils.PLAYLIST_TYPE_LIVE) {
             /*
                Configuring live playback parameters:
                 targetOffsetMs: The target live offset. The player will attempt to get close to this live offset during playback if possible.
                 minOffsetMs: The minimum allowed live offset. Even when adjusting the offset to current network conditions, the player will not attempt to get below this offset during playback.
                 maxOffsetMs: The maximum allowed live offset. Even when adjusting the offset to current network conditions, the player will not attempt to get above this offset during playback.
                 minPlaybackSpeed: The minimum playback speed the player can use to fall back when trying to reach the target live offset.
                 maxPlaybackSpeed: The maximum playback speed the player can use to catch up when trying to reach the target live offset.

                 Since 2.13 you can build a media item with a LiveConfiguration. Something like

                 MediaItem mediaItem = new MediaItem.Builder()
                    .setUri(uri)
                    .setLiveTargetOffsetMs(10 * 60_000)
                    .build();

                 The above media item would make the player use a target offset of 10 minutes behind the live edge.
                 This would allow the to set an arbitrary position. If you know the duration of the live window
                 of your stream you can set it appropriately at the beginning of the live window (possibly
                 with some margin to avoid running into a BehindLiveWindowException).

                 Something like a method to override the start time in the playlist for a given media item would work for you?

                 MediaItem mediaItem = new MediaItem.Builder()
                    .setUri(uri)
                    .setRequestedLiveUnixStartTimeMs(unixStartTimeMs)
                    .build();

                The liveTargetOffset would then be calculated from that start time you are declaring in the media item.

                If I force LivePlaybackSpeedControl Min/Max playback speed to 1f - playback is smooth.
                If use default (do not specify) - playback stutters periodically

                https://exoplayer.dev/live-streaming.html

             */
             KANTVLog.g(TAG, "live content");
             mediaItem
                     .setUri(uri)
                     .setLiveTargetOffsetMs(1000)
                     .setLiveMinOffsetMs(900)
                     .setLiveMaxOffsetMs(9000)
                     .setLiveMinPlaybackSpeed(0.50f)
                     .setLiveMaxPlaybackSpeed(1.20f)
                     .setMediaMetadata(new MediaMetadata.Builder().setTitle(title).build())
                     .setMimeType(adaptiveMimeType);

         } else {
             KANTVLog.g(TAG, "vod content");
             mediaItem
                     .setUri(uri)
                     .setMediaMetadata(new MediaMetadata.Builder().setTitle(title).build())
                     .setMimeType(adaptiveMimeType);
         }

         MediaItem kantvmediaItem = mediaItem.build();
         if (mUsingTEE) {
             //做点sanity check
             MediaItem.DrmConfiguration drmConfiguration = checkNotNull(kantvmediaItem.playbackProperties).drmConfiguration;
             if (drmConfiguration != null) {
                 KANTVLog.j(TAG, "drm uuid:" + drmConfiguration.uuid.toString());
                 if (Util.SDK_INT < 18) {
                     String errMsg = "DRM content not supported";
                     KANTVLog.j(TAG, errMsg);
                     showToast(errMsg);
                     ExoPlaybackException error = ExoPlaybackException.createFromString(errMsg);
                     for (Listener listener : listeners) {
                         listener.onError(error);
                     }
                     return;
                 } else if (!FrameworkMediaDrm.isCryptoSchemeSupported(drmConfiguration.uuid)) {
                     String errMsg = "this device does not support the required DRM scheme: " + KANTVUtils.getDrmSchemeName();
                     KANTVLog.j(TAG, errMsg);
                     showToast(errMsg);
                     ExoPlaybackException error = ExoPlaybackException.createFromString(errMsg);
                     for (Listener listener : listeners) {
                         listener.onError(error);
                     }

                     return;
                 }
                 if (FrameworkMediaDrm.isCryptoSchemeSupported(drmConfiguration.uuid)) {
                     if (drmScheme != null && !drmScheme.isEmpty()) {
                         KANTVLog.j(TAG, "this device support the required DRM scheme: " + drmScheme);
                     } else {
                         KANTVLog.j(TAG, "this device support the required DRM scheme: " + Util.getDrmUuid("chinadrm").toString());
                     }
                 }
             } else {
                 KANTVLog.j(TAG, "drmConfiguation is null, it shouldn't happen, pls check");
             }
         }

         KANTVLog.d(TAG, "player.setMediaItem(mediaItem)");
         player.setMediaItem(kantvmediaItem);
         KANTVLog.d(TAG, "player.prepare()");
         player.prepare();
         KANTVLog.d(TAG, "leave prepare");
     }

     public void play() {
         KANTVLog.d(TAG, "enter play");
         assert player != null;
         player.play();
         Format videoFormat = player.getVideoFormat();
         Format audioFormat = player.getAudioFormat();
         if (videoFormat != null)
            mVideoInfo =  videoFormat.toString();

         if (audioFormat != null)
             mAudioInfo =  audioFormat.toString();
         KANTVLog.d(TAG, "leave play");
     }

     public void setDataSource(String videoUrl) {
         KANTVLog.d(TAG, "enter setDataSource: " + videoUrl);
         mVideoUrl = videoUrl;
         KANTVLog.d(TAG, "leave setDataSource");
     }

     public void setVideoTitle(String videoTitle) {
         KANTVLog.d(TAG, "enter setVideoTitle: " + videoTitle);
         mVideoTitle = videoTitle;
         KANTVLog.d(TAG, "leave setVideoTitle");
     }


     public void setPlayWhenReady(boolean playWhenReady) {
         KANTVLog.d(TAG, "enter setPlayWhenReady: " + playWhenReady);
         assert player != null;
         KANTVLog.d(TAG, "player.setPlayWhenReady(" + playWhenReady + ")");


         KANTVLog.d(TAG, "disable audio: " + mDisableAudioTrack);
         KANTVLog.d(TAG, "disable video: " + mDisableVideoTrack);
         if ((mDisableAudioTrack) && (mDisableVideoTrack)) {
             KANTVLog.j(TAG, "audio and video couldn't be both disabled, pls check");
         }
         else if (mDisableAudioTrack) {
             int indexOfAudioRenderer = -1;
             KANTVLog.d(TAG, "track counts: " + player.getRendererCount());
             for (int i = 0; i < player.getRendererCount(); i++) {
                 if (player.getRendererType(i) == C.TRACK_TYPE_AUDIO) {
                     KANTVLog.d(TAG, "found audio track index " + i);
                     indexOfAudioRenderer = i;
                     break;
                 }
             }
             DefaultTrackSelector.ParametersBuilder builder = trackSelectorParameters.buildUpon();
             builder.setRendererDisabled(indexOfAudioRenderer, /* disabled= */ true);
             trackSelector.setParameters(builder);
         } else if (mDisableVideoTrack) {
             int indexOfVideoRenderer = -1;
             KANTVLog.j(TAG, "track counts: " + player.getRendererCount());
             for (int i = 0; i < player.getRendererCount(); i++) {
                 if (player.getRendererType(i) == C.TRACK_TYPE_VIDEO) {
                     KANTVLog.d(TAG, "found video track index " + i);
                     indexOfVideoRenderer = i;
                     break;
                 }
             }
             DefaultTrackSelector.ParametersBuilder builder = trackSelectorParameters.buildUpon();
             builder.setRendererDisabled(indexOfVideoRenderer, /* disabled= */ true);
             trackSelector.setParameters(builder);
         }
         //end added

         player.setPlayWhenReady(playWhenReady);
         KANTVLog.d(TAG, "leave setPlayWhenReady");
     }

     public void seekTo(long positionMs) {
         KANTVLog.d(TAG, "enter seekTo " +  positionMs);
         assert player != null;
         player.seekTo(positionMs);
         KANTVLog.d(TAG, "leave seekTo");
     }

     public void release() {
         KANTVLog.d(TAG, "enter release");
         if (player != null) {
             surface = null;
             player.release();
             player = null;
         } else {
             KANTVLog.d(TAG, "already released");
         }
         KANTVLog.d(TAG, "leave release");
     }

     protected void releasePlayer() {
         KANTVLog.d(TAG, "enter releasePlayer");
         if (player != null) {
             surface = null;
             player.release();
             player = null;
             //mediaItems = Collections.emptyList();
             trackSelector = null;
         } else {
             KANTVLog.d(TAG, "already released");
         }
         KANTVLog.d(TAG, "leave releasePlayer");
     }


     public String getVideoInfo() {
         /*
         if (player != null) {
             Format videoFormat = player.getVideoFormat();
             if (videoFormat != null)
                 mVideoInfo = videoFormat.toString();
         }
         */
         return mVideoInfo;
     }


     public String getAudioInfo() {
         /*
         if (player != null) {
             Format audioFormat = player.getAudioFormat();
             if (audioFormat != null)
                 mAudioInfo = audioFormat.toString();
         }
         */

         return mAudioInfo;
     }

     public String getVideoDecoderName() {
         return mVideoDecoder;
     }

     public String getAudioDecoderName() {
         return mAudioDecoder;
     }

     public void selectTrack(TrackInfoBean trackInfo, boolean isAudio) {
         KANTVLog.j(TAG, "select track:" + trackInfo.getName());
         ExoTrackInfoBean trackInfoBean = (ExoTrackInfoBean) trackInfo;

         MappingTrackSelector.MappedTrackInfo mappedTrackInfo = trackSelector.getCurrentMappedTrackInfo();
         TrackGroupArray trackGroupArray = null;
         if (mappedTrackInfo != null) {
             trackGroupArray = mappedTrackInfo.getTrackGroups(trackInfoBean.getRenderId());
         }
         DefaultTrackSelector.SelectionOverride override =
                 new DefaultTrackSelector.SelectionOverride(trackInfoBean.getTrackGroupId(), trackInfoBean.getTrackId());
         DefaultTrackSelector.ParametersBuilder parametersBuilder = trackSelector.buildUponParameters();
         parametersBuilder.setRendererDisabled(trackInfoBean.getRenderId(), false);
         parametersBuilder.setSelectionOverride(trackInfoBean.getRenderId(), trackGroupArray, override);
         trackSelector.setParameters(parametersBuilder);
     }

     public long getDuration() {
         assert player != null;
         return player.getDuration();
     }


     public long getCurrentPosition() {
         assert player != null;
         return player.getCurrentPosition();
     }


     public int getBufferedPercentage() {
         assert player != null;
         return player.getBufferedPercentage();
     }

     public long getBitRate() {
         assert player != null;
         return mBitrate;
     }

     private AnalyticsListener mAnalyticsListener = new AnalyticsListener() {

         @Override
         public void onTimelineChanged(EventTime eventTime, int reason) {
             KANTVLog.d(TAG, "onTimelineChanged");
             //For non-live sources this commonly is called only once

             if (reason == Player.TIMELINE_CHANGE_REASON_SOURCE_UPDATE) {
                 boolean isDynamic = player.isCurrentWindowDynamic();
                 boolean isLive    = player.isCurrentWindowLive();
                 //do stuff
             }
         }

         @Override
         public void onSeekStarted(EventTime eventTime) {
             KANTVLog.j(TAG, "onSeekStarted, event time:" + eventTime.currentPlaybackPositionMs);
         }

         @Override
         public void onSeekProcessed(EventTime eventTime) {
             KANTVLog.j(TAG, "onSeekProcessed");
         }

         @Override
         public void onPlaybackParametersChanged(EventTime eventTime, PlaybackParameters playbackParameters) {
             KANTVLog.j(TAG, "onPlaybackParametersChanged: " + eventTime.toString());
         }

         @Override
         public void onRepeatModeChanged(EventTime eventTime, int repeatMode) {
             KANTVLog.j(TAG, "onRepeatModeChanged");
         }

         @Override
         public void onShuffleModeChanged(EventTime eventTime, boolean shuffleModeEnabled) {
             KANTVLog.j(TAG, "onShuffleModeChanged");
         }

         @Override
         public void onLoadingChanged(EventTime eventTime, boolean isLoading) {
             KANTVLog.j(TAG, "onLoadingChanged: isLoading: " + isLoading
                     + " currentPlaybackPositionMs:" + eventTime.currentPlaybackPositionMs
                     + " currentWindowIndex:" + eventTime.currentWindowIndex
                     + " eventPlaybackPositionMs:" + eventTime.eventPlaybackPositionMs
                     + " totalBufferedDurationMs:" + eventTime.totalBufferedDurationMs
                     + " realtimeMs:" + eventTime.realtimeMs
             );
         }


         @Override
         public void onTracksChanged(EventTime eventTime, TrackGroupArray trackGroups, TrackSelectionArray trackSelections) {
             KANTVLog.j(TAG, "onTracksChanged");
         }


         @Override
         public void onBandwidthEstimate(EventTime eventTime, int totalLoadTimeMs, long totalBytesLoaded, long bitrateEstimate) {
             KANTVLog.d(TAG, "totalLoadTimeMs: " + totalLoadTimeMs + ",totalBytesLoaded: " + totalBytesLoaded + ",bandwidth:" + bitrateEstimate /  1024  + " kbps");
             mBitrate = bitrateEstimate;
             // The bandwidth meter calculate the estimated bandwidth based on the downloading/read speed.
             // It doesn't include the time on connection.
             // video variant selection is based on the calculated downloading bitrate
             // the bitrate here is the value used for video track selection.

             // EXOPlayer's bandwidth meter uses sliding window algorithm, which works good for us. In some cases,
             // depending on your media segment size, you may need to adjust the weight (setSlidingWindowMaxWeight()) to
             // get more accurate downloading bitrate.
         }

         @Override
         public void onSurfaceSizeChanged(EventTime eventTime, int width, int height) {
             KANTVLog.j(TAG, "onSurfaceSizeChanged: width x height = " + width + "x" + height);
         }


         @Override
         public void onDecoderInitialized(EventTime eventTime, int trackType, String decoderName, long initializationDurationMs) {
             KANTVLog.g(TAG, "onDecoderInitialized trackType: " + trackType + ",decoderName:" + decoderName);
             if (decoderName.contains("video")) {
                 mVideoDecoder = decoderName;
             }
             if (decoderName.contains("audio")) {
                 mAudioDecoder = decoderName;
             }
         }

         @Override
         public void onDecoderInputFormatChanged(EventTime eventTime, int trackType, Format format) {
             KANTVLog.j(TAG, "onDecoderInputFormatChanged format:" + format.toString());
         }

         @Override
         public void onAudioAttributesChanged(EventTime eventTime, AudioAttributes audioAttributes) {
             KANTVLog.j(TAG, "onAudioAttributesChanged");
         }

         @Override
         public void onVolumeChanged(EventTime eventTime, float volume) {
             KANTVLog.j(TAG, "onVolumeChanged");
         }

         @Override
         public void onAudioUnderrun(EventTime eventTime, int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
             /*
             Overrun(aka overflow):  the receiver hardware is unable to hand received data to a hardware buffer because the input rate
                                     exceeds the receivers ability to handle the data.

             A buffer overflow occurs when data written to a buffer, due to insufficient bounds checking, corrupts data values in memory
             addresses adjacent to the allocated buffer.

             Underrun(aka underflow): There are no data in the output queue.
             In computing, buffer underrun or buffer underflow is a state occurring when a buffer used to communicate
             between two devices or processes is fed with data at a lower speed than the data is being read from it.
             This requires the program or device reading from the buffer to pause its processing while the buffer refills.
             This can cause undesired and sometimes serious side effects, since the data being buffered is generally
             not suited to stop-start access of this kind.
             */
             KANTVLog.j(TAG, "bufferSize: " + bufferSize + ", bufferSizeMs: " + bufferSizeMs + ",elapsedSinceLastFeedMs: " + elapsedSinceLastFeedMs);
         }

         @Override
         public void onDroppedVideoFrames(EventTime eventTime, int droppedFrames, long elapsedMs) {
             KANTVLog.j(TAG, "droppedFrames : " + droppedFrames);
         }

         @Override
         public void onVideoSizeChanged(EventTime eventTime, int width, int height, int unappliedRotationDegrees, float pixelWidthHeightRatio) {
             KANTVLog.j(TAG, "onVideoSizeChanged: width x height = " + width + "x" + height);
         }

         @Override
         public void onDrmSessionAcquired(EventTime eventTime) {
             KANTVLog.j(TAG, "onDrmSessionAcquired");
             mDRMEnabled = true;
         }

         @Override
         public void onDrmKeysLoaded(EventTime eventTime) {
             KANTVLog.j(TAG, "drm enabled, it's encrypted content");
             mDRMEnabled = true;
         }

         @Override
         public void onDrmSessionManagerError(EventTime eventTime, Exception error) {
             KANTVLog.j(TAG, "onDrmSessionManagerError");
         }

         @Override
         public void onDrmKeysRestored(EventTime eventTime) {
             KANTVLog.j(TAG, "onDrmKeysRestored");
         }

         @Override
         public void onDrmKeysRemoved(EventTime eventTime) {
             KANTVLog.j(TAG, "onDrmKeysRemoved");
         }

         @Override
         public void onDrmSessionReleased(EventTime eventTime) {
             KANTVLog.j(TAG, "onDrmSessionReleased");
         }
     };


     private class PlayerEventListener implements Player.Listener {
         @Override
         public void onPlaybackStateChanged(@Player.State int playbackState) {
             KANTVLog.d(TAG, "enter onPlaybackStateChanged");
             KANTVLog.j(TAG, "playbackState: " + getStateString(playbackState));
             KANTVLog.d(TAG, "leave onPlaybackStateChanged");
             if (playbackState == Player.STATE_READY) {
                 mStartTimes = System.currentTimeMillis();
             } else {
                 if (player.getDuration() != 0 && player.getDuration() <= player.getCurrentPosition()) {
                     long Elapsedtimes = System.currentTimeMillis() - mStartTimes;
                     KANTVLog.j(TAG, "Playback has completed after " + Elapsedtimes / 1000 + " sec ");
                 } else {
                     //The player is not able to immediately play from its current position. This mostly happens because more data needs to be loaded.
                     KANTVLog.j(TAG, "Playback is buffering or paused automatically");
                     /*Create a custom load control which continuously keeps the buffer filled up based on max buffer — a technique called Drip-Feeding.*/
                 }
             }
             for (Listener listener : listeners) {
                 listener.onStateChanged(true, playbackState);
             }
         }


         @Override
         public void onRenderedFirstFrame() {
             KANTVLog.d(TAG, "onRenderedFirstFrame");
             for (Listener listener : listeners) {
                 listener.onRenderedFirstFrame();
             }
         }

         @Override
         public void onTimelineChanged(Timeline timeline, int reason) {
             KANTVLog.d(TAG, "enter onTimelineChanged");
             KANTVLog.d(TAG, "reason: " + reason);

             KANTVLog.d(TAG, "leave onTimelineChanged");
         }

         @Override
         public void onPositionDiscontinuity(
                 Player.PositionInfo oldPosition, Player.PositionInfo newPosition, @Player.DiscontinuityReason int reason) {
             KANTVLog.d(TAG, "enter onPositionDiscontinuity");
             if (reason == Player.DISCONTINUITY_REASON_AUTO_TRANSITION) {

             }
             KANTVLog.d(TAG, "leave onPositionDiscontinuity");
         }

         @Override
         public void onVideoSizeChanged(VideoSize videoSize) {
             KANTVLog.d(TAG, "onVideoSizeChanged : width " + videoSize.width + " height:" + videoSize.height);
             for (Listener listener : listeners) {
                 listener.onVideoSizeChanged(videoSize.width, videoSize.height, 0, 0);
             }
         }

         @Override
         public void onPlayerError(@NonNull PlaybackException error) {
             KANTVLog.d(TAG, "enter onPlayerError");
             KANTVLog.j(TAG, "player error: " + error.toString());
             KANTVLog.j(TAG, "player error code: " + error.errorCode);
             KANTVLog.j(TAG, "player error: " + error.getErrorCodeName());
             KANTVLog.j(TAG, "HLS type: " + KANTVUtils.getHLSPlaylistTypeString());

             boolean shouldCatchError = false;
             if (error.errorCode == PlaybackException.ERROR_CODE_BEHIND_LIVE_WINDOW) {
                 KANTVLog.j(TAG,"BehindLiveWindowException occurred");
                 shouldCatchError = true;
             }

             if (KANTVUtils.getHLSPlaylistType() == KANTVUtils.PLAYLIST_TYPE_LIVE) {
                 if (error.errorCode == PlaybackException.ERROR_CODE_PARSING_MANIFEST_MALFORMED) {
                     shouldCatchError = true;
                 }

                 if (error.errorCode == PlaybackException.ERROR_CODE_IO_UNSPECIFIED) {
                     KANTVLog.j(TAG,"Source error occurred with live content");
                     shouldCatchError = true;
                 }
             }

             if (shouldCatchError) {
                 // BehindLiveWindowException can occur when playing a rolling live stream in which segments become unavailable after a certain period of time.
                 // For example a rolling live stream might allow you to request segments that correspond to the most recent 2 minutes of the broadcast,
                 // but not earlier. In this case we'd say that the window is 2 minutes in length.
                 // BehindLiveWindowException is thrown if the player attempts to request a segment that's no longer available.
                 // This is typically caused by:
                 // (1)The window being unreasonably small. In this case the fix is to increase the window on the server side.
                 // (2)Repeated client buffering causing the client to be playing further and further behind the broadcast until
                 // it eventually ends up trying to request segments that are no longer available. In this case the fix might
                 // be on the server side if the buffering is due to a server issue. If it's not a server side issue, it's
                 // most likely just that the client doesn't have sufficient bandwidth. In this case preparing the player again
                 // will allow playback to resume. It should be possible to do this immediately (the 3 - 5 second you mention
                 // wait shouldn't be necessary). In previous releases something equivalent happened internally, but we opted
                 // to propagate the error and require that applications do this instead. This was for consistency reasons,
                 // and also because some applications may wish to do something different.

                 // most often the exception is caused by a combination of the two bullet points above. Repeated client buffering
                 // will only cause the exception if the accumulated duration of time spent buffering exceeds the window size
                 // minus the duration that the client wants to buffer locally (typically ~30 seconds).
                 // So the larger the window size, the less likely this is to occur.
                 // I would say that you'd want the window to be at least ~2 minutes.
                 // Window refers to (N-1)*(SegmentDuration), where SegmentDuration is 10s in your case,
                 // and N is the number of segments that are listed in each HLS playlist at any given point in time.
                 // I suspect you'll find that your HLS media playlists only contain 3 or 4 segments at any given point in time.
                 // Increasing that would help you to avoid this issue.
                 //
                 // You should request that the provider resolves the issue. Fundamentally you're trying to play a live stream
                 // with a live window of insufficient size. It's a content issue. It can only be properly resolved by the content provider.

                 // In live streams, it is risky for playback to be close to either end of the window.
                 // Being too close to the live edge might mean the player has to wait for a new segment to
                 // be appended to the playlist, which the user will perceive as a rebuffer.
                 // Being too close to the start of the window might cause the loading to fall behind,
                 // which will require a preparation of the player (look for BehindLiveWindowException in the demo app's PlayerActivity).
                 // Short live windows have two ill effects:
                 // (1)The playback position is never far from either edge.
                 // (2)They require much more frequent playlist refreshes.
                 // In addition to these points, long live windows allow the user to comfortably seek in the stream.
                 // We recommend a minimum length of one minute for live windows.
                 //
                 // When the stream is first played in ExoPlayer, playback is started at the default position (C.TIME_UNSET).
                 // This default position is resolved to an actual position in the live window by the media source (HlsMediaSource
                 // or DashMediaSource). This includes downloading the manifest (in case of DASH) or the master and media playlist
                 // (in case of HLS) to get the information needed for calculating the requested live latency and hence the start position.
                 // If playback starts directly with the CastPlayer, C.TIME_UNSET is simply translated to 0.

                 // Since 2.13.0, exoplayer start support low latency, that affect where to start live playback.
                 // Currently exoplayer logic is checking following in order:
                 // if user defined liveConfiguration.targetOffsetMs, use it
                 // if stream manifest defined serviceDescription, use its "latency target" value
                 // if stream manifest defined suggestedPresentationDelay, use it
                 // if non-above, use exoplayer default fallback that's 30 seconds.

                 // I'm assuming "end of it" you means the live-edge (most recent segments).
                 // ExoPlayer defaults to the 3 target-durations back from the live edge,
                 // this is from the Client Responsibilities section Section 6.3.3(https://tools.ietf.org/html/rfc8216#section-6.3.3) of the spec.
                 // So, with your example of 2 min window, assuming 6 second segments,
                 // and EXT-X-TARGETDURATION:6 you would have 20 segments in your window and ExoPlayer would start playing the 17th segment.
                 // The origin server may specify the EXT-X-START(https://tools.ietf.org/html/rfc8216#section-4.3.5.2) tag in the playlist if
                 // behavior other than the default is desired.
                 // If this is not what is happening, then please provide an example URL for triage.

                 // If the live stream you are serving is only going to add segments to the media playlist then you
                 // can also indicate that the playlist is an "event" type of playlist, using the
                 // EXT-X-PLAYLIST-TYPE tag and ExoPlayer will start playback from the start of the playlist.
                 // If the playlist is a sliding window (segments are removed from the start of the playlist),
                 // then playing from the start increases the likelihood of a BehindLiveWindowException,
                 // so please keep that in mind so as to handle it gracefully. There's plenty of material
                 // in this issue tracker about BehindLiveWindowException.

                 // PlaylistStuckException indicates an issue where the media playlist isn't being updated correctly on the server-side

                 // issues relative BLWE(BehindLiveWindowException)
                 // https://github.com/google/ExoPlayer/issues/1074
                 // https://github.com/google/ExoPlayer/issues/1782
                 // https://githubmate.com/repo/google/ExoPlayer/issues/8907
                 // https://github.com/google/ExoPlayer/issues/9044
                 // https://github.com/google/ExoPlayer/commit/b09b8dc2aba5f4b54d76a0b958c62aeb5e5b8075
                 // https://exoplayer.dev/live-streaming.html#behindlivewindowexception
                 // https://medium.com/google-exoplayer/hls-playback-in-exoplayer-a33959a47be7
                 // https://github.com/google/ExoPlayer/issues/9160
                 // https://github.com/google/ExoPlayer/issues/7975
                 // https://github.com/google/ExoPlayer/issues/8675
                 // https://github.com/google/ExoPlayer/issues/7531
                 // https://github.com/google/ExoPlayer/pull/9386
                 // https://github.com/google/ExoPlayer/issues/9122
                 // https://github.com/google/ExoPlayer/issues/8218
                 // https://github.com/google/ExoPlayer/issues/8560
                 // https://github.com/google/ExoPlayer/issues/7708
                 // https://github.com/google/ExoPlayer/issues/7658
                 // https://github.com/google/ExoPlayer/issues/7523
                 // https://github.com/google/ExoPlayer/issues/7711

                 // re-initialize player at the live edge, workaround for this exception need to more test for any side effect
                 // If you want to start at the beginning of the live stream window, you can also specifically
                 // call player.seekTo(0) (or any other position) before player.prepare().
                 // The `setLiveTargetOffsetMs()` does exactly what we would want. Careful with
                 // seeking to position 0 in a live playlist, you’ll need to gracefully deal
                 // with `BenindLiveWindowException` in that case
                 // highly cost as following(shouldn't be used in commercial app):
                 //if (isBehindLiveWindow(error)) {
                 //    try {
                 //        reInitPlayer()
                 //    } catch (e : Exception) {
                 //        e.printStackTrace()
                 //    }
                 //}
                 //fun reInitPlayer() {
                 //    initPlayer()         //    here is initializePlayer()
                 //    setMediaSource(url)  //    here is prepare()
                 //}
                 // gracefully as following:
                 // according to https://github.com/google/ExoPlayer/issues/7975
                 // replace
                 // player.seekToDefaultPosition();
                 // to
                 player.seekTo(1);
                 player.prepare();
                 return;
             }

             for (Listener listener : listeners) {
                 listener.onError(error);
             }
             KANTVLog.d(TAG, "leave onPlayerError");
         }

         @Override
         @SuppressWarnings("ReferenceEquality")
         public void onTracksChanged(
                 TrackGroupArray trackGroups, TrackSelectionArray trackSelections) {
             KANTVLog.j(TAG, "enter onTracksChanged");
             if (trackGroups != lastSeenTrackGroupArray) {
                 MappedTrackInfo mappedTrackInfo = trackSelector.getCurrentMappedTrackInfo();
                 if (mappedTrackInfo != null) {
                     if (mappedTrackInfo.getTypeSupport(C.TRACK_TYPE_VIDEO)
                             == MappedTrackInfo.RENDERER_SUPPORT_UNSUPPORTED_TRACKS) {
                         //showToast(R.string.error_unsupported_video);
                     }
                     if (mappedTrackInfo.getTypeSupport(C.TRACK_TYPE_AUDIO)
                             == MappedTrackInfo.RENDERER_SUPPORT_UNSUPPORTED_TRACKS) {
                         //showToast(R.string.error_unsupported_audio);
                     }
                 }
                 lastSeenTrackGroupArray = trackGroups;
             }
             for (Listener listener : listeners) {
                 listener.onTracksChanged((Object)trackSelector, (Object) trackGroups, (Object) trackSelections);
             }
             KANTVLog.j(TAG, "leave onTracksChanged");
         }
     }

     private class PlayerErrorMessageProvider implements ErrorMessageProvider<ExoPlaybackException> {

         @Override
         public Pair<Integer, String> getErrorMessage(ExoPlaybackException e) {
             KANTVLog.d(TAG, "enter getErrorMessage");
             String errorString = "Playback failed";
             if (e.type == ExoPlaybackException.TYPE_RENDERER) {
                 Exception cause = e.getRendererException();
                 if (cause instanceof DecoderInitializationException) {
                     // Special case for decoder initialization failures.
                     DecoderInitializationException decoderInitializationException =
                             (DecoderInitializationException) cause;
                     if (decoderInitializationException.codecInfo == null) {
                         if (decoderInitializationException.getCause() instanceof DecoderQueryException) {
                             errorString = "Unable to query device decoders";
                         } else if (decoderInitializationException.secureDecoderRequired) {
                             errorString = "This device does not provide a secure decoder";

                         } else {
                             errorString = "This device does not provide a decoder";
                         }
                     } else {
                         errorString = "Unable to instantiate decoder";
                     }
                 }
             }
             KANTVLog.d(TAG, "error: " + errorString);
             KANTVLog.d(TAG, "leave getErrorMessage");
             return Pair.create(0, errorString);
         }
     }

     public static String getStateString(int state) {
         switch (state) {
             case ExoPlayer.STATE_BUFFERING:
                 return "STATE_BUFFERING";
             case ExoPlayer.STATE_ENDED:
                 return "STATE_ENDED";
             case ExoPlayer.STATE_IDLE:
                 return "STATE_IDLE";
             case ExoPlayer.STATE_READY:
                 return "STATE_READY";
             default:
                 return "?";
         }
     }

     /**
      * A listener for core events.
      */
     public interface Listener {
         void onStateChanged(boolean playWhenReady, int playbackState);

         void onError(Exception e);

         void onVideoSizeChanged(int width, int height, int unappliedRotationDegrees,
                                 float pixelWidthHeightRatio);

         void onRenderedFirstFrame();

         void onTracksChanged(Object trackSelector, Object trackGroups, Object trackSelections);
     }

     /**
      * A listener for internal errors.
      * <p>
      * These errors are not visible to the user, and hence this listener is provided for
      * informational purposes only. Note however that an internal error may cause a fatal
      * error if the player fails to recover. If this happens, {link Listener#onError(Exception)}
      * will be invoked.
      */
     public interface InternalErrorListener {
         void onRendererInitializationError(Exception e);


         void onAudioTrackUnderrun(int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs);

         void onDecoderInitializationError(DecoderInitializationException e);

         void onCryptoError(MediaCodec.CryptoException e);

         void onLoadError(int sourceId, IOException e);

         void onDrmSessionManagerError(Exception e);
     }

     /**
      * A listener for debugging information.
      */
     public interface InfoListener {
         void onVideoFormatEnabled(Format format, int trigger, long mediaTimeMs);

         void onAudioFormatEnabled(Format format, int trigger, long mediaTimeMs);

         void onDroppedFrames(int count, long elapsed);

         void onBandwidthSample(int elapsedMs, long bytes, long bitrateEstimate);

         void onLoadStarted(int sourceId, long length, int type, int trigger, Format format,
                            long mediaStartTimeMs, long mediaEndTimeMs);

         void onLoadCompleted(int sourceId, long bytesLoaded, int type, int trigger, Format format,
                              long mediaStartTimeMs, long mediaEndTimeMs, long elapsedRealtimeMs, long loadDurationMs);

         void onDecoderInitialized(String decoderName, long elapsedRealtimeMs,
                                   long initializationDurationMs);

     }

     public static native int kantv_anti_remove_rename_this_file();
 }
