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
