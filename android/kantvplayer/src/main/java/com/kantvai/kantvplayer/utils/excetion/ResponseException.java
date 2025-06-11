package com.kantvai.kantvplayer.utils.excetion;

public class ResponseException extends Exception {

    public static final int ERROR_CODE_0 = 0;
    public static final int ERROR_CODE_1 = 1;
    public static final int ERROR_CODE_2 = 2;

    private int errorCode = -1;

    public ResponseException() {

    }

    public ResponseException(int errorCode, String message) {
        super(message);
        this.errorCode = errorCode;
    }

    public ResponseException(String message, Throwable cause) {
        super(message, cause);
        this.initCause(cause);
    }

    public int getErrorCode() {
        return errorCode;
    }

    public void setErrorCode(int errorCode) {
        this.errorCode = errorCode;
    }
}
