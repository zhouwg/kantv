 /*
  * Copyright (C) 2021 zhouwg <zhouwg2000@gmail.com>
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

package tv.danmaku.ijk.media.example.content;

public enum IjkUrlType
{
	HLS("HLS", 0),

	RTMP("RTMP", 1),

	DASH("DASH", 2),

	FILE("FILE",3);

	private int type;
	private String name;


	private IjkUrlType(String name, int type) {
		this.name = name;
		this.type = type;
	}

	IjkUrlType(int type) {
		this.type = type;
	}

	public int getValue() {
		return type;
	}

	@Override
	public String toString() {
		return this.type + "_" + this.name;
	}
}