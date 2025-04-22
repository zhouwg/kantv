package com.kantvai.kantvplayer.player.subtitle;

import androidx.annotation.NonNull;

import com.blankj.utilcode.util.ConvertUtils;
import com.kantvai.kantvplayer.player.subtitle.util.Caption;
import com.kantvai.kantvplayer.player.subtitle.util.TimedTextObject;

import java.util.ArrayList;
import java.util.List;
import java.util.SortedMap;


public class SubtitleManager {
    private boolean showExternalSubtitle = false;
    private int textSize = 40;

    private SubtitleView subtitleView;
    private TimedTextObject subtitleData;

    public SubtitleManager(@NonNull SubtitleView subtitleView) {
        this.subtitleView = subtitleView;
    }

    public void setExSubData(TimedTextObject subtitleData) {
        this.subtitleData = subtitleData;
    }

    public void hideExSub() {
        if (showExternalSubtitle) {
            showExternalSubtitle = false;
            clearSubtitle();
        }
    }

    public void showExSub() {
        if (!showExternalSubtitle) {
            showExternalSubtitle = true;
            clearSubtitle();
        }
    }

    public void seekExSubTo(long duration) {
        if (!showExternalSubtitle) {
            return;
        }
        if (subtitleData != null && !subtitleData.captions.isEmpty()) {
            List<Caption> captionList = searchSubtitle(duration);
            if (captionList != null && captionList.size() > 0) {
                StringBuilder content = new StringBuilder();
                for (Caption caption : captionList) {
                    content.append(caption.content.replace("<br />", "\n")).append("\n");
                }
                SubtitleUtils.showSubtitle(subtitleView, content.substring(0, content.length() - 1), textSize);
            } else {
                clearSubtitle();
            }
        }
    }

    public void setInnerSub(String subtitle) {
        if (showExternalSubtitle) {
            return;
        }
        SubtitleUtils.showSubtitle(subtitleView, subtitle, textSize);
    }

    public void setTextSizeProgress(int textSizeProgress) {
        this.textSize = (int) (((float) textSizeProgress / 100) * ConvertUtils.dp2px(36));
    }

    public boolean isShowExternalSubtitle() {
        return showExternalSubtitle;
    }

    private void clearSubtitle() {
        SubtitleUtils.showSubtitle(subtitleView, "", textSize);
    }

    private List<Caption> searchSubtitle(long duration) {
        List<Caption> captionList = new ArrayList<>();
        try {
            long min = subtitleData.captions.firstKey();
            long max = subtitleData.captions.lastKey();
            if (duration > min) {
                long start = duration - 10 * 1000 < min
                        ? subtitleData.captions.firstKey()
                        : subtitleData.captions.lowerKey(duration - 10 * 1000);
                SortedMap<Long, Caption> temp = subtitleData.captions.subMap(start, max);
                for (Long key1 : temp.keySet()) {
                    Caption caption = temp.get(key1);
                    if (caption == null)
                        return null;
                    if (duration - caption.start.getMseconds() >= -1 && duration <= caption.end.getMseconds()) {
                        captionList.add(caption);
                    }
                    if (caption.start.getMseconds() > duration + 1) {
                        break;
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return captionList;
    }
}
