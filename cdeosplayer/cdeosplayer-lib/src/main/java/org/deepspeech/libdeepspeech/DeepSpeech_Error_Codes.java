
package org.deepspeech.libdeepspeech;

public enum DeepSpeech_Error_Codes {
    ERR_OK(0),
    ERR_NO_MODEL(4096),
    ERR_INVALID_ALPHABET(8192),
    ERR_INVALID_SHAPE(8193),
    ERR_INVALID_SCORER(8194),
    ERR_MODEL_INCOMPATIBLE(8195),
    ERR_SCORER_NOT_ENABLED(8196),
    ERR_SCORER_UNREADABLE(8197),
    ERR_SCORER_INVALID_LM(8198),
    ERR_SCORER_NO_TRIE(8199),
    ERR_SCORER_INVALID_TRIE(8200),
    ERR_SCORER_VERSION_MISMATCH(8201),
    ERR_FAIL_INIT_MMAP(12288),
    ERR_FAIL_INIT_SESS(12289),
    ERR_FAIL_INTERPRETER(12290),
    ERR_FAIL_RUN_SESS(12291),
    ERR_FAIL_CREATE_STREAM(12292),
    ERR_FAIL_READ_PROTOBUF(12293),
    ERR_FAIL_CREATE_SESS(12294),
    ERR_FAIL_CREATE_MODEL(12295),
    ERR_FAIL_INSERT_HOTWORD(12296),
    ERR_FAIL_CLEAR_HOTWORD(12297),
    ERR_FAIL_ERASE_HOTWORD(12304);

    private final int swigValue;

    public final int swigValue() {
        return this.swigValue;
    }

    public static DeepSpeech_Error_Codes swigToEnum(int swigValue) {
        DeepSpeech_Error_Codes[] swigValues = (DeepSpeech_Error_Codes[])DeepSpeech_Error_Codes.class.getEnumConstants();
        if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue) {
            return swigValues[swigValue];
        } else {
            DeepSpeech_Error_Codes[] var2 = swigValues;
            int var3 = swigValues.length;

            for(int var4 = 0; var4 < var3; ++var4) {
                DeepSpeech_Error_Codes swigEnum = var2[var4];
                if (swigEnum.swigValue == swigValue) {
                    return swigEnum;
                }
            }

            throw new IllegalArgumentException("No enum " + DeepSpeech_Error_Codes.class + " with value " + swigValue);
        }
    }

    private DeepSpeech_Error_Codes() {
        this.swigValue = DeepSpeech_Error_Codes.SwigNext.next++;
    }

    private DeepSpeech_Error_Codes(int swigValue) {
        this.swigValue = swigValue;
        DeepSpeech_Error_Codes.SwigNext.next = swigValue + 1;
    }

    private DeepSpeech_Error_Codes(DeepSpeech_Error_Codes swigEnum) {
        this.swigValue = swigEnum.swigValue;
        DeepSpeech_Error_Codes.SwigNext.next = this.swigValue + 1;
    }

    private static class SwigNext {
        private static int next = 0;

        private SwigNext() {
        }
    }
}

