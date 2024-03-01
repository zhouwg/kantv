
package org.deepspeech.libdeepspeech;

public class CandidateTranscript {
    private transient long swigCPtr;
    protected transient boolean swigCMemOwn;

    protected CandidateTranscript(long cPtr, boolean cMemoryOwn) {
        this.swigCMemOwn = cMemoryOwn;
        this.swigCPtr = cPtr;
    }

    protected static long getCPtr(CandidateTranscript obj) {
        return obj == null ? 0L : obj.swigCPtr;
    }

    public synchronized void delete() {
        if (this.swigCPtr != 0L) {
            if (this.swigCMemOwn) {
                this.swigCMemOwn = false;
                throw new UnsupportedOperationException("C++ destructor does not have public access");
            }

            this.swigCPtr = 0L;
        }

    }

    public long getNumTokens() {
        return implJNI.CandidateTranscript_NumTokens_get(this.swigCPtr, this);
    }

    public double getConfidence() {
        return implJNI.CandidateTranscript_Confidence_get(this.swigCPtr, this);
    }

    public TokenMetadata getToken(int i) {
        return new TokenMetadata(implJNI.CandidateTranscript_getToken(this.swigCPtr, this, i), false);
    }
}

