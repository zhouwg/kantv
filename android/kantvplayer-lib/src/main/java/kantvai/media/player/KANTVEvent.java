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


 final public class KANTVEvent {
     //keep sync with KANTV_event.h in native layer
     public final static int KANTV_ERROR = 100;
     public final static int KANTV_INFO = 200;

     public final static int KANTV_INFO_PREVIEW_START = 0;
     public final static int KANTV_INFO_PREVIEW_STOP = 1;
     public final static int KANTV_INFO_SETPREVIEWDISPLAY = 2;
     public final static int KANTV_INFO_SETRENDERID = 3;
     public final static int KANTV_INFO_OPEN = 4;
     public final static int KANTV_INFO_CLOSE = 5;
     public final static int KANTV_INFO_PREVIEW = 8;
     public final static int KANTV_INFO_GRAPHIC_BENCHMARK_START = 9;
     public final static int KANTV_INFO_GRAPHIC_BENCHMARK_STOP = 10;
     public final static int KANTV_INFO_ENCODE_BENCHMARK_START = 11;
     public final static int KANTV_INFO_ENCODE_BENCHMARK_STOP = 12;
     public final static int KANTV_INFO_ENCODE_BENCHMARK_INFO = 13;
     public final static int KANTV_INFO_STATUS = 14;

     public final static int KANTV_INFO_ASR_INIT = 15;
     public final static int KANTV_INFO_ASR_FINALIZE = 16;

     public final static int KANTV_INFO_ASR_START = 17;
     public final static int KANTV_INFO_ASR_STOP = 18;
     public final static int KANTV_INFO_ASR_GGML_INTERNAL = 19;
     public final static int KANTV_INFO_ASR_WHISPERCPP_INTERNAL = 20;
     public final static int KANTV_INFO_ASR_RESULT = 21;

     public final static int KANTV_INFO_LLM_START = 30;
     public final static int KANTV_INFO_LLM_STOP = 31;

     public final static int KANTV_INFO_LLM_INIT = 40;
     public final static int KANTV_INFO_LLM_FINALIZE = 41;

     public final static int KANTV_ERROR_PREVIEW_START = 0;
     public final static int KANTV_ERROR_PREVIEW_STOP = 1;
     public final static int KANTV_ERROR_SETPREVIEWDISPLAY = 2;
     public final static int KANTV_ERROR_SETRENDERID = 3;
     public final static int KANTV_ERROR_OPEN = 4;
     public final static int KANTV_ERROR_CLOSE = 5;
     public final static int KANTV_ERROR_PREVIEW = 8;
     public final static int KANTV_ERROR_GRAPHIC_BENCHMARK = 9;
     public final static int KANTV_ERROR_ENCODE_BENCHMARK = 10;

     public final static int KANTV_ERROR_ASR_INIT = 11;
     public final static int KANTV_ERROR_ASR_FINALiZE = 12;
     public final static int KANTV_ERROR_ASR_START = 13;
     public final static int KANTV_ERROR_ASR_STOP = 14;
     public final static int KANTV_ERROR_ASR_GGML_INTERNAL = 15;
     public final static int KANTV_ERROR_ASR_WHISPERCPP_INTERNAL = 16;
     public final static int KANTV_ERROR_ASR_RESULT = 17;

     public static native int kantv_anti_remove_rename_this_file();
 }
