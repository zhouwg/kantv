package com.kantvai.kantvplayer.utils.net.utils;

public class ResponseError extends Exception {
    public int code;
    public String message;

    ResponseError(Throwable throwable, int code) {
        super(throwable);
        this.code = code;
    }
}
