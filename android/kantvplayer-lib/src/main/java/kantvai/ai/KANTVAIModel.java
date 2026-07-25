/*
 * Copyright (c) 2024- KanTV Authors
 */
package kantvai.ai;

import kantvai.media.player.KANTVLog;

public class KANTVAIModel {
     public enum AIModelType {
         TYPE_ASR,
         TYPE_TEXT2IMAGE,
         TYPE_LLM,
     };
    private static final String TAG = KANTVAIModel.class.getSimpleName();
    private int index;

    private AIModelType type;
    private String nickname;    //model's short name
    private String name;        //model's full name
    private String mmproj_name; //mmproj model name
    private String url;         //original url of model

    private String mmproj_url;  //original url of mmproj model

    private String quality;     //quality of model on Android phone

    private String sample_name;
    private String sample_url;

    private boolean downloadAble;

    public KANTVAIModel(int index, AIModelType type, String nick, String name, String url) {
        this.index = index;
        this.type = type;
        this.nickname  = nick;
        this.name  = name;
        this.url   = url;
        this.downloadAble = true;
    }

     public KANTVAIModel(int index, AIModelType type, String nick, String name, String mmprojName, String url, String mmprojUrl) {
         this(index, type, nick, name, url);
         this.mmproj_name = mmprojName;
         this.mmproj_url  = mmprojUrl;
         KANTVLog.j(TAG, "init");
     }

    public String getNickname() { return nickname; }
    public String getName() {
        return name;
    }

    public String getMMProjName() { return mmproj_name; }
    public String getMMProjUrl() { return mmproj_url; }
    public void setMMprojUrl(String mmprojUrl) { this.mmproj_url = mmprojUrl; }

    public String getUrl() {
        return url;
    }
    public void setUrl(String modelUrl) { this.url = modelUrl;}

    public int getIndex() {
        return index;
    }

    public String getQuality() { return quality; }

    public void setQuality(String quality) {
        this.quality = quality;
    }

    public void setSample(String sampleName, String url) {
        this.sample_name = sampleName;
        this.sample_url = url;
    }

    public boolean isDownloadAble() {
        return downloadAble;
    }
}
