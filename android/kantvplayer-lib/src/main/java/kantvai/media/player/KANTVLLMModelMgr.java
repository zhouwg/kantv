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
package kantvai.media.player;

public class KANTVLLMModelMgr {
    private static final String TAG = KANTVLLMModelMgr.class.getSimpleName();

    //https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main
    //default LLM model
    private String defaultModelName = "gemma-3-4b-it-Q8_0.gguf"; //4.1 GiB
    private int defaultModelIndex = 4; //index of selected LLM model, default index is 4 (gemma-3-4b)
    private final int LLM_MODEL_MAXCOUNTS    = 8;
    private KANTVLLMModel[] LLMModels        = new KANTVLLMModel[LLM_MODEL_MAXCOUNTS];
    private String[]        arrayModelName   = new String[LLM_MODEL_MAXCOUNTS + 1]; //contains all LLM models + ggml-tiny.en-q8_0.bin

    private static KANTVLLMModelMgr instance = null;
    private static volatile boolean isInitModels = false;

    private KANTVLLMModelMgr() {
        KANTVLog.g(TAG, "private constructor");
    }

    public static KANTVLLMModelMgr getInstance() {
        if (!isInitModels) {
            instance = new KANTVLLMModelMgr();
            instance.initLLMModels();
            isInitModels = true;
        } else {
            KANTVLog.d(TAG, "KANTVLLMModelMgr already inited");
        }
        return instance;
    }

    private void initLLMModels() {
        KANTVLog.g(TAG, "initLLMModels");
        //how to convert safetensors to GGUF and quantize LLM model:https://www.kantvai.com/posts/Convert-safetensors-to-gguf.html
        LLMModels[0] = new KANTVLLMModel(0, "Qwen1.5-1.8B", "qwen1_5-1_8b-chat-q4_0.gguf", "https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF/blob/main/qwen1_5-1_8b-chat-q4_0.gguf");
        LLMModels[1] = new KANTVLLMModel(1, "Qwen2.5-3B", "qwen2.5-3b-instruct-q4_0.gguf", "https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/tree/main");
        LLMModels[2] = new KANTVLLMModel(2, "Qwen3-4B","Qwen3-4B-Q8_0.gguf", "https://huggingface.co/Qwen/Qwen3-4B/tree/main");
        LLMModels[3] = new KANTVLLMModel(3, "Qwen3-8B", "Qwen3-8B-Q8_0.gguf", "https://huggingface.co/Qwen/Qwen3-8B");
        LLMModels[4] = new KANTVLLMModel(4, "Gemma3-4B", "gemma-3-4b-it-Q8_0.gguf","mmproj-gemma3-4b-f16.gguf", "https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/tree/main", "good");
        LLMModels[5] = new KANTVLLMModel(5, "Gemma3-12B", "gemma-3-12b-it-Q4_K_M.gguf", "mmproj-gemma3-12b-f16.gguf", "https://huggingface.co/ggml-org/gemma-3-12b-it-GGUF/tree/main", "good");
        LLMModels[6] = new KANTVLLMModel(6, "DS-R1-Distill-Qwen-1.5B", "DeepSeek-R1-Distill-Qwen-1.5B-Q8_0.gguf", "https://huggingface.co/deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B");
        LLMModels[7] = new KANTVLLMModel(7, "DS-R1-Distill-Qwen-7B", "DeepSeek-R1-Distill-Qwen-7B-Q8_0.gguf", "https://huggingface.co/deepseek-ai/DeepSeek-R1-Distill-Qwen-7B/tree/main");


        arrayModelName[0] = "ggml-tiny.en-q8_0.bin"; //the built-in and default ASR model, size is 42 MiB
        for (int i = 1; i <= LLM_MODEL_MAXCOUNTS; i++) {
            arrayModelName[i] = LLMModels[i - 1].getNickname();
        }
        LLMModels[0].setQuality("not bad and fast");
        LLMModels[1].setQuality("good");
        LLMModels[2].setQuality("not bad");
        LLMModels[3].setQuality("slow but impressive");//can understand word counts should be less then 100, but many repeated sentences
        LLMModels[4].setQuality("perfect"); //inference speed is fast and the answer is concise and accurate is exactly what I
        LLMModels[5].setQuality("slow and good");
        LLMModels[6].setQuality("bad"); //inference speed is fast but the answer is wrong
        LLMModels[7].setQuality("not bad"); //the answer is not concise

        //make LLM downloader happy
        LLMModels[4].setUrl("https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q8_0.gguf?download=true"); //4.13 GiB
        LLMModels[4].setMmprojUrl("https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true");//851 MiB
    }

    public String[] getArrayModelName() {
        return arrayModelName;
    }

    public int getLLMModelMaxCounts() {
        return LLM_MODEL_MAXCOUNTS;
    }

    public String getName(int index) {
        return LLMModels[index].getName();
    }

    public String getNickname(int index) {
        return LLMModels[index].getNickname();
    }

    public String getModelUrl(int index) {
        return LLMModels[index].getUrl();
    }

    public String getMMProjName(int index) {
        return LLMModels[index].getMMProjName();
    }

    public String getMMProjUrl(int index) {
        return LLMModels[index].getMMProjUrl();
    }

    public String getDefaultModelName() {
        return LLMModels[defaultModelIndex].getName();
    }

    public String getDefaultModelUrl() {
        return LLMModels[defaultModelIndex].getUrl();
    }

    public int getDefaultModelIndex() {
        return defaultModelIndex;
    }
}
