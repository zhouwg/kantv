
package org.deepspeech.libdeepspeech;

import java.io.File;

import cdeos.media.player.CDELog;

public class DeepSpeechModel {
    private final static String TAG = DeepSpeechModel.class.getName();
    private SWIGTYPE_p_p_ModelState _mspp = impl.new_modelstatep();
    private SWIGTYPE_p_ModelState _msp;

    public boolean isInitOK() {
        return _msp != null;
    }

    private void evaluateErrorCode(int errorCode) {
        DeepSpeech_Error_Codes code = DeepSpeech_Error_Codes.swigToEnum(errorCode);
        if (code != DeepSpeech_Error_Codes.ERR_OK) {
            throw new RuntimeException("Error: " + impl.ErrorCodeToErrorMessage(errorCode) + " (0x" + Integer.toHexString(errorCode) + ").");
        }
    }

    public DeepSpeechModel(String modelPath) {
        CDELog.j(TAG, "modelPath:" + modelPath);
        //try {
        File file = new File(modelPath);
        if (file.exists()) {
            CDELog.j(TAG, "model file exist:" + modelPath);
        } else {
            CDELog.j(TAG, "mode file not exist:" + modelPath);
        }

        int result = impl.CreateModel(modelPath, this._mspp);
        CDELog.j(TAG, "result=" + Integer.toHexString(result));
        DeepSpeech_Error_Codes code = DeepSpeech_Error_Codes.swigToEnum(result);
        if (code != DeepSpeech_Error_Codes.ERR_OK) {
            CDELog.j(TAG, "Error: " + impl.ErrorCodeToErrorMessage(result) + " (0x" + Integer.toHexString(result) + ").");
        }
        if (result != 0x3002) {
            this.evaluateErrorCode(result);
            this._msp = impl.modelstatep_value(this._mspp);
        } else {
            this._msp = null;
        }
        //} catch (Exception ex) {
        //   CDELog.j(TAG, "create model failed, reason:" + ex.toString());
        //}
    }

    public long beamWidth() {
        return impl.GetModelBeamWidth(this._msp);
    }

    public void setBeamWidth(long beamWidth) {
        this.evaluateErrorCode(impl.SetModelBeamWidth(this._msp, beamWidth));
    }

    public int sampleRate() {
        return impl.GetModelSampleRate(this._msp);
    }

    public void freeModel() {
        impl.FreeModel(this._msp);
    }

    public void enableExternalScorer(String scorer) {
        this.evaluateErrorCode(impl.EnableExternalScorer(this._msp, scorer));
    }

    public void disableExternalScorer() {
        this.evaluateErrorCode(impl.DisableExternalScorer(this._msp));
    }

    public void setScorerAlphaBeta(float alpha, float beta) {
        this.evaluateErrorCode(impl.SetScorerAlphaBeta(this._msp, alpha, beta));
    }

    public String stt(short[] buffer, int buffer_size) {
        return impl.SpeechToText(this._msp, buffer, (long) buffer_size);
    }

    public Metadata sttWithMetadata(short[] buffer, int buffer_size, int num_results) {
        return impl.SpeechToTextWithMetadata(this._msp, buffer, (long) buffer_size, (long) num_results);
    }

    public DeepSpeechStreamingState createStream() {
        SWIGTYPE_p_p_StreamingState ssp = impl.new_streamingstatep();
        this.evaluateErrorCode(impl.CreateStream(this._msp, ssp));
        return new DeepSpeechStreamingState(impl.streamingstatep_value(ssp));
    }

    public void feedAudioContent(DeepSpeechStreamingState ctx, short[] buffer, int buffer_size) {
        impl.FeedAudioContent(ctx.get(), buffer, (long) buffer_size);
    }

    public String intermediateDecode(DeepSpeechStreamingState ctx) {
        return impl.IntermediateDecode(ctx.get());
    }

    public Metadata intermediateDecodeWithMetadata(DeepSpeechStreamingState ctx, int num_results) {
        return impl.IntermediateDecodeWithMetadata(ctx.get(), (long) num_results);
    }

    public String finishStream(DeepSpeechStreamingState ctx) {
        return impl.FinishStream(ctx.get());
    }

    public Metadata finishStreamWithMetadata(DeepSpeechStreamingState ctx, int num_results) {
        return impl.FinishStreamWithMetadata(ctx.get(), (long) num_results);
    }

    public void addHotWord(String word, float boost) {
        this.evaluateErrorCode(impl.AddHotWord(this._msp, word, boost));
    }

    public void eraseHotWord(String word) {
        this.evaluateErrorCode(impl.EraseHotWord(this._msp, word));
    }

    public void clearHotWords() {
        this.evaluateErrorCode(impl.ClearHotWords(this._msp));
    }

    public String getVersion() {
        return impl.Version();
    }

    static {
        System.loadLibrary("deepspeech-jni");
        System.loadLibrary("deepspeech");
    }
}

