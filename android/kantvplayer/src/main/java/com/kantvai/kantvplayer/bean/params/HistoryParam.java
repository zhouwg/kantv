package com.kantvai.kantvplayer.bean.params;

import java.util.List;


public class HistoryParam {
    private boolean addToFavorite;
    private int rating;
    private List<Integer> episodeIdList;

    public boolean isAddToFavorite() {
        return addToFavorite;
    }

    public void setAddToFavorite(boolean addToFavorite) {
        this.addToFavorite = addToFavorite;
    }

    public int getRating() {
        return rating;
    }

    public void setRating(int rating) {
        this.rating = rating;
    }

    public List<Integer> getEpisodeIdList() {
        return episodeIdList;
    }

    public void setEpisodeIdList(List<Integer> episodeIdList) {
        this.episodeIdList = episodeIdList;
    }
}
