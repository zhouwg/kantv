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
package kantvai.media.exo2;

import android.media.AudioTrack;
import android.media.MediaCodec;
import android.os.Build;
import android.os.SystemClock;
import android.util.Log;

import com.google.android.exoplayer2.ExoPlayer;
import com.google.android.exoplayer2.Format;
import com.google.android.exoplayer2.analytics.AnalyticsListener;
import com.google.android.exoplayer2.mediacodec.MediaCodecRenderer;
import com.google.android.exoplayer2.source.TrackGroupArray;
import com.google.android.exoplayer2.trackselection.TrackSelectionArray;

import java.io.IOException;
import java.text.NumberFormat;
import java.util.Locale;

import androidx.annotation.RequiresApi;
import kantvai.media.player.KANTVLog;


public class DemoPlayerEvent implements DemoPlayer.Listener, DemoPlayer.InfoListener,
        DemoPlayer.InternalErrorListener {
    private static final String TAG = DemoPlayerEvent.class.getName();
    private static final NumberFormat TIME_FORMAT;
    static {
        TIME_FORMAT = NumberFormat.getInstance(Locale.US);
        TIME_FORMAT.setMinimumFractionDigits(2);
        TIME_FORMAT.setMaximumFractionDigits(2);
    }

    private long sessionStartTimeMs;
    private long sessionEndTimeMs;
    private long[] loadStartTimeMs;

    public DemoPlayerEvent() {
        loadStartTimeMs = new long[DemoPlayer.RENDERER_COUNT];
    }

    public void startSession() {
        sessionStartTimeMs = SystemClock.elapsedRealtime();
        KANTVLog.d(TAG, "start [session] ");
    }

    public void endSession() {
        sessionEndTimeMs = SystemClock.elapsedRealtime();
        KANTVLog.d(TAG, "end [session " + getSessionTimeString() + "]");
    }


    @Override
    public void onStateChanged(boolean playWhenReady, int state) {
        KANTVLog.d(TAG, "state [" + getSessionTimeString() + ", " + playWhenReady + ", "
                + DemoPlayer.getStateString(state) + "]");
    }

    @Override
    public void onRenderedFirstFrame() {
        KANTVLog.d(TAG, "onRenderedFirstFrame");
    }

    @Override
    public void onError(Exception e) {
        KANTVLog.e(TAG, "playerFailed [" + getSessionTimeString() + "]" +  e.toString());
    }

    @Override
    public void onVideoSizeChanged(int width, int height, int unappliedRotationDegrees,
                                   float pixelWidthHeightRatio) {
        KANTVLog.d(TAG, "videoSizeChanged [" + width + ", " + height + ", " + unappliedRotationDegrees
                + ", " + pixelWidthHeightRatio + "]");
    }

    @Override
    public void onTracksChanged(Object trackSelector, Object trackGroups, Object trackSelections) {
        KANTVLog.d(TAG, "onTracksChanged");
    }


    @Override
    public void onBandwidthSample(int elapsedMs, long bytes, long bitrateEstimate) {
        KANTVLog.d(TAG, "bandwidth [" + getSessionTimeString() + ", " + bytes + ", "
                + getTimeString(elapsedMs) + ", " + bitrateEstimate + "]");
    }

    @Override
    public void onVideoFormatEnabled(Format format, int trigger, long mediaTimeMs) {

    }

    @Override
    public void onDroppedFrames(int count, long elapsed) {
        Log.d(TAG, "droppedFrames [" + getSessionTimeString() + ", " + count + "]");
    }

    @Override
    public void onLoadStarted(int sourceId, long length, int type, int trigger, Format format,
                              long mediaStartTimeMs, long mediaEndTimeMs) {
        loadStartTimeMs[sourceId] = SystemClock.elapsedRealtime();
        KANTVLog.v(TAG, "loadStart [" + getSessionTimeString() + ", " + sourceId + ", " + type
                    + ", " + mediaStartTimeMs + ", " + mediaEndTimeMs + "]");
    }

    @Override
    public void onLoadCompleted(int sourceId, long bytesLoaded, int type, int trigger, Format format,
                                long mediaStartTimeMs, long mediaEndTimeMs, long elapsedRealtimeMs, long loadDurationMs) {

            long downloadTime = SystemClock.elapsedRealtime() - loadStartTimeMs[sourceId];
            KANTVLog.v(TAG, "loadEnd [" + getSessionTimeString() + ", " + sourceId + ", " + downloadTime + "]");
    }


    @Override
    public void onAudioFormatEnabled(Format format, int trigger, long mediaTimeMs) {
        KANTVLog.d(TAG, "audioFormat [" + getSessionTimeString() + ", " + format.id + ", "
                + Integer.toString(trigger) + "]");
    }


    @Override
    public void onLoadError(int sourceId, IOException e) {
        printInternalError("loadError", e);
    }

    @Override
    public void onRendererInitializationError(Exception e) {
        printInternalError("rendererInitError", e);
    }

    @Override
    public void onDrmSessionManagerError(Exception e) {
        printInternalError("drmSessionManagerError", e);
    }


    @Override
    public void onAudioTrackUnderrun(int bufferSize, long bufferSizeMs, long elapsedSinceLastFeedMs) {
        printInternalError("audioTrackUnderrun [" + bufferSize + ", " + bufferSizeMs + ", "
                + elapsedSinceLastFeedMs + "]", null);
    }

    @Override
    public void onDecoderInitializationError(MediaCodecRenderer.DecoderInitializationException e) {
        printInternalError("decoderInitializationError", e);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
    @Override
    public void onCryptoError(MediaCodec.CryptoException e) {
        printInternalError("cryptoError", e);
    }

    @Override
    public void onDecoderInitialized(String decoderName, long elapsedRealtimeMs, long initializationDurationMs) {
        KANTVLog.d(TAG, "decoderInitialized [" + getSessionTimeString() + ", " + decoderName + "]");
    }



    private void printInternalError(String type, Exception e) {
        KANTVLog.e(TAG, "internalError [" + getSessionTimeString() + ", " + type + "]" + e.toString());
    }



    private String getSessionTimeString() {
        return getTimeString(SystemClock.elapsedRealtime() - sessionStartTimeMs);
    }

    private String getTimeString(long timeMs) {
        return TIME_FORMAT.format((timeMs) / 1000f);
    }

    public static native int kantv_anti_remove_rename_this_file();
}
