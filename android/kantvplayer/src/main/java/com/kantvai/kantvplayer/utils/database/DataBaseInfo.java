package com.kantvai.kantvplayer.utils.database;

import android.database.SQLException;

import java.util.Arrays;
import java.util.List;

import kantvai.media.player.KANTVLog;
import kantvai.media.player.KANTVUtils;

public class DataBaseInfo {
    public static final String DATABASE_NAME = "kantv_db.db";
    static final int DATABASE_VERSION = 30;
    private static final String TAG = DataBaseInfo.class.getName();

    private static String[][] FieldNames;
    private static String[][] FieldTypes;
    private static String[] TableNames;

    static {
        TableNames = new String[]{
                "folder",
                "file",
                "subgroup",
                "scan_folder",
                "local_play_history"
        };

        FieldNames = new String[][] {
                {"_id", "folder_path", "file_number"},
                {"_id", "folder_path", "file_path",  "current_position", "duration", "episode_id", "file_size", "file_id", "zimu_path"},
                {"_id", "subgroup_id", "subgroup_name"},
                {"_id", "folder_path", "folder_type"},
                {"_id", "video_path", "video_title", "episode_id", "source_origin", "play_time", "zimu_path"}
        };

        FieldTypes = new String[][] {
                {"INTEGER PRIMARY KEY AUTOINCREMENT", "VARCHAR(255) NOT NULL","INTEGER NOT NULL"},
                {"INTEGER PRIMARY KEY AUTOINCREMENT", "VARCHAR(255) NOT NULL","VARCHAR(255) NOT NULL","INTEGER", "VARCHAR(255) NOT NULL", "INTEGER","VARCHAR(255)", "INTEGER", "VARCHAR(255)"},
                {"INTEGER PRIMARY KEY AUTOINCREMENT", "INTEGER", "VARCHAR(255)"},
                {"INTEGER PRIMARY KEY AUTOINCREMENT", "VARCHAR(255) NOT NULL", "INTEGER"},
                {"INTEGER PRIMARY KEY AUTOINCREMENT", "VARCHAR(255) NOT NULL", "VARCHAR(255)", "INTEGER", "INTEGER", "INTEGER", "VARCHAR(255)"}
        };
    }

    public static String[][] getFieldNames() {
        return FieldNames;
    }

    public static String[][] getFieldTypes() {
        return FieldTypes;
    }

    public static String[] getTableNames() {
        return TableNames;
    }

    public static int checkTableName(String tableName){
        List<String> tableList = Arrays.asList(TableNames);
        int tablePosition = tableList.indexOf(tableName);
        if (tablePosition >= 0) {
            return tablePosition;
        } else {
            KANTVLog.j(TAG, "\""+tableName + "\" table not found");
            throw new SQLException("\""+tableName + "\" table not found");
        }
    }

    public static void checkColumnName(String colName, int tablePosition){
        String[] colArray = FieldNames[tablePosition];
        List<String> colList = Arrays.asList(colArray);
        int colPosition = colList.indexOf(colName);
        if (colPosition < 0){
            String tableName = DataBaseInfo.getTableNames()[tablePosition];
            KANTVLog.j(TAG, "\""+colName + "\" field no found in the "+tableName + "table");
            throw new SQLException("\""+colName + "\" field no found in the "+tableName + "table");
        }
    }
}