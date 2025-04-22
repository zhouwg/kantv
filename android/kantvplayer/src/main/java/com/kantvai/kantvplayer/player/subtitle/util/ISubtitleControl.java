package com.kantvai.kantvplayer.player.subtitle.util;

import android.widget.TextView;


public interface ISubtitleControl
{

    void setData(TimedTextObject model);

    void setItemSubtitle(TextView view, String item);

    void setLanguage(int type);

    void start();

    void pause();

    void seekTo(long position);


    void stop();


    void setPlayOnBackground(boolean pb);


    void setTextSize(int languageType, float chineseSize);


    void setTextSize(float chineseSize, float englishSize);


    void hide();


    void show();
}
