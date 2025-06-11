package com.kantvai.kantvplayer.player.common.utils;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.List;
import java.util.Map;


public final class SharedPreferencesUtil {

    private static String FILE_NAME = "file_name";
    private Context context;
    private static SharedPreferencesUtil instance;

    private SharedPreferencesUtil(Context context) {
        this.context = context.getApplicationContext();
    }

    public synchronized static SharedPreferencesUtil getInstance(Context context, String filename) {
        if (instance == null) {
            instance = new SharedPreferencesUtil(context);
        }
        if (filename != null) {
            FILE_NAME = filename;
        }
        return instance;
    }


    public void save(String key, String value) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.putString(key, value);
        editor.apply();
    }

    public void save(String key, boolean value){
        SharedPreferences.Editor editor = getShare().edit();
        editor.putBoolean(key, value);
        editor.apply();
    }


    public String load(String key, String defaultValue) {
        return getShare().getString(key, defaultValue);
    }
    public Boolean load(String key, boolean defaultValue) {
        return getShare().getBoolean(key, defaultValue);
    }

    public synchronized SharedPreferences getShare() {
        return this.context.getSharedPreferences(FILE_NAME, Context.MODE_PRIVATE);
    }

    public void saveSharedPreferences(String key, int value) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.putInt(key, value);
        editor.apply();
    }

    public int loadIntSharedPreference(String key) {
        return getShare().getInt(key, 0);
    }

    public int loadIntSharedPreference(String key, int defaultValue) {
        return getShare().getInt(key, defaultValue);
    }

    public void saveSharedPreferences(String key, float value) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.putFloat(key, value);
        editor.apply();
    }

    public float loadFloatSharedPreference(String key) {
        return getShare().getFloat(key, 0f);
    }

    public void saveSharedPreferences(String key, Long value) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.putLong(key, value);
        editor.apply();
    }

    public long loadLongSharedPreference(String key) {
        return getShare().getLong(key, 0L);
    }

    public void saveSharedPreferences(String key, Boolean value) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.putBoolean(key, value);
        editor.apply();
    }

    public boolean loadBooleanSharedPreference(String key) {
        return getShare().getBoolean(key, false);
    }

    public void saveAllSharePreference(String keyName, List<?> list) {
        int size = list.size();
        if (size < 1) {
            return;
        }
        SharedPreferences.Editor editor = getShare().edit();
        if (list.get(0) instanceof String) {
            for (int i = 0; i < size; i++) {
                editor.putString(keyName + i, (String) list.get(i));
            }
        } else if (list.get(0) instanceof Long) {
            for (int i = 0; i < size; i++) {
                editor.putLong(keyName + i, (Long) list.get(i));
            }
        } else if (list.get(0) instanceof Float) {
            for (int i = 0; i < size; i++) {
                editor.putFloat(keyName + i, (Float) list.get(i));
            }
        } else if (list.get(0) instanceof Integer) {
            for (int i = 0; i < size; i++) {
                editor.putLong(keyName + i, (Integer) list.get(i));
            }
        } else if (list.get(0) instanceof Boolean) {
            for (int i = 0; i < size; i++) {
                editor.putBoolean(keyName + i, (Boolean) list.get(i));
            }
        }
        editor.apply();
    }


    public void setParam(Context context, String key, Object object) {

        String type = object.getClass().getSimpleName();
        SharedPreferences sp = context.getSharedPreferences(FILE_NAME,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sp.edit();

        if ("String".equals(type)) {
            editor.putString(key, (String) object);
        } else if ("Integer".equals(type)) {
            editor.putInt(key, (Integer) object);
        } else if ("Boolean".equals(type)) {
            editor.putBoolean(key, (Boolean) object);
        } else if ("Float".equals(type)) {
            editor.putFloat(key, (Float) object);
        } else if ("Long".equals(type)) {
            editor.putLong(key, (Long) object);
        }

        key = null;
        object = null;
        editor.apply();
        editor = null;
        sp = null;
        context = null;
    }


    public static Object getParam(Context context, String key,
                                  Object defaultObject) {
        String type = defaultObject.getClass().getSimpleName();
        SharedPreferences sp = context.getSharedPreferences(FILE_NAME,
                Context.MODE_PRIVATE);

        if ("String".equals(type)) {
            return sp.getString(key, (String) defaultObject);
        } else if ("Integer".equals(type)) {
            return sp.getInt(key, (Integer) defaultObject);
        } else if ("Boolean".equals(type)) {
            return sp.getBoolean(key, (Boolean) defaultObject);
        } else if ("Float".equals(type)) {
            return sp.getFloat(key, (Float) defaultObject);
        } else if ("Long".equals(type)) {
            return sp.getLong(key, (Long) defaultObject);
        }

        return null;
    }

    public Map<String, ?> loadAllSharePreference(String key) {
        return getShare().getAll();
    }

    public void removeKey(String key) {
        SharedPreferences.Editor editor = getShare().edit();
        editor.remove(key);
        editor.apply();
    }

    public void removeAllKey() {
        SharedPreferences.Editor editor = getShare().edit();
        editor.clear();
        editor.apply();
    }

}
