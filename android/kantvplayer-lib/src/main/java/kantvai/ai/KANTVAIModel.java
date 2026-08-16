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

    /**
     * Chat template components. Used by ChatAdapter.getHistoryPrompt() to
     * fold a list of ChatMessage(role, content) into a single string that
     * the model expects.
     *
     * The components are intentionally separate (not a single template
     * string) because each model has different turn delimiters and the
     * Java side needs to iterate over an arbitrary-length message list
     * and produce a deterministic concatenation.
     *
     * Defaults are the simplest possible "User: ...\nAssistant: ...\n"
     * plain-text format that works for any base model but does not
     * match the official chat template for any specific LLM. Callers
     * (KANTVAIModelMgr) should override these for each registered model
     * so the formatted prompt matches the model's expected format.
     *
     * Example for Gemma-3:
     *   bos = "<bos>"
     *   userOpen = "<start_of_turn>user\n"
     *   userClose = "<end_of_turn>\n"
     *   modelOpen = "<start_of_turn>model\n"
     *   modelClose = "<end_of_turn>\n"
     *   generationPrompt = "<start_of_turn>model\n"
     *
     * Example for Qwen2.5 (ChatML):
     *   bos = ""
     *   userOpen = "<|im_start|>user\n"
     *   userClose = "<|im_end|>\n"
     *   modelOpen = "<|im_start|>assistant\n"
     *   modelClose = "<|im_end|>\n"
     *   generationPrompt = "<|im_start|>assistant\n"
     */
    private String bos               = "";
    private String userOpen          = "User: ";
    private String userClose         = "\n";
    private String modelOpen         = "Assistant: ";
    private String modelClose        = "\n";
    private String generationPrompt  = "Assistant: ";

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

    // ---------------------------------------------------------------------
    // Chat template (multi-turn prompt formatting)
    // ---------------------------------------------------------------------

    /**
     * Set all 6 chat template components in one call. See
     * {@link #bos}..{@link #generationPrompt} for the meaning.
     */
    public void setChatTemplate(String bos, String userOpen, String userClose,
                                String modelOpen, String modelClose,
                                String generationPrompt) {
        if (bos != null)              this.bos              = bos;
        if (userOpen != null)         this.userOpen         = userOpen;
        if (userClose != null)        this.userClose        = userClose;
        if (modelOpen != null)        this.modelOpen        = modelOpen;
        if (modelClose != null)       this.modelClose       = modelClose;
        if (generationPrompt != null) this.generationPrompt = generationPrompt;
    }

    public String getBos()              { return bos; }
    public String getUserOpen()         { return userOpen; }
    public String getUserClose()        { return userClose; }
    public String getModelOpen()        { return modelOpen; }
    public String getModelClose()       { return modelClose; }
    public String getGenerationPrompt() { return generationPrompt; }
}
