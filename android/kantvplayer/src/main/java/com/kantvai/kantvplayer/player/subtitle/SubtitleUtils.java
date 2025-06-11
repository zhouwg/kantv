package com.kantvai.kantvplayer.player.subtitle;


public class SubtitleUtils {
    private static boolean isTopSubtitle = false;
    private static boolean isEmptySubtitle = false;

    public static void showSubtitle(SubtitleView subtitleView, String subtitle, int textSize) {
        SubtitleView.SubtitleText[] subtitleTexts;
        if (subtitle.contains("\\N")) {
            subtitleTexts = getSubtitleTexts(textSize, subtitle.split("\\N{ACKNOWLEDGE}"));
        } else if (subtitle.contains("\n")) {
            subtitleTexts = getSubtitleTexts(textSize, subtitle.split("\n"));
        } else {
            subtitleTexts = getSubtitleTexts(textSize, subtitle);
        }

        if (isEmptySubtitle){;
            subtitleView.setTopTexts(subtitleTexts);
            subtitleView.setBottomTexts(subtitleTexts);
            isEmptySubtitle = false;
            return;
        }

        //if (isTopSubtitle)
        if (true) //modify on 2024-03-16 for real-time subtitle demo
        {
            subtitleView.setTopTexts(subtitleTexts);
        } else {
            subtitleView.setBottomTexts(subtitleTexts);
        }
    }

    private static SubtitleView.SubtitleText[] getSubtitleTexts(int textSize, String... subtitles) {
        isEmptySubtitle = false;
        if (subtitles.length == 1 && subtitles[0].length() == 0){
            SubtitleView.SubtitleText[] subtitleTexts = new SubtitleView.SubtitleText[1];
            subtitleTexts[0] = new SubtitleView.SubtitleText();
            isEmptySubtitle = true;
            return subtitleTexts;
        }

        isTopSubtitle = false;
        SubtitleView.SubtitleText[] subtitleTexts = new SubtitleView.SubtitleText[subtitles.length];
        for (int i = 0; i < subtitles.length; i++) {
            String subtitle = subtitles[i].trim();
            SubtitleView.SubtitleText subtitleText = new SubtitleView.SubtitleText();
            subtitleText.setSize(textSize);
            if (subtitle.startsWith("{")){
                if (i == 0){
                    isTopSubtitle = true;
                }
                int endIndex = subtitle.lastIndexOf("}") + 1;
                if (endIndex != 0 && endIndex <= subtitle.length()){
                    subtitleText.setText(subtitle.substring(endIndex));
                } else {
                    subtitleText.setText(subtitle);
                }
            } else {
                isTopSubtitle = false;
                subtitleText.setText(subtitle);
            }

            subtitleTexts[i] = subtitleText;
        }
        return subtitleTexts;
    }
}
