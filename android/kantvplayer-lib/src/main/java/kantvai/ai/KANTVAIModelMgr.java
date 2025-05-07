 /*
  * Copyright (c) 2024- KanTV Authors
  */
 package kantvai.ai;

 import kantvai.media.player.KANTVLog;

 public class KANTVAIModelMgr {
     private static final String TAG = KANTVAIModelMgr.class.getSimpleName();

     private int defaultLLMModelIndex       = 4; //index of the default LLM model, default index is 4 (gemma-3-4b)
     private final int LLM_MODEL_COUNTS     = 8; // counts of LLM models
     private final int NON_LLM_MODEL_COUNTS = 2; // counts of non LLM models:1 ASR model ggml-tiny.en-q8_0.bin + 1 StableDiffusion model sd-v1-4.ckpt

     private int capacity                   = LLM_MODEL_COUNTS + NON_LLM_MODEL_COUNTS;


     private KANTVAIModel[] AIModels;           //contains all LLM models + ASR model ggml-tiny.en-q8_0.bin + StableDiffusion model sd-v1-4.ckpt
     private String[] arrayModelName;
     private static KANTVAIModelMgr instance      = null;
     private static volatile boolean isInitModels = false;

     private int modelIndex  = 0;
     private int modelCounts = 0;              //contains all LLM models + ASR model ggml-tiny.en-q8_0.bin + StableDiffusion model sd-v1-4.ckpt

     private KANTVAIModelMgr() {
         AIModels = new KANTVAIModel[capacity];
         arrayModelName = new String[capacity];
     }

     public static KANTVAIModelMgr getInstance() {
         if (!isInitModels) {
             instance = new KANTVAIModelMgr();
             instance.initAIModels();
             isInitModels = true;
         } else {
             KANTVLog.d(TAG, "KANTVAIModelMgr already inited");
         }
         return instance;
     }

     private void checkCapacity() {
         if (modelIndex == capacity) {
             capacity *= 2;
             KANTVAIModel[] newAIModels = new KANTVAIModel[capacity];
             for (int idx = 0; idx < modelIndex; idx++) {
                 newAIModels[idx] = AIModels[idx];
             }
             AIModels = newAIModels;
         }
     }

     private void addAIModel(KANTVAIModel.AIModelType type, String nick, String name, String url) {
         checkCapacity();
         AIModels[modelIndex] = new KANTVAIModel(modelIndex, type, nick, name, url);
         modelIndex++;
     }

     private void addAIModel(KANTVAIModel.AIModelType type, String nick, String name, String url, long size) {
         KANTVLog.g(TAG,"modelIndex " + modelIndex + " capacity " + capacity);
         checkCapacity();
         AIModels[modelIndex] = new KANTVAIModel(modelIndex, type, nick, name, url, size);
         modelIndex++;
     }


     private void addAIModel(KANTVAIModel.AIModelType type, String nick, String name, String mmprojName, String url, String mmprojUrl, long modelSize, long mmprojModelSize) {
         checkCapacity();
         AIModels[modelIndex] = new KANTVAIModel(modelIndex, type, nick, name, mmprojName, url, mmprojUrl, modelSize, mmprojModelSize);
         modelIndex++;
     }

     public KANTVAIModel getKANTVAIModelFromName(String nickName) {
         for (int index = 0; index  < modelCounts; index++) {
             if (nickName.equals(AIModels[index].getNickname())) {
                 return AIModels[index];
             }
         }
         return null;
     }

     public KANTVAIModel getKANTVAIModelFromIndex(int modelIndex) {
         for (int index = 0; index  < modelCounts; index++) {
             if (modelIndex == AIModels[index].getIndex()) {
                 return AIModels[index];
             }
         }
         return null;
     }

     public KANTVAIModel getLLMModelFromIndex(int modelIndex) {
         for (int index = 0; index  < modelCounts; index++) {
             if (modelIndex == AIModels[index + NON_LLM_MODEL_COUNTS].getIndex()) {
                 return AIModels[index];
             }
         }
         return null;
     }

     public int getModelIndex(String nickName) {
         for (int index = 0; index  < modelCounts; index++) {
             if (nickName.equals(AIModels[index].getNickname())) {
                 return AIModels[index].getIndex();
             }
         }
         return 0;
     }

     public int getLLMModelIndex(String nickName) {
         for (int index = 0; index  < modelCounts; index++) {
             if (nickName.equals(AIModels[index].getNickname())) {
                 return AIModels[index].getIndex() - NON_LLM_MODEL_COUNTS;
             }
         }
         return 0;
     }

     public String[] getArrayModelName() {
         return arrayModelName;
     }

     public int getLLMModelCounts() {
         return modelCounts - NON_LLM_MODEL_COUNTS;
     }

     public int getNonLLMModelCounts() {
         return NON_LLM_MODEL_COUNTS;
     }

     public String getModelName(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getName();
     }

     public boolean isDownloadAble(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].isDownloadAble();
     }

     public String getNickname(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getNickname();
     }

     public String getModelUrl(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getUrl();
     }

     public String getMMProjmodelName(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getMMProjName();
     }

     public String getMMProjmodelUrl(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getMMProjUrl();
     }

     public int getDefaultModelIndex() {
         return defaultLLMModelIndex;
     }

     public void setDefaultModelIndex(int index) {
         defaultLLMModelIndex = index;
     }

     public long getModelSize(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getSize();
     }

     public long getMMProjmodelSize(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getMMprojSize();
     }

     //how to convert safetensors to GGUF and quantize LLM model:https://www.kantvai.com/posts/Convert-safetensors-to-gguf.html
     private void initAIModels() {
         KANTVLog.g(TAG, "init AI Models");

         addAIModel(KANTVAIModel.AIModelType.TYPE_ASR, "tiny.en-q8_0", "ggml-tiny.en-q8_0.bin",
                 "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q8_0.bin",
                 43550795 //the built-in and default ASR model, size is 42 MiB
         );
         //there are only one Whisper model currently
         AIModels[0].setSample("jfk.wav", 43550795,
                 "https://huggingface.co/datasets/Xenova/transformers.js-docs/resolve/main/jfk.wav");


         //there are only one StableDiffusion model currently
         addAIModel(KANTVAIModel.AIModelType.TYPE_TEXT2IMAGE, "sd-v1.4", "sd-v1-4.ckpt",
                 "https://huggingface.co/CompVis/stable-diffusion-v-1-4-original/resolve/main/sd-v1-4.ckpt",
                 4265380512L); // size of the StableDiffusion model, about 4.0 GiB


         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen1.5-1.8B", "qwen1_5-1_8b-chat-q4_0.gguf",
                 "https://huggingface.co/Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_0.gguf?download=true",
                 (long)(1.12 * 1024 * 1024 * 1024L)
                 );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-3B", "qwen2.5-3b-instruct-q4_0.gguf",
                 "https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_0.gguf?download=true",
                 (long)(2 * 1024 * 1024 * 1024)
                 );


         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-VL-3B",
                 "Qwen2.5-VL-3B-Instruct-Q4_K_M.gguf", "mmproj-Qwen2.5-VL-3B-Instruct-Q8_0.gguf",
                 "https://huggingface.co/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/Qwen2.5-VL-3B-Instruct-Q4_K_M.gguf?download=true",
                 "https://huggingface.co/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/mmproj-Qwen2.5-VL-3B-Instruct-Q8_0.gguf?download=true",
                 (long)(1.93 * 1024 * 1024 * 1024L),
                 845 * 1024 * 1024L
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-4B", "Qwen3-4B-Q8_0.gguf",
                 "https://huggingface.co/ggml-org/Qwen3-4B-GGUF/resolve/main/Qwen3-4B-Q8_0.gguf?download=true",
                 (long)(4.28 * 1024 * 1024 * 1024L));

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-8B", "Qwen3-8B-Q8_0.gguf",
                 "https://huggingface.co/ggml-org/Qwen3-8B-GGUF/resolve/main/Qwen3-8B-Q8_0.gguf?download=true",
                 (long)(8.71 * 1024 * 1024 * 1024L));

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma3-4B", "gemma-3-4b-it-Q8_0.gguf", "mmproj-gemma3-4b-f16.gguf",
                 "https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q8_0.gguf?download=true",
                 "https://huggingface.co/ggml-org/gemma-3-4b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true",
                 4130226336L,//size of the main model in bytes, 4.13 GiB
                 851251104L //size of the mmproj model in bytes, 851 MiB
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma3-12B", "gemma-3-12b-it-Q4_K_M.gguf", "mmproj-gemma3-12b-f16.gguf",
                 "https://huggingface.co/ggml-org/gemma-3-12b-it-GGUF/resolve/main/gemma-3-12b-it-Q4_K_M.gguf?download=true",
                 "https://huggingface.co/ggml-org/gemma-3-12b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true",
                 7300574976L,
                 854200224L
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "DS-R1-Distill-Qwen-1.5B", "DeepSeek-R1-Distill-Qwen-1.5B-Q8_0.gguf",
                 "https://huggingface.co/deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B", 1646570368L);

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "DS-R1-Distill-Qwen-7B", "DeepSeek-R1-Distill-Qwen-7B-Q8_0.gguf",
                 "https://huggingface.co/deepseek-ai/DeepSeek-R1-Distill-Qwen-7B/tree/main", 8098524896L);

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Nemotron-Nano-8B-v1", "Llama-3.1-Nemotron-Nano-8B-v1.gguf",
                 "https://huggingface.co/nvidia/Llama-3.1-Nemotron-Nano-8B-v1/tree/main");


         modelCounts = modelIndex;
         //initialize arrayModeName for UI
         arrayModelName = new String[modelCounts];
         for (int i = 0; i < modelCounts; i++) {
             arrayModelName[i] = AIModels[i].getNickname();
         }

         if (getKANTVAIModelFromName("Gemma3-4B") != null) {
             setDefaultModelIndex(getKANTVAIModelFromName("Gemma3-4B").getIndex() - NON_LLM_MODEL_COUNTS);
         }

         //UT for download the default LLM model in APK
         //AIModels[defaultLLMModelIndex + NON_LLM_MODEL_COUNTS].setUrl("http://192.168.0.200/gemma-3-4b-it-Q8_0.gguf"); //download url of the LLM main model
         //AIModels[defaultLLMModelIndex + NON_LLM_MODEL_COUNTS].setMMprojUrl("http://192.168.0.200/mmproj-gemma3-4b-f16.gguf");//download url of the LLM mmproj model

         //UT for download Qwen2.5-VL-3B from huggingface's mirror in China, validate ok although it seems that this mirror site is NOT stable
         //if (getKANTVAIModelFromName("Qwen2.5-VL-3B") != null) {
         //    getKANTVAIModelFromName("Qwen2.5-VL-3B").setUrl("https://hf-mirror.com/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/Qwen2.5-VL-3B-Instruct-Q4_K_M.gguf?download=true");
         //    getKANTVAIModelFromName("Qwen2.5-VL-3B").setMMprojUrl("https://hf-mirror.com/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/mmproj-Qwen2.5-VL-3B-Instruct-Q8_0.gguf?download=true");
         //}
     }
 }
