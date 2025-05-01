 /*
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

public class KANTVLLMModel {
    private static final String TAG = KANTVLLMModel.class.getSimpleName();
    private int index;
    private String nickname;    //model's short name
    private String name;        //model's full name
    private String mmproj_name; //mmproj model name
    private String url;         //original url of model

    private String mmproj_url;  //original url of mmproj model

    private String quality;     //quality of model on Android phone

    public KANTVLLMModel(int index, String nick, String name, String url) {
        this.index = index;
        this.nickname  = nick;
        this.name  = name;
        this.url   = url;
    }

    public KANTVLLMModel(int index, String nick, String name, String url, String quality) {
        this(index, nick, name, url);
        this.quality = quality;
    }

    public KANTVLLMModel(int index, String nick, String name, String mmprojName, String url, String quality) {
        this(index, nick, name, url, quality);
        this.mmproj_name = mmprojName;
        KANTVLog.j(TAG, "init");
    }

    public String getNickname() { return nickname; }
    public String getName() {
        return name;
    }

    public String getMMProjName() { return mmproj_name; }
    public String getMMProjUrl() { return mmproj_url; }
    public void setMmprojUrl(String mmprojUrl) { this.mmproj_url = mmprojUrl; }

    public String getUrl() {
        return url;
    }
    public void setUrl(String modelUrl) { this.url = modelUrl;}

    public int getIndex() {
        return index;
    }

    public String getQuality() { return quality; }

    public void setQuality(String quality) {
        this.quality = quality;
    }


}
