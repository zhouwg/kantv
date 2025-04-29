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

 import static java.nio.file.StandardCopyOption.COPY_ATTRIBUTES;
 import static java.nio.file.StandardCopyOption.REPLACE_EXISTING;

 import android.annotation.SuppressLint;
 import android.content.Context;

 import java.io.BufferedReader;
 import java.io.File;
 import java.io.FileOutputStream;
 import java.io.FileReader;
 import java.io.IOException;
 import java.io.InputStream;
 import java.io.OutputStream;
 import java.nio.file.Files;
 import java.nio.file.Path;
 import java.nio.file.Paths;


 public class KANTVAssetLoader {
     private static final String TAG = KANTVAssetLoader.class.getName();

     public static String getDataPath(Context context) {
         return new StringBuilder("/data/data/").append(context.getPackageName()).append("/").toString();
     }

     public static void copyAssetFile(Context context, String srcFilePath, String destFilePath) {
         KANTVLog.j(TAG, "src path: " + srcFilePath);
         KANTVLog.j(TAG, "dst path: " + destFilePath);
         InputStream inStream = null;
         FileOutputStream outStream = null;
         try {
             File destFile = new File(destFilePath);

             if (destFile.exists()) {
                 if (destFilePath.contains("ggml-hexagon.cfg")) {
                     //ensure using updated ggml-hexagon.cfg
                     destFile.delete();
                 }
             }

             if (destFile.exists() && destFile.isFile()) {
                 // 03-30-2024 17:09:35.152 24446 24797 D KANTV   : [LogUtils.cpp, logStdoutCallback, 48]:
                 // 0.3ms [ ERROR ] Unable to load backend. pal::dynamicloading::dlError():
                 // dlopen failed: file offset for the library "/data/data/com.kantvai.kantv/libQnnCpu.so" >= file size: 0 >= 0


                 //03-30-2024 17:15:23.949 25507 25796 D KANTV   : [LogUtils.cpp, logStdoutCallback, 48]:
                 // 6.5ms [ ERROR ] Unable to load model. pal::dynamicloading::dlError():
                 // dlopen failed: library "/sdcard/kantv/libInception_v3.so" needed or dlopened by
                 // "/data/app/~~70peMvcNIhRmzhm-PhmfRg==/com.kantvai.kantv-bUwy7gbMeCP0JFLe1J058g==/base.apk!/lib/arm64-v8a/libggml-jni.so"
                 // is not accessible for the namespace "clns-4"

                 KANTVLog.j(TAG, "dst file already exist");
                 if (0 == destFile.length()) {
                     destFile.delete();
                     destFile.createNewFile();
                 } else {
                     return;
                 }
             } else {
                 destFile.createNewFile();
             }

             inStream = context.getAssets().open(srcFilePath);
             outStream = new FileOutputStream(destFilePath);

             int bytesRead = 0;
             byte[] buf = new byte[2048];
             while ((bytesRead = inStream.read(buf)) != -1) {
                 outStream.write(buf, 0, bytesRead);
             }
         } catch (Exception e) {
             KANTVLog.j(TAG, "error: " + e.toString());
             e.printStackTrace();
         } finally {
             close(inStream);
             close(outStream);
         }
     }

     public static void copyAssetDir(Context context, String srcDirPath, String destDirPath) {
         KANTVLog.j(TAG, "src path: " + srcDirPath);
         KANTVLog.j(TAG, "dst path: " + destDirPath);
         try {
             File destFile = new File(destDirPath);
             if (destFile.exists()) {
                 KANTVLog.j(TAG, "dir: " + destDirPath + " already exist");
                 return;
             } else {
                 KANTVLog.j(TAG, "dir: " + destDirPath + " not exist");
                 destFile.mkdirs();
             }

             String fileNames[] = context.getAssets().list(srcDirPath);
             if (fileNames.length > 0) {
                 File file = new File(destDirPath);
                 file.mkdirs();
                 for (String fileName : fileNames) {
                     copyAssetDir(context, srcDirPath + File.separator + fileName, destDirPath + File.separator + fileName);
                 }
             } else {
                 InputStream is = context.getAssets().open(srcDirPath);
                 FileOutputStream fos = new FileOutputStream(new File(destDirPath));
                 byte[] buffer = new byte[2048];
                 int byteCount = 0;
                 while ((byteCount = is.read(buffer)) != -1) {
                     fos.write(buffer, 0, byteCount);
                 }
                 fos.flush();
                 is.close();
                 fos.close();
             }
             //Path copyFrom = Paths.get(srcDirPath);
             //Path copyTo = Paths.get(destDirPath);
             //Files.copy(copyFrom, copyTo, REPLACE_EXISTING, COPY_ATTRIBUTES);
         } catch (Exception e) {
             KANTVLog.j(TAG, "error: " + e.toString());
             e.printStackTrace();
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
             KANTVLog.j(TAG, "error: " + e.toString());
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
                 } catch (IOException e) {
                     e.printStackTrace();
                     KANTVLog.j(TAG, "error: " + e.toString());
                 }
             }
         }
         return sbf.toString();
     }

     public static native int kantv_anti_remove_rename_this_file();
 }
