/*
 * Copyright (c) 2024- KanTV Authors
 */
package kantvai.ai;

import kantvai.media.player.KANTVLog;

public class KANTVAIModel {
     public enum AIModelType {
         TYPE_ASR,
         TYPE_LLM,
     };
    /**
     * Modality bitmask. A model can advertise any combination of text / image / audio
     * support. Default is text-only so that legacy code paths that never set the
     * modality still get a sane value.
     */
    public static final int MODALITY_TEXT  = 1 << 0;   //1
    public static final int MODALITY_IMAGE = 1 << 1;   //2
    public static final int MODALITY_AUDIO = 1 << 2;   //4
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

    /** Modality bitmask, default text-only. */
    private int modality = MODALITY_TEXT;

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

    // ---------------------------------------------------------------------
    // Modality (text / image / audio) - bitmask API
    // ---------------------------------------------------------------------

    /**
     * Set the modality bitmask. Caller passes a bitwise OR of
     * {@link #MODALITY_TEXT}, {@link #MODALITY_IMAGE}, {@link #MODALITY_AUDIO}.
     */
    public void setModality(int modality) {
        KANTVLog.j(TAG, "setModality " + modality + " for " + (nickname != null ? nickname : ""));
        this.modality = modality;
    }

    /** @return the raw modality bitmask. */
    public int getModality() {
        return modality;
    }

    public boolean supportsText()  { return (modality & MODALITY_TEXT)  != 0; }
    public boolean supportsImage() { return (modality & MODALITY_IMAGE) != 0; }
    public boolean supportsAudio() { return (modality & MODALITY_AUDIO) != 0; }

    /**
     * Human-readable tag for display in preference dropdowns / chat UI.
     * Examples: {@code [text]}, {@code [text+image]}, {@code [text+image+audio]}.
     * Order is fixed (text, image, audio) so users can scan the list
     * consistently.
     */
    public String getModalityTag() {
        java.util.List<String> parts = new java.util.ArrayList<>(3);
        if (supportsText())  parts.add("text");
        if (supportsImage()) parts.add("image");
        if (supportsAudio()) parts.add("audio");
        return "[" + String.join("+", parts) + "]";
    }
}
