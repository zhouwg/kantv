
package org.deepspeech.libdeepspeech;

public class Metadata {
    private transient long swigCPtr;
    protected transient boolean swigCMemOwn;

    protected Metadata(long cPtr, boolean cMemoryOwn) {
        this.swigCMemOwn = cMemoryOwn;
        this.swigCPtr = cPtr;
    }

    protected static long getCPtr(Metadata obj) {
        return obj == null ? 0L : obj.swigCPtr;
    }

    protected void finalize() {
        this.delete();
    }

    public synchronized void delete() {
        if (this.swigCPtr != 0L) {
            if (this.swigCMemOwn) {
                this.swigCMemOwn = false;
                implJNI.delete_Metadata(this.swigCPtr);
            }

            this.swigCPtr = 0L;
        }

    }

    public long getNumTranscripts() {
        return implJNI.Metadata_NumTranscripts_get(this.swigCPtr, this);
    }

    public CandidateTranscript getTranscript(int i) {
        return new CandidateTranscript(implJNI.Metadata_getTranscript(this.swigCPtr, this, i), false);
    }
}

