package com.kantvai.kantvplayer.bean.event;

import java.io.Serializable;


public class OpenFolderEvent implements Serializable {
    public final static String FOLDERPATH = "FOLDERPATH";

    private String folderPath;

    public OpenFolderEvent(String folderPath) {
        this.folderPath = folderPath;
    }

    public String getFolderPath() {
        return folderPath;
    }

    public void setFolderPath(String folderPath) {
        this.folderPath = folderPath;
    }
}
