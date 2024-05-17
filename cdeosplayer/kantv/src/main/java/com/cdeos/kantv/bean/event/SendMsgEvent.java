package com.cdeos.kantv.bean.event;

public class SendMsgEvent {

    private String msg;

    public SendMsgEvent(String msg) {
        this.msg = msg;
    }

    public String getMsg() {
        return msg;
    }
}
