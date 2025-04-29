/*
 * Copyright (C) 2015 Bilibili
 * Copyright (C) 2015 Zhang Rui <bbcallen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

package kantvai.media.player;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.AsyncTask;
import androidx.loader.content.AsyncTaskLoader;
import android.text.TextUtils;

import kantvai.media.player.KANTVLog;

public class RecentMediaStorage {
    private Context mAppContext;

    private final static String TAG = RecentMediaStorage.class.getName();
    public RecentMediaStorage(Context context) {
        mAppContext = context.getApplicationContext();
    }

    public void saveUrlAsync(String url) {
        new AsyncTask<String, Void, Void>() {
            @Override
            protected Void doInBackground(String... params) {
                //KANTVLog.d(TAG, "params[0] : " + params[0]);
                saveUrl(params[0]);
                return null;
            }
        }.execute(url);
    }

    public void saveUrl(String syncUrl) {
        ContentValues cv = new ContentValues();
        String url = "unknown";
        String mediaType = "unknown";
        String title = "unknown";
        String playPos = "0";
        try {
            String[] strArr = syncUrl.split("_KANTV_");
            if (4 != strArr.length) {
                KANTVLog.w(TAG, "it shouldn't happen here, pls check");
                return;
            }
            url = strArr[0];
            mediaType = strArr[1];
            title = strArr[2];
            playPos = strArr[3];

            cv.putNull(Entry.COLUMN_NAME_ID);
            //KANTVLog.d(TAG,"url: " + url + " mediatype:" + mediaType + " mediaTitle:" + title + " playPos: " + Long.valueOf(playPos));
            cv.put(Entry.COLUMN_NAME_URL, url);
            cv.put(Entry.COLUMN_NAME_TYPE, mediaType);
            cv.put(Entry.COLUMN_NAME_LAST_ACCESS, System.currentTimeMillis());
            //cv.put(Entry.COLUMN_NAME_NAME, getNameOfUrl(url));
            cv.put(Entry.COLUMN_NAME_NAME, title);
            cv.put(Entry.COLUMN_NAME_PLAY_POS, Long.valueOf(playPos));
            save(cv);
        } catch (IllegalArgumentException ex) {
            KANTVLog.w(TAG, "can't get url and mediatype" + ex.getMessage());
            return;
        } finally {
           return;
        }
    }

    public void save(ContentValues contentValue) {
        OpenHelper openHelper = new OpenHelper(mAppContext);
        SQLiteDatabase db = openHelper.getWritableDatabase();
        db.replace(Entry.TABLE_NAME, null, contentValue);
    }

    public static String getNameOfUrl(String url) {
        return getNameOfUrl(url, "");
    }

    public static String getNameOfUrl(String url, String defaultName) {
        String name = null;
        int pos = url.lastIndexOf('/');
        if (pos >= 0)
            name = url.substring(pos + 1);

        if (TextUtils.isEmpty(name))
            name = defaultName;

        return name;
    }

    public static class Entry {
        public static final String TABLE_NAME = "RecentMedia";
        public static final String COLUMN_NAME_ID = "id";
        public static final String COLUMN_NAME_URL = "url";
        public static final String COLUMN_NAME_TYPE = "type";
        public static final String COLUMN_NAME_NAME = "name";
        public static final String COLUMN_NAME_LAST_ACCESS = "last_access";
        public static final String COLUMN_NAME_PLAY_POS = "play_pos";
    }

    public static final String ALL_COLUMNS[] = new String[]{
            Entry.COLUMN_NAME_ID + " as _id",
            Entry.COLUMN_NAME_ID,
            Entry.COLUMN_NAME_URL,
            Entry.COLUMN_NAME_TYPE,
            Entry.COLUMN_NAME_NAME,
            Entry.COLUMN_NAME_PLAY_POS,
            Entry.COLUMN_NAME_LAST_ACCESS};

    public static class OpenHelper extends SQLiteOpenHelper {
        private static final int DATABASE_VERSION = 1;
        private static final String DATABASE_NAME = "RecentMedia_kantv.db";
        private static final String SQL_CREATE_ENTRIES =
                " CREATE TABLE IF NOT EXISTS " + Entry.TABLE_NAME + " (" +
                        Entry.COLUMN_NAME_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
                        Entry.COLUMN_NAME_URL + " VARCHAR UNIQUE, " +
                        Entry.COLUMN_NAME_TYPE + " VARCHAR , " +
                        Entry.COLUMN_NAME_NAME + " VARCHAR UNIQUE, " +
                        Entry.COLUMN_NAME_PLAY_POS + " INTEGER , " +
                        Entry.COLUMN_NAME_LAST_ACCESS + " INTEGER) ";

        public OpenHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(SQL_CREATE_ENTRIES);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        }
    }

    public static class CursorLoader extends AsyncTaskLoader<Cursor> {
        public CursorLoader(Context context) {
            super(context);
        }

        @Override
        public Cursor loadInBackground() {
            Context context = getContext();
            OpenHelper openHelper = new OpenHelper(context);
            SQLiteDatabase db = openHelper.getReadableDatabase();

            return db.query(Entry.TABLE_NAME, ALL_COLUMNS, null, null, null, null,
                    Entry.COLUMN_NAME_LAST_ACCESS + " DESC",
                    "100");
        }

        @Override
        protected void onStartLoading() {
            forceLoad();
        }
    }
}
