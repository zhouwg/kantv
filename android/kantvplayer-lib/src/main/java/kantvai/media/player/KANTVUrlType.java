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