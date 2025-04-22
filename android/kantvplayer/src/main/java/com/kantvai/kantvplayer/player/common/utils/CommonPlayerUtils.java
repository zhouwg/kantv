package com.kantvai.kantvplayer.player.common.utils;

import android.content.Context;

import androidx.annotation.ColorInt;
import androidx.annotation.ColorRes;
import androidx.core.content.ContextCompat;
import android.text.TextUtils;

import com.blankj.utilcode.util.FileUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


public final class CommonPlayerUtils {

    public static final String[] subtitleExtension = new String[]{
            "ASS", "SCC", "SRT", "STL", "TTML"
    };


    public static String getSubtitlePath(String videoPath, String subtitleDownloadFolder) {
        if (TextUtils.isEmpty(videoPath) || !videoPath.contains("."))
            return "";

        File videoFile = new File(videoPath);
        if (!videoFile.exists())
            return "";


        List<String> extensionList = Arrays.asList(subtitleExtension);


        String videoPathNoExt = videoFile.getAbsolutePath();
        int pointIndex = videoPathNoExt.lastIndexOf(".");
        videoPathNoExt = videoPathNoExt.substring(0, pointIndex);

        List<String> subtitlePathList = new ArrayList<>();
        File folderFile = videoFile.getParentFile();
        for (File childFile : folderFile.listFiles()) {
            String childFilePath = childFile.getAbsolutePath();

            if (childFilePath.startsWith(videoPathNoExt)) {
                String extension = FileUtils.getFileExtension(childFilePath);

                if (extensionList.contains(extension.toUpperCase())) {

                    if (childFilePath.length() == videoPathNoExt.length() + extension.length() + 1)
                        return childFilePath;
                    subtitlePathList.add(childFilePath);
                }
            }
        }

        if (subtitlePathList.size() < 1 && !TextUtils.isEmpty(subtitleDownloadFolder)){
            File folder = new File(subtitleDownloadFolder);
            if (folder.exists() && folder.isDirectory()){
                for (File childFile : folder.listFiles()) {
                    String childFileName = FileUtils.getFileName(childFile);
                    String videoNameNoExt = FileUtils.getFileNameNoExtension(videoFile);

                    if (childFileName.startsWith(videoNameNoExt)) {
                        String extension = FileUtils.getFileExtension(childFileName);

                        if (extensionList.contains(extension.toUpperCase())) {

                            if (childFileName.length() == videoNameNoExt.length() + extension.length() + 1)
                                return childFile.getAbsolutePath();
                            subtitlePathList.add(childFile.getAbsolutePath());
                        }
                    }
                }
            }
        }

        if (subtitlePathList.size() < 1) {
            return "";
        } else if (subtitlePathList.size() == 1) {
            return subtitlePathList.get(0);
        } else {
            for (String subtitlePath : subtitlePathList) {
                String extension = FileUtils.getFileExtension(subtitlePath);
                int endIndex = subtitlePath.length() - extension.length() - 1;
                if (endIndex < videoPathNoExt.length()){
                    continue;
                }
                String centerContent = subtitlePath.substring(videoPathNoExt.length(), endIndex);

                if (centerContent.contains("."))
                    return subtitlePath;
            }
        }
        return "";
    }


    @ColorInt
    public static int getResColor(Context context, @ColorRes int colorId){
        return ContextCompat.getColor(context, colorId);
    }
}
