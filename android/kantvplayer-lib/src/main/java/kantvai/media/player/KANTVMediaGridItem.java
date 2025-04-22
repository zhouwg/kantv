 /*
  * Copyright (c) Project KanTV. 2021-2023. All rights reserved.
  *
  * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *      http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
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