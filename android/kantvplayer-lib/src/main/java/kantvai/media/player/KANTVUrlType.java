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

public enum KANTVUrlType
{
	HLS("HLS", 0),

	RTMP("RTMP", 1),

	DASH("DASH", 2),

	MP4("MP4", 3),

	TS( "TS", 4),

	MKV("MKV", 5),

	MP3("MP3", 6),

	OGG("OGG", 7),
	AAC("AAC", 8),
	AC3("AC3", 9),

	FILE("FILE",20);

	private int type;
	private String name;


	private KANTVUrlType(String name, int type) {
		this.name = name;
		this.type = type;
	}

	KANTVUrlType(int type) {
		this.type = type;
	}

	public int getValue() {
		return type;
	}

	@Override
	public String toString() {
		return this.type + "_" + this.name;
	}

	public static native int kantv_anti_remove_rename_this_file();
}