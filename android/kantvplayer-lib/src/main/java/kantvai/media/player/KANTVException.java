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

public class KANTVException extends Exception
{

    private int result = 0x00000000;
    private int internalCode = 0;

    public KANTVException(int result)
    {
        super();
        this.result = result;
    }

    public KANTVException(int result, String message)
    {
        super(message);
        this.result = result;
    }

    public KANTVException(String message)
    {
        super(message);

        String resultPart = "Result:";
        int index = message.indexOf(resultPart);
        String errorCodePart = "ErrorCode:";
        int index2 = message.indexOf(errorCodePart);

        if (index >=0)
        {
            try
            {
                if (index2 >= 0)
                {
                    this.result = Integer.parseInt(message.substring(index+resultPart.length(), index2).trim());
                }
                else
                {
                    this.result = Integer.parseInt((message.substring(index+resultPart.length())).trim());
                }

            }
            catch (Exception e)
            {
            }
        }

        if (index2 >=0)
        {
            try
            {
                this.internalCode = Integer.parseInt((message.substring(index2+errorCodePart.length())).trim());

            }
            catch (Exception e)
            {
            }
        }
    }

    public int getResult()
    {
        return result;
    }

    public int getInternalCode()
    {
        return internalCode;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
