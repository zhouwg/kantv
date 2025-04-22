package com.kantvai.kantvplayer.utils;

import android.annotation.SuppressLint;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.text.TextUtils;

import androidx.annotation.ColorInt;
import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;

import com.blankj.utilcode.util.UriUtils;
import com.kantvai.kantvplayer.app.IApplication;

import java.io.Closeable;
import java.io.File;;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Pattern;

import skin.support.content.res.SkinCompatResources;

//TODO:merge code to KANTVUtils.java
public class CommonUtils {
    private static final String EXTERNAL_STORAGE = "com.android.externalstorage.documents";
    private static final String DOWNLOAD_DOCUMENT = "com.android.providers.downloads.documents";
    private static final String MEDIA_DOCUMENT = "com.android.providers.media.documents";
    private static final String DOWNLOAD_URI = "content://downloads/public_downloads";

    private static String appVersion = "";

    public static String getAppVersion() {
        if (!TextUtils.isEmpty(appVersion)) {
            return appVersion;
        }

        String localVersionName = "";
        try {
            PackageInfo packageInfo = IApplication.get_context()
                    .getPackageManager()
                    .getPackageInfo(IApplication.get_context().getPackageName(), 0);
            localVersionName = packageInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return localVersionName;
    }


    public static String formatDuring(long mss) {
        long hours = mss / (1000 * 60 * 60);
        long minutes = (mss % (1000 * 60 * 60)) / (1000 * 60);
        long seconds = (mss % (1000 * 60)) / 1000;
        StringBuilder stringBuilder = new StringBuilder();
        if (hours != 0) {
            if (hours < 10)
                stringBuilder.append("0").append(hours).append(":");
            else
                stringBuilder.append(hours).append(":");
        }
        if (minutes == 0) {
            stringBuilder.append("00:");
        } else {
            if (minutes < 10)
                stringBuilder.append("0").append(minutes).append(":");
            else
                stringBuilder.append(minutes).append(":");
        }
        if (seconds == 0) {
            stringBuilder.append("00");
        } else {
            if (seconds < 10)
                stringBuilder.append("0").append(seconds);
            else
                stringBuilder.append(seconds);
        }
        return stringBuilder.toString();
    }

    @SuppressLint("DefaultLocale")
    public static String convertFileSize(long size) {
        long kb = 1024;
        long mb = kb * 1024;
        long gb = mb * 1024;

        if (size >= gb) {
            return String.format("%.1f GB", (float) size / gb);
        } else if (size >= mb) {
            float f = (float) size / mb;
            return String.format(f > 100 ? "%.0f M" : "%.1f M", f);
        } else if (size >= kb) {
            float f = (float) size / kb;
            return String.format(f > 100 ? "%.0f K" : "%.1f K", f);
        } else
            return String.format("%d B", size);
    }


    public static String getFolderName(String folderPath) {
        if (TextUtils.isEmpty(folderPath))
            return "";

        while (folderPath.endsWith("/"))
            folderPath = folderPath.substring(0, folderPath.length() - 1);

        int index = folderPath.lastIndexOf("/");
        if (index > 0 && index + 1 < folderPath.length()) {
            return folderPath.substring(index + 1);
        } else {
            return folderPath;
        }
    }


    public static boolean isNum(String str) {
        if (TextUtils.isEmpty(str))
            return false;
        Pattern pattern = Pattern.compile("^-?[0-9]+");
        return pattern.matcher(str).matches();
    }


    public static boolean isUrlLink(String str) {
        String regex = "^([hH][tT]{2}[pP]://|[hH][tT]{2}[pP][sS]://)(([A-Za-z0-9-~]+).)+([A-Za-z0-9-~/])+$";
        Pattern pattern = Pattern.compile(regex);
        return pattern.matcher(str).matches();
    }


    public static boolean isMediaFile(String fileName) {
        switch (com.blankj.utilcode.util.FileUtils.getFileExtension(fileName).toLowerCase()) {
            case "3gp":
            case "avi":
            case "flv":
            case "mp4":
            case "m4v":
            case "mkv":
            case "mov":
            case "mpeg":
            case "mpg":
            case "mpe":
            case "rm":
            case "rmvb":
            case "wmv":
            case "asf":
            case "asx":
            case "dat":
            case "vob":
            case "m3u8":
                return true;
            default:
                return false;
        }
    }


    public static String getRealFilePath(Context context, @NonNull Uri uri) {
        if ("http".equalsIgnoreCase(uri.getScheme())) {
            return uri.toString();
        } else if ("https".equalsIgnoreCase(uri.getScheme())) {
            return uri.toString();
        } else {
            File file = UriUtils.uri2File(uri);
            if (file != null)
                return file.getAbsolutePath();
            else {
                try {
                    return getUriPath(context, uri);
                } catch (Exception ignore){

                }
            }
        }
        return null;
    }

    private static String getUriPath(Context context, @NonNull Uri uri){
        boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        if (isKitKat && DocumentsContract.isDocumentUri(context, uri)) {
            String authority = uri.getAuthority();
            if (authority == null)
                return "";
            switch (authority) {
                case EXTERNAL_STORAGE:
                    String docId = DocumentsContract.getDocumentId(uri);
                    String[] exSplit = docId.split(":");
                    String type = exSplit[0];
                    if ("primary".equalsIgnoreCase(type)) {
                        return Environment.getExternalStorageDirectory() + "/" + exSplit[1];
                    }
                    break;
                case DOWNLOAD_DOCUMENT:
                    String id = DocumentsContract.getDocumentId(uri);
                    Uri documentUri = ContentUris.withAppendedId(Uri.parse(DOWNLOAD_URI), Long.parseLong(id));
                    return getDataColumn(context, documentUri, null, null);
                case MEDIA_DOCUMENT:
                    String[] split = DocumentsContract.getDocumentId(uri).split(":");
                    Uri contentUri = null;
                    switch (split[0]) {
                        case "image":
                            contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                            break;
                        case "video":
                            contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                            break;
                        case "audio":
                            contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                            break;
                    }
                    return getDataColumn(context, contentUri, "_id=?", new String[]{split[1]});
            }
        } else if ("content".equalsIgnoreCase(uri.getScheme())) {
            return getDataColumn(context, uri, null, null);
        } else if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        }
        return null;
    }

    private static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs) {
        context.grantUriPermission(context.getPackageName(), uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try (Cursor cursor = context.getContentResolver().query(uri, new String[]{"_data"}, selection, selectionArgs, null)) {
            if (cursor != null && cursor.moveToNext()) {
                return cursor.getString(0);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }


    public static Drawable getResDrawable(@DrawableRes int drawableId) {
        return SkinCompatResources.getDrawable(IApplication.get_context(), drawableId);
        //return ContextCompat.getColor(IApplication.get_context(), colorId);
    }


    @ColorInt
    public static int getResColor(@ColorRes int colorId) {
        return SkinCompatResources.getColor(IApplication.get_context(), colorId);
        //return ContextCompat.getColor(IApplication.get_context(), colorId);
    }

    @ColorInt
    public static int getResColor(@IntRange(from = 0, to = 255) int alpha, @ColorRes int colorId) {
        int color = getResColor(colorId);

        int red = Color.red(color);
        int green = Color.green(color);
        int blue = Color.blue(color);

        return Color.argb(alpha, red, green, blue);
    }


    public static String decodeHttpUrl(String url) {
        if (TextUtils.isEmpty(url) || !url.toLowerCase().startsWith("http"))
            return url;

        try {
            if (url.contains("%252F")) {
                String decodePath = URLDecoder.decode(url, "utf-8");
                url = URLDecoder.decode(decodePath, "utf-8");
            }
            else if (url.contains("%2F")) {
                url = URLDecoder.decode(url, "utf-8");

            }
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return url;
    }


    @SuppressLint("SimpleDateFormat")
    public static String getCurrentFileName(String header, String tail) {
        SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMdd_HHmmss");
        Date curDate = new Date(System.currentTimeMillis());
        String curTime = formatter.format(curDate);
        return "/" + header + "_" + curTime + tail;
    }


    public static void closeResource(Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}
