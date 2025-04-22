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

 import java.util.Date;
 import java.util.Formatter;

 import android.util.Log;

 public class KANTVLog {
     public static boolean DEBUG = true;
     public static int v = 1;
     public static int d = 2;
     public static int i = 3;
     public static int w = 4;
     public static int e = 5;
     public static int j = 6;

     /*
      * @param logLevel 1 v ; 2 d ; 3 i ; 4 w ; 5 e .
      */
     public static void showLog(int logLevel, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode()) {
             DEBUG = false;
         }

         switch (logLevel) {
             case 1:
                 if (DEBUG)
                     Log.v(infos[0], info + " : " + infos[1] + infos[2]);
                 break;
             case 2:
                 if (DEBUG)
                     Log.d(infos[0], info + " : " + infos[1] + infos[2]);
                 break;
             case 3:
                 if (DEBUG)
                     Log.i(infos[0], infos[1] + infos[2] + info);
                 break;
             case 4:
                 if (DEBUG)
                     Log.w(infos[0], infos[1] + infos[2] + info);
                 break;
             case 5:
                 if (DEBUG)
                     Log.e(infos[0], info + " : " + infos[1] + infos[2]);
                 break;
             case 6:
                 if (DEBUG)
                     Log.d(infos[0], info + " : " + infos[1] + infos[2]);
                 break;
         }
     }


     public static void v(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode())
             DEBUG = false;

         if (DEBUG)
             Log.v(infos[0], infos[1] + infos[2] + info);
     }


     public static void d(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode())
             DEBUG = false;

         if (DEBUG)
             Log.d(infos[0], infos[1] + infos[2] + info);
     }


     public static void i(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode())
             DEBUG = false;

         if (DEBUG)
             Log.i(infos[0], infos[1] + infos[2] + info);
     }


     public static void w(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode())
             DEBUG = false;

         if (DEBUG)
             Log.w(infos[0], infos[1] + infos[2] + info);
     }


     public static void e(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         if (KANTVUtils.getReleaseMode())
             DEBUG = false;

         if (DEBUG)
             Log.e(infos[0], infos[1] + infos[2] + info);
     }

     public static void j(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();

         if (!KANTVUtils.getReleaseMode()) {
             Log.d(infos[0], infos[1] + infos[2] + info);
         }
     }


     public static void g(String tag, String info) {
         String[] infos = getAutoJumpLogInfos();
         Log.d(infos[0], infos[1] + infos[2] + info);
     }

     private static String[] getAutoJumpLogInfos() {
         String[] infos = new String[]{"", "", ""};
         StackTraceElement[] elements = Thread.currentThread().getStackTrace();
         if (elements.length < 5) {
             Log.e("KANTV", "invalid stack info");
             return infos;
         } else {
             infos[0] = elements[4].getClassName().substring(
                     elements[4].getClassName().lastIndexOf(".") + 1);
             //FIXME: can't get line number on some Android devices
            /*
            infos[1] = elements[4].getMethodName() + "()";
            infos[2] = " at (" + elements[4].getClassName() + ".java:"
                       + elements[4].getLineNumber() + ")";
            */
             //add prefix KANTV in Java Layer and Native layer to make adb logcat happy
             infos[1] = "[KANTV][" + elements[4].getClassName() + ".java:"
                     + elements[4].getLineNumber() + ",";
             infos[2] = elements[4].getMethodName() + "()" + "] ";
             return infos;
         }

     }


     private static class ReusableFormatter {
         private Formatter formatter;
         private StringBuilder builder;

         public ReusableFormatter() {
             builder = new StringBuilder();
             formatter = new Formatter(builder);
         }

         public String format(String msg, Object... args) {
             formatter.format(msg, args);
             String s = builder.toString();
             builder.setLength(0);
             return s;
         }
     }


     private static final ThreadLocal thread_local_formatter = new ThreadLocal() {
         protected ReusableFormatter initialValue() {
             return new ReusableFormatter();
         }
     };


     public static String format(String msg, Object... args) {
         ReusableFormatter formatter = (ReusableFormatter) (thread_local_formatter.get());
         return formatter.format(msg, args);
     }


     public static native int kantv_anti_remove_rename_this_file();
 }
