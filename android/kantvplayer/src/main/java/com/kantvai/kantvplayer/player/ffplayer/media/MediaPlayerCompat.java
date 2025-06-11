/*
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
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

package com.kantvai.kantvplayer.player.ffplayer.media;


import kantvai.media.player.FFmpegMediaPlayer;
import kantvai.media.player.IMediaPlayer;
import kantvai.media.player.MediaPlayerProxy;
import kantvai.media.player.TextureMediaPlayer;


public class MediaPlayerCompat {

    public static String getName(IMediaPlayer mp) {
        if (mp == null) {
            return "null";
        } else if (mp instanceof TextureMediaPlayer) {
            StringBuilder sb = new StringBuilder("TextureMediaPlayer <");
            IMediaPlayer internalMediaPlayer = ((TextureMediaPlayer) mp).getInternalMediaPlayer();
            if (internalMediaPlayer == null) {
                sb.append("null>");
            } else {
                sb.append(internalMediaPlayer.getClass().getSimpleName());
                sb.append(">");
            }
            return sb.toString();
        } else {
            return mp.getClass().getSimpleName();
        }
    }

    public static FFmpegMediaPlayer getIjkMediaPlayer(IMediaPlayer mp) {
        FFmpegMediaPlayer ijkMediaPlayer = null;
        if (mp == null) {
            return null;
        }
        if (mp instanceof FFmpegMediaPlayer) {
            ijkMediaPlayer = (FFmpegMediaPlayer) mp;
        } else if (mp instanceof MediaPlayerProxy && ((MediaPlayerProxy) mp).getInternalMediaPlayer() instanceof FFmpegMediaPlayer) {
            ijkMediaPlayer = (FFmpegMediaPlayer) ((MediaPlayerProxy) mp).getInternalMediaPlayer();
        }
        return ijkMediaPlayer;
    }

    public static void selectTrack(IMediaPlayer mp, int stream) {
        FFmpegMediaPlayer ijkMediaPlayer = getIjkMediaPlayer(mp);
        if (ijkMediaPlayer == null)
            return;
        ijkMediaPlayer.selectTrack(stream);
    }

    public static void deselectTrack(IMediaPlayer mp, int stream) {
        FFmpegMediaPlayer ijkMediaPlayer = getIjkMediaPlayer(mp);
        if (ijkMediaPlayer == null)
            return;
        ijkMediaPlayer.deselectTrack(stream);
    }

    public static int getSelectedTrack(IMediaPlayer mp, int trackType) {
        FFmpegMediaPlayer ijkMediaPlayer = getIjkMediaPlayer(mp);
        if (ijkMediaPlayer == null)
            return -1;
        return ijkMediaPlayer.getSelectedTrack(trackType);
    }
}
