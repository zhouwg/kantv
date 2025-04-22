package com.kantvai.kantvplayer.player.common.utils;

import android.text.TextUtils;

import com.blankj.utilcode.util.StringUtils;



import java.util.ArrayList;
import java.util.List;
import java.util.Locale;


import kantvai.media.player.KANTVLog;
import kantvai.media.player.bean.ExoTrackInfoBean;
import kantvai.media.player.bean.IJKTrackInfoBean;
import kantvai.media.player.bean.TrackInfoBean;
import kantvai.media.player.misc.ITrackInfo;
import kantvai.media.player.misc.IjkTrackInfo;

public class TrackInfoUtils {
    private static final String TAG = TrackInfoUtils.class.getName();
    private List<TrackInfoBean> audioTrackList;
    private List<TrackInfoBean> subTrackList;

    public TrackInfoUtils(){
        audioTrackList = new ArrayList<>();
        subTrackList = new ArrayList<>();
    }

    public void initTrackInfo(ITrackInfo[] trackInfo, int selectedAudioTrack, int selectedSubTrack){
        subTrackList.clear();
        audioTrackList.clear();

        int audioCount = 1;
        int subCount = 1;
        for (int i = 0; i < trackInfo.length; i++) {
            if (trackInfo[i].getTrackType() == IjkTrackInfo.MEDIA_TRACK_TYPE_AUDIO) {
                IJKTrackInfoBean trackInfoBean = new IJKTrackInfoBean();
                String title = trackInfo[i].getTitle();
                String language = trackInfo[i].getLanguage();
                String codecName = trackInfo[i].getCodecName();
                String audioTrackText = String.format(Locale.CHINESE,"#%d：%s[%s, %s]", audioCount, title, language, codecName);

                trackInfoBean.setStreamId(i);
                trackInfoBean.setName(audioTrackText);
                if (i == selectedAudioTrack)
                    trackInfoBean.setSelect(true);
                audioCount++;
                audioTrackList.add(trackInfoBean);
            } else if (trackInfo[i].getTrackType() == IjkTrackInfo.MEDIA_TRACK_TYPE_TIMEDTEXT){
                IJKTrackInfoBean trackInfoBean = new IJKTrackInfoBean();
                String title = trackInfo[i].getTitle();
                String codecName = trackInfo[i].getCodecName();
                String subTrackText = String.format(Locale.CHINESE,"#%d：%s[%s]", subCount, title, codecName);

                trackInfoBean.setStreamId(i);
                trackInfoBean.setName(subTrackText);
                if (i == selectedSubTrack)
                    trackInfoBean.setSelect(true);
                subCount++;
                subTrackList.add(trackInfoBean);
            }
        }
    }

    /*
    public void initTrackInfo(DefaultTrackSelector trackSelector, TrackSelectionArray trackSelections){
        subTrackList.clear();
        audioTrackList.clear();

        String audioId = "";
        String subtitleId = "";
        for (TrackSelection selection : trackSelections.getAll()) {
            if (selection == null) continue;
            KANTVLog.j(TAG, "track: " + selection.toString());
            Format selectionFormat = selection.getFormat(0);
            if (MimeTypes.isAudio(selectionFormat.sampleMimeType)) {
                audioId = selectionFormat.id;
                continue;
            }
            if (MimeTypes.isText(selectionFormat.sampleMimeType)) {
                subtitleId = selectionFormat.id;
            }
        }

        MappingTrackSelector.MappedTrackInfo mappedTrackInfo = trackSelector.getCurrentMappedTrackInfo();
        if (mappedTrackInfo != null) {
            int renderCount = mappedTrackInfo.getRendererCount();
            for (int i = 0; i < renderCount; i++) {
                TrackGroupArray trackGroupArray = mappedTrackInfo.getTrackGroups(i);
                for (int j = 0; j < trackGroupArray.length; j++) {
                    TrackGroup trackGroup = trackGroupArray.get(j);
                    for (int k = 0; k < trackGroup.length; k++) {
                        Format format = trackGroup.getFormat(k);
                        String label = TextUtils.isEmpty(format.label) ? "und" : format.label;
                        String language = TextUtils.isEmpty(format.language) ? "und" : format.language;
                        String miniType = TextUtils.isEmpty(format.sampleMimeType) ? "und" : format.sampleMimeType;
                        if (MimeTypes.isAudio(format.sampleMimeType)) {
                            ExoTrackInfoBean trackInfoBean = new ExoTrackInfoBean();
                            String audioTrackText = String.format(Locale.CHINESE, "#%d：%s[%s]", audioTrackList.size() + 1, label, language);
                            trackInfoBean.setName(audioTrackText);
                            trackInfoBean.setRenderId(i);
                            trackInfoBean.setTrackGroupId(j);
                            trackInfoBean.setTrackId(k);
                            if (!StringUtils.isEmpty(audioId) && audioId.equals(format.id))
                                trackInfoBean.setSelect(true);
                            audioTrackList.add(trackInfoBean);
                        } else if (MimeTypes.isText(format.sampleMimeType)) {
                            ExoTrackInfoBean trackInfoBean = new ExoTrackInfoBean();
                            String subtitleTrackText = String.format(Locale.CHINESE, "#%d：%s[%s]", subTrackList.size() + 1, label, miniType);
                            trackInfoBean.setName(subtitleTrackText);
                            trackInfoBean.setRenderId(i);
                            trackInfoBean.setTrackGroupId(j);
                            trackInfoBean.setTrackId(k);
                            if (!StringUtils.isEmpty(subtitleId) && subtitleId.equals(format.id))
                                trackInfoBean.setSelect(true);
                            subTrackList.add(trackInfoBean);
                        }
                    }
                }
            }
        }
    }
    */

    public List<TrackInfoBean> getAudioTrackList() {
        return audioTrackList;
    }

    public List<TrackInfoBean> getSubTrackList() {
        return subTrackList;
    }
}
