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

public enum KANTVEventType
{
    //keep sync with KANTV_event.h in native layer
    KANTV_EVENT_ERROR(100, "KANTV ERROR"),
    KANTV_EVENT_INFO(200, "KANTV INFO");

    public final int KANTV_ERROR = 100;
    public final int KANTV_INFO  = 200;
    public final int KANTV_INFO_PREVIEW = 8;

    private int type;
    private String message;

    KANTVEventType(int type, String message)
    {
        this.type = type;
        this.message = message;
    }

    public int getValue()
    {
        return type;
    }

    public static KANTVEventType fromValue(int type)
    {
        for (KANTVEventType eventType: KANTVEventType.values())
        {
            if (eventType.getValue() == type)
                return eventType;
        }
        throw new IllegalArgumentException("Invalid EventType:" + type);
    }

    public String toString()
    {
        return this.message;
    }
}
