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


public class KANTVMediaGridItem {
    private int ItemId;
    private String ItemName;
    private String ItemUri;

    public KANTVMediaGridItem() {
    }

    public KANTVMediaGridItem(int itemId, String itemName) {
        this.ItemId   = itemId;
        this.ItemName = itemName;
    }

    public KANTVMediaGridItem(int itemId, String itemName, String itemUri) {
        this.ItemId   = itemId;
        this.ItemName = itemName;
        this.ItemUri  = itemUri;
    }

    public int getItemId() {
        return ItemId;
    }

    public String getItemName() {
        return ItemName;
    }

    public String getItemUri() {
        return ItemUri;
    }

    public void setItemId(int itemId) {
        this.ItemId = itemId;
    }

    public void setItemName(String itemName) {
        this.ItemName = itemName;
    }

    public void setItemUri(String itemUri) {
        this.ItemUri = itemUri;
    }
}