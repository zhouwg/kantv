/*
 * Copyright (c) 2024- KanTV Authors
 */
package kantvai.ai;

import kantvai.media.player.KANTVLibraryLoader;
import kantvai.media.player.KANTVLog;

public class KANTVAIModelMgr {
     private static final String TAG = KANTVAIModelMgr.class.getSimpleName();

     private int defaultLLMModelIndex       = 4; //index of the default LLM model, default index is 4 (gemma-3-4b)
     private final int LLM_MODEL_COUNTS     = 8; // default counts of LLM models, might-be not the real counts of all LLM models
     private int NON_LLM_MODEL_COUNTS = 2; // counts of non LLM models:1 ASR model ggml-tiny.en-q8_0.bin + 1 StableDiffusion model sd-v1-4.ckpt

     private int capacity                   = LLM_MODEL_COUNTS + NON_LLM_MODEL_COUNTS; // default capacity of all AI models


     private KANTVAIModel[] AIModels;           //contains all LLM models + ASR model ggml-tiny.en-q8_0.bin + StableDiffusion model sd-v1-4.ckpt
     private String[] arrayModelName;           //space/memory ---> time/performance
     private String[] arrayBenchType;
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

     public String[] getAllAIModelNickName() {
         return arrayModelName;
     }

    public String[] getAllAIModelBenchType() {
         return arrayBenchType;
    }

     public String[] getAllLLMModelNickName() {
         String[] arrayLLMModelsName = new String[getLLMModelCounts()];
         for (int i = 0; i < getLLMModelCounts(); i++) {
             arrayLLMModelsName[i] = AIModels[i + NON_LLM_MODEL_COUNTS].getNickname();
         }
         return arrayLLMModelsName;
     }

     /*
       return the real counts of all LLM models
      */
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
         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.g(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.g(TAG, "failed to initialize ggml jni");
             return;
         }

         boolean isStableDiffusionEnabled = ggmljava.isStableDiffusionEnabled();
         boolean isGGMLHexagonEnabled = ggmljava.isGGMLHexagonEnabled();
         KANTVLog.g(TAG, "isGGMLHexagonEnabled: " + isGGMLHexagonEnabled);

         if (isStableDiffusionEnabled) {
             arrayBenchType = new String[5];
             arrayBenchType[0] = "memcpy";
             arrayBenchType[1] = "mulmat";
             arrayBenchType[2] = "ASR";
             arrayBenchType[3] = "LLM";
             arrayBenchType[4] = "Text2Image";
         } else {
             arrayBenchType = new String[4];
             arrayBenchType[0] = "memcpy";
             arrayBenchType[1] = "mulmat";
             arrayBenchType[2] = "ASR";
             arrayBenchType[3] = "LLM";
         }

         addAIModel(KANTVAIModel.AIModelType.TYPE_ASR, "tiny.en-q8_0", "ggml-tiny.en-q8_0.bin",
                 "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q8_0.bin",
                 43550795 //the built-in and default ASR model, size is 42 MiB
         );
         //there are only one Whisper model currently
         AIModels[0].setSample("jfk.wav", 43550795,
                 "https://huggingface.co/datasets/Xenova/transformers.js-docs/resolve/main/jfk.wav");


         if (isStableDiffusionEnabled) {
             //there are only one StableDiffusion model currently
             addAIModel(KANTVAIModel.AIModelType.TYPE_TEXT2IMAGE, "sd-v1.4", "sd-v1-4.ckpt",
                     "https://huggingface.co/CompVis/stable-diffusion-v-1-4-original/resolve/main/sd-v1-4.ckpt",
                     4265380512L); // size of the StableDiffusion model, about 4.0 GiB
         } else {
             NON_LLM_MODEL_COUNTS = 1;
         }

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
                 4280404640L);

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-8B", "Qwen3-8B-Q8_0.gguf",
                 "https://huggingface.co/ggml-org/Qwen3-8B-GGUF/resolve/main/Qwen3-8B-Q8_0.gguf?download=true",
                 (long)(8.71 * 1024 * 1024 * 1024L));

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-14B", "Qwen3-14B-Q4_K_M.gguf",
                 "https://huggingface.co/Qwen/Qwen3-14B-GGUF/resolve/main/Qwen3-14B-Q4_K_M.gguf?download=true",
                 (long)(9 * 1024 * 1024 * 1024L));

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

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "SmolVLM-500M", "SmolVLM-500M-Instruct-f16.gguf", "mmproj-SmolVLM-500M-Instruct-f16.gguf",
                 "https://huggingface.co/ggml-org/SmolVLM-500M-Instruct-GGUF/resolve/main/SmolVLM-500M-Instruct-f16.gguf?download=true",
                 "https://huggingface.co/ggml-org/SmolVLM-500M-Instruct-GGUF/resolve/main/mmproj-SmolVLM-500M-Instruct-f16.gguf?download=true",
                 820422912L,
                 199468800L
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "SmolVLM2-256M",
                 "SmolVLM2-256M-Video-Instruct-f16.gguf", "mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf",
                 "https://huggingface.com/ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/SmolVLM2-256M-Video-Instruct-f16.gguf?download=true",
                 "https://huggingface.com/ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf?download=true",
                 327811552L,
                 190033440L
         );


         modelCounts = modelIndex;  //modelCounts is real counts of all AI models
         //initialize arrayModeName for UI AIResearchFragment.java to display all AI models(1 ASR model + all LLM models + others)
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

         //UT for download Qwen2.5-VL-3B from huggingface's mirror site, validate ok although it seems that this site is not stable
         //if (getKANTVAIModelFromName("Qwen2.5-VL-3B") != null) {
         //    getKANTVAIModelFromName("Qwen2.5-VL-3B").setUrl("https://hf-mirror.com/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/Qwen2.5-VL-3B-Instruct-Q4_K_M.gguf?download=true");
         //    getKANTVAIModelFromName("Qwen2.5-VL-3B").setMMprojUrl("https://hf-mirror.com/ggml-org/Qwen2.5-VL-3B-Instruct-GGUF/resolve/main/mmproj-Qwen2.5-VL-3B-Instruct-Q8_0.gguf?download=true");
         //}

         //UT for download Qwen3-14B from huggingface's mirror site, validate ok although it seems that this site is not stable
         //if (getKANTVAIModelFromName("Qwen3-14B") != null) {
         //    getKANTVAIModelFromName("Qwen3-14B").setUrl("https://hf-mirror.com/Qwen/Qwen3-14B-GGUF/resolve/main/Qwen3-14B-Q4_K_M.gguf?download=true");
         //}
     }
 }
