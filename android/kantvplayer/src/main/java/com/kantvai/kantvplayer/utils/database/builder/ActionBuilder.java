package com.kantvai.kantvplayer.utils.database.builder;

import android.database.sqlite.SQLiteDatabase;
import android.os.Looper;

import io.reactivex.annotations.CheckReturnValue;


public class ActionBuilder{
    private int tablePosition;
    private SQLiteDatabase sqLiteDatabase;

    public ActionBuilder(int tablePosition, SQLiteDatabase sqLiteDatabase){
        this.sqLiteDatabase = sqLiteDatabase;
        this.tablePosition = tablePosition;
    }

    @CheckReturnValue
    public InsertBuilder insert(){
        return new InsertBuilder(tablePosition, sqLiteDatabase);
    }

    @CheckReturnValue
    public DeleteBuilder delete(){
        return new DeleteBuilder(tablePosition, sqLiteDatabase);
    }

    @CheckReturnValue
    public QueryBuilder query(){
        return new QueryBuilder(tablePosition, sqLiteDatabase);
    }

    @CheckReturnValue
    public UpdateBuilder update(){
        return new UpdateBuilder(tablePosition, sqLiteDatabase);
    }

    public static void checkThreadLocal(){
        if (Looper.getMainLooper() == Looper.myLooper()){
            throw new IllegalThreadStateException("database cannot run in the main thread");
        }
    }

}