package com.kantvai.kantvplayer.bean;

import com.chad.library.adapter.base.entity.MultiItemEntity;



public class QuestionEntity implements MultiItemEntity {

    public static final int ITEM_HEADER = 101;
    public static final int ITEM_CONTENT = 102;

    private int type;
    private Object object;

    private boolean isOpen;

    public QuestionEntity(int type, Object object) {
        this.type = type;
        this.object = object;
    }

    public Object getObject() {
        return object;
    }

    public boolean isOpen() {
        return isOpen;
    }

    public void setOpen(boolean open) {
        isOpen = open;
    }

    @Override
    public int getItemType() {
        return type;
    }
}
