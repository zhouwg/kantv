
package org.deepspeech.libdeepspeech;

public class TokenMetadata {
    private transient long swigCPtr;
    protected transient boolean swigCMemOwn;

    protected TokenMetadata(long cPtr, boolean cMemoryOwn) {
        this.swigCMemOwn = cMemoryOwn;
        this.swigCPtr = cPtr;
    }

    protected static long getCPtr(TokenMetadata obj) {
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

    public String getText() {
        return implJNI.TokenMetadata_Text_get(this.swigCPtr, this);
    }

    public long getTimestep() {
        return implJNI.TokenMetadata_Timestep_get(this.swigCPtr, this);
    }

    public float getStartTime() {
        return implJNI.TokenMetadata_StartTime_get(this.swigCPtr, this);
    }
}

