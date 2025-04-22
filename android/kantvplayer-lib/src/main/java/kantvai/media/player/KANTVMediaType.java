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
