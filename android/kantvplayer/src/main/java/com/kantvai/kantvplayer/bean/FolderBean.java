package com.kantvai.kantvplayer.bean;

import java.io.Serializable;


public class FolderBean implements Serializable {
    private String folderPath;
    private int fileNumber;

    public FolderBean(String folderPath, int fileNumber) {
        this.folderPath = folderPath;
        this.fileNumber = fileNumber;
    }

    public int getFileNumber() {
        return fileNumber;
    }

    public void setFileNumber(int fileNumber) {
        this.fileNumber = fileNumber;
    }

    public String getFolderPath() {
        return folderPath;
    }

    public void setFolderPath(String folderPath) {
        this.folderPath = folderPath;
    }
}
