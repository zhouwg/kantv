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

package tv.danmaku.ijk.kantv.content;

import android.content.Context;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class AssetLoader {
    public static String getDataPath(Context context) {
        return new StringBuilder("/data/data/").append(context.getPackageName()).append("/").toString();
    }

    public static void copyAssetFile(Context context, String srcFilePath, String destFilePath) {
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            inStream = context.getAssets().open(srcFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[2048];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }

    public static void close(InputStream is) {
        if (is == null) return;
        try {
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void close(OutputStream os) {
        if (os == null) return;
        try {
            os.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static String readTextFromFile(String fileName) {
        File file = new File(fileName);
        BufferedReader reader = null;
        StringBuffer sbf = new StringBuffer();
        try {
            reader = new BufferedReader(new FileReader(file));
            String tempStr;
            while ((tempStr = reader.readLine()) != null) {
                sbf.append(tempStr);
            }
            reader.close();
            return sbf.toString();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
            }
        }
        return sbf.toString();
    }
}
