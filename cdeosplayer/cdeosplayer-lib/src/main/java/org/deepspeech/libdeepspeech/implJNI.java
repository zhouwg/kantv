
package org.deepspeech.libdeepspeech;

public class implJNI {
    public implJNI() {
    }

    public static final native long new_modelstatep();

    public static final native long copy_modelstatep(long var0);

    public static final native void delete_modelstatep(long var0);

    public static final native void modelstatep_assign(long var0, long var2);

    public static final native long modelstatep_value(long var0);

    public static final native long new_streamingstatep();

    public static final native long copy_streamingstatep(long var0);

    public static final native void delete_streamingstatep(long var0);

    public static final native void streamingstatep_assign(long var0, long var2);

    public static final native long streamingstatep_value(long var0);

    public static final native String TokenMetadata_Text_get(long var0, TokenMetadata var2);

    public static final native long TokenMetadata_Timestep_get(long var0, TokenMetadata var2);

    public static final native float TokenMetadata_StartTime_get(long var0, TokenMetadata var2);

    public static final native long CandidateTranscript_NumTokens_get(long var0, CandidateTranscript var2);

    public static final native double CandidateTranscript_Confidence_get(long var0, CandidateTranscript var2);

    public static final native long CandidateTranscript_getToken(long var0, CandidateTranscript var2, int var3);

    public static final native long Metadata_NumTranscripts_get(long var0, Metadata var2);

    public static final native long Metadata_getTranscript(long var0, Metadata var2, int var3);

    public static final native void delete_Metadata(long var0);

    public static final native int CreateModel(String var0, long var1);

    public static final native long GetModelBeamWidth(long var0);

    public static final native int SetModelBeamWidth(long var0, long var2);

    public static final native int GetModelSampleRate(long var0);

    public static final native void FreeModel(long var0);

    public static final native int EnableExternalScorer(long var0, String var2);

    public static final native int AddHotWord(long var0, String var2, float var3);

    public static final native int EraseHotWord(long var0, String var2);

    public static final native int ClearHotWords(long var0);

    public static final native int DisableExternalScorer(long var0);

    public static final native int SetScorerAlphaBeta(long var0, float var2, float var3);

    public static final native String SpeechToText(long var0, short[] var2, long var3);

    public static final native long SpeechToTextWithMetadata(long var0, short[] var2, long var3, long var5);

    public static final native int CreateStream(long var0, long var2);

    public static final native void FeedAudioContent(long var0, short[] var2, long var3);

    public static final native String IntermediateDecode(long var0);

    public static final native long IntermediateDecodeWithMetadata(long var0, long var2);

    public static final native String FinishStream(long var0);

    public static final native long FinishStreamWithMetadata(long var0, long var2);

    public static final native void FreeStream(long var0);

    public static final native void FreeMetadata(long var0, Metadata var2);

    public static final native void FreeString(String var0);

    public static final native String Version();

    public static final native String ErrorCodeToErrorMessage(int var0);
}

