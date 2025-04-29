 /*
  * Copyright (c) Project KanTV. 2021-2023
  *
  * Copyright (c) 2024- KanTV Authors
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


public enum KANTVMediaType {
    MEDIA_TV("Online_TV", 0),
    MEDIA_RADIO("Online_RADIO", 1),
    MEDIA_MOVIE("Online_MOVIE", 2),
    MEDIA_FILE("Local_File", 3);

    private String name;
    private int index;

    private KANTVMediaType(String name, int index) {
        this.name  = name;
        this.index = index;
    }

    @Override
    public String toString() {
        return this.index + "_" + this.name;
    }

    public static KANTVMediaType toMediaType(String mediaType) {
        if (mediaType.equals("0_Online_TV"))
            return MEDIA_TV;
        else if (mediaType.equals("1_Online_RADIO"))
            return MEDIA_RADIO;
        else if (mediaType.equals("2_Online_MOVIE"))
            return MEDIA_MOVIE;
        else if (mediaType.equals("3_Local_File"))
            return MEDIA_FILE;

        return MEDIA_TV;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
