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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


public class KANTVJNIDecryptBuffer {
    private static volatile boolean mIsInit = false;
    private static KANTVJNIDecryptBuffer instance = null;
    private int result;
    private int dataLength;
    private byte[] data;
    private static ByteBuffer mDirectBuffer;
    public static final int MAX_DECRYPT_BUFFER = 1024 * 2048;

    private KANTVJNIDecryptBuffer() {

    }
    public static KANTVJNIDecryptBuffer getInstance() {
        if (!mIsInit) {
            mDirectBuffer = ByteBuffer.allocateDirect(MAX_DECRYPT_BUFFER);
            mDirectBuffer.order(ByteOrder.LITTLE_ENDIAN);
            instance = new KANTVJNIDecryptBuffer();
            mIsInit = true;
        }
        return instance;

    }

    //public void setResult(int result){
       // this.result = result;
    //}

    public int getResult(){
        return this.result;
    }

    //public void setDataLength(int dataLength){
     //   this.dataLength = dataLength;
    //}

    public int getDataLength(){
        return this.dataLength;
    }

    //public void setData(byte[] data){
     //   this.data = data;
   // }

    public static ByteBuffer getDirectBuffer() { return  mDirectBuffer; }
    public static byte[] getDirectBufferData(){
        return mDirectBuffer.array();
    }

    public  byte[] getData(){
        return this.data;
    }

    public void setAll(int result, int dataLength){
        this.result = result;
        this.dataLength = dataLength;
        //this.data = data;
    }

    public void setAll3(int result, int dataLength, byte [] data){
        this.result = result;
        this.dataLength = dataLength;
        this.data = data;
    }

    public static native int kantv_anti_remove_rename_this_file();
}
