package tv.danmaku.ijk.media.example.application;


public enum MediaType {
    MEDIA_TV("Online TV", 0),
    MEDIA_MOVIE("Online MOVIE", 1),
    MEDIA_FILE("Local File", 2);

    private String name;
    private int index;

    private MediaType(String name, int index) {
        this.name  = name;
        this.index = index;
    }

    @Override
    public String toString() {
        return this.index + "_" + this.name;
    }
}
