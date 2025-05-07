 /*
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

package kantvai.ai;

public final class KANTVAIUtils {

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
