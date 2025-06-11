package com.kantvai.kantvplayer.utils.interf;

import androidx.annotation.NonNull;

import java.util.List;


public interface IAdapter<T> {

    void setData(@NonNull List<T> data);

    List<T> getData();

    T getItem(int position);

    @NonNull
    AdapterItem<T> onCreateItem(int viewType);

    void addItem(int position, T model);

    void addItem(T model);

    T removeItem(int position);

    boolean removeItem(T model);

    void itemsClear();

    void addAll(List<T> data);

    void addAll(int position, List<T> data);

    void removeSubList(int startPosition, int count);

    void swap(int fromPosition, int toPosition);
}
