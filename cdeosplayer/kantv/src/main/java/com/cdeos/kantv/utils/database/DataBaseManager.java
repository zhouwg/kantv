package com.cdeos.kantv.utils.database;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import com.cdeos.kantv.utils.database.builder.ActionBuilder;

import java.util.concurrent.atomic.AtomicInteger;

import cdeos.media.player.CDELog;
import cdeos.media.player.CDEUtils;


public class DataBaseManager {

    private AtomicInteger mOpenCounter = new AtomicInteger();
    private static DataBaseManager instance;
    private static SQLiteOpenHelper mDatabaseHelper;
    private SQLiteDatabase mDatabase;
    private static final String TAG = DataBaseManager.class.getName();

    public static synchronized void init(Context context) {
        if (instance == null) {
            instance = new DataBaseManager();
            mDatabaseHelper = new DataBaseHelper(context);
        }
    }

    public static synchronized DataBaseManager getInstance() {
        if (instance == null) {
            throw new IllegalStateException(DataBaseManager.class.getSimpleName() +
                    " is not initialized, call initializeInstance(..) method first.");
        }

        return instance;
    }

    public synchronized void closeDatabase() {
        CDELog.j(TAG, "enter close database");
        if (mOpenCounter.decrementAndGet() == 0) {
            CDELog.j(TAG, "ok close database");
            mDatabase.close();
        } else {
            CDELog.j(TAG, "can not close database");
        }
    }

    public ActionBuilder selectTable(String tableName) {
        int tablePosition = DataBaseInfo.checkTableName(tableName);
        return new ActionBuilder(tablePosition, getSQLiteDatabase());
    }

    private synchronized SQLiteDatabase getSQLiteDatabase() {
        if (mDatabase == null || !mDatabase.isOpen()) {
            return openDatabase();
        }
        return mDatabase;
    }

    private synchronized SQLiteDatabase openDatabase() {
        if (mOpenCounter.incrementAndGet() == 1) {
            mDatabase = mDatabaseHelper.getWritableDatabase();
        }
        return mDatabase;
    }

    static class DataBaseHelper extends SQLiteOpenHelper {
        private static final String TAG = DataBaseHelper.class.getName();
        private static String[][] FieldNames;
        private static String[][] FieldTypes;
        private static String[] TableNames = DataBaseInfo.getTableNames();

        static {
            FieldNames = DataBaseInfo.getFieldNames();
            FieldTypes = DataBaseInfo.getFieldTypes();
        }

        DataBaseHelper(Context context) {
            super(context, CDEUtils.getDataPath(context) + DataBaseInfo.DATABASE_NAME, null, DataBaseInfo.DATABASE_VERSION);
            CDELog.j(TAG, "create sql db:" + DataBaseInfo.DATABASE_NAME);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            CDELog.j(TAG, "on create");
            if (TableNames == null) return;
            String str1;
            String str2;
            for (int i = 0; i < TableNames.length; i++) {
                str1 = "CREATE TABLE " + TableNames[i] + " (";
                for (int j = 0; j < FieldNames[i].length; j++) {
                    str1 = str1 + FieldNames[i][j] + " " + FieldTypes[i][j] + ",";
                }
                str2 = str1.substring(0, str1.length() - 1) + ");";
                CDELog.j(TAG, "sql statement:" + str2);
                db.execSQL(str2);
            }
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            CDELog.j(TAG,"updata");
            for (String tab : TableNames) {
                db.execSQL("DROP TABLE IF EXISTS " + tab + ";");
            }
            onCreate(db);
        }
    }
}
