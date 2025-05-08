 /*
  * Copyright (c) 2024- KanTV Authors
  */

package kantvai.ai;

 import kantvai.media.player.KANTVLog;

 public final class KANTVAIUtils {

    //this is experimental value to check whether a specified AI model has downloaded successfully
    // naive algorithm at the moment:
    // if (real size of AI model - size of downloaded file == 1
    //     download ok
    // if (real size of AI model - size of downloaded file > DOWNLOAD_SIZE_CHECK_RANGE)
    //     download failure
    public static final long DOWNLOAD_SIZE_CHECK_RANGE = 700 * 1024 * 1024L;

    //FIXME: should I move these helper functions to KANTVAIModelMgr.java?
    public static boolean isASRModel(String name) {
        String[] asrModels = {
                "tiny",
                "tiny.en",
                "tiny.en-q5_1",
                "tiny.en-q8_0",
                "tiny-q5_1",
                "base",
                "base.en",
                "base-q5_1",
                "small",
                "small.en",
                "small.en-q5_1",
                "small-q5_1",
                "medium",
                "medium.en",
                "medium.en-q5_0",
                "large"
        };
        for (int i = 0; i < asrModels.length; i++) {
            if (name.contains(asrModels[i])) {
                return true;
            }
        }
        return false;
    }

    public static boolean isLLMVModel(String name) {
        String[] llmModels = {
                "gemma-3",
                "Qwen2.5-VL-3B",
        };
        for (int i = 0; i < llmModels.length; i++) {
            if (name.contains(llmModels[i])) {
                return true;
            }
        }
        return false;
    }
}
