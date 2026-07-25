/*
 * Copyright (c) 2024- KanTV Authors
 */
package kantvai.ai;

import java.util.Locale;

import kantvai.media.player.KANTVLibraryLoader;
import kantvai.media.player.KANTVLog;

public class KANTVAIModelMgr {
     private static final String TAG = KANTVAIModelMgr.class.getSimpleName();

     private int defaultLLMModelIndex       = 4; //index of the default LLM model, default index is 4 (gemma-3-4b)
     private final int LLM_MODEL_COUNTS     = 8; // default counts of LLM models, might-be not the real counts of all LLM models
     private int NON_LLM_MODEL_COUNTS = 1; // counts of non LLM models:1 ASR model ggml-tiny.en-q8_0.bin

     private int capacity                   = LLM_MODEL_COUNTS + NON_LLM_MODEL_COUNTS; // default capacity of all AI models


     private KANTVAIModel[] AIModels;           //contains all LLM models + ASR model ggml-tiny.en-q8_0.bin
     private String[] arrayModelName;           //space/memory ---> time/performance
     private String[] arrayBenchType;
     private static KANTVAIModelMgr instance      = null;
     private static volatile boolean isInitModels = false;

     private int modelIndex  = 0;
     private int modelCounts = 0;              //contains all LLM models + ASR model ggml-tiny.en-q8_0.bin

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


     private void addAIModel(KANTVAIModel.AIModelType type, String nick, String name, String mmprojName, String url, String mmprojUrl) {
         checkCapacity();
         AIModels[modelIndex] = new KANTVAIModel(modelIndex, type, nick, name, mmprojName, url, mmprojUrl);
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
         return replaceHFEndpoint(AIModels[index + NON_LLM_MODEL_COUNTS].getUrl());
     }

     public String getMMProjmodelName(int index) {
         return AIModels[index + NON_LLM_MODEL_COUNTS].getMMProjName();
     }

     public String getMMProjmodelUrl(int index) {
         return replaceHFEndpoint(AIModels[index + NON_LLM_MODEL_COUNTS].getMMProjUrl());
     }

     // Replace the HF endpoint domain in the stored URL with the currently configured one.
     // This is necessary because initAIModels() runs only once (singleton), so when the user
     // switches between huggingface.co and hf-mirror.com in Settings, the stored URLs are
     // not regenerated. Returning a rewritten copy here ensures downloads always use the
     // endpoint the user currently selected, without re-running initAIModels().
     private String replaceHFEndpoint(String url) {
         if (url == null) {
             return null;
         }
         String currentEndpoint = KANTVAIUtils.getHFEndPointUrl(KANTVAIUtils.getHFEndpoint());
         url = url.replace("https://huggingface.co/", currentEndpoint);
         url = url.replace("https://hf-mirror.com/", currentEndpoint);
         return url;
     }

     public int getDefaultModelIndex() {
         return defaultLLMModelIndex;
     }

     public void setDefaultModelIndex(int index) {
         defaultLLMModelIndex = index;
     }

     private void initAIModels() {
         String hf_endpoint = "https://huggingface.co/"; //the official default HuggingFace site
         KANTVLog.g(TAG, "init AI Models");
         Locale local = Locale.getDefault();
         String language = local.getLanguage();
         KANTVLog.g(TAG, "language " + language);
         int hfendpoint = KANTVAIUtils.getHFEndpoint();
         KANTVLog.g(TAG, "hfendpoint " + hfendpoint);
         //if (language.equals("zh") || (1 == hfendpoint)) {
         if (1 == hfendpoint) {
             hf_endpoint = "https://hf-mirror.com/"; //the mirror HuggingFace site in China
         }

         try {
             KANTVLibraryLoader.load("ggml-jni");
             KANTVLog.g(TAG, "cpu core counts:" + ggmljava.get_cpu_core_counts());
         } catch (Exception e) {
             KANTVLog.g(TAG, "failed to initialize ggml jni");
             return;
         }

         boolean isGGMLHexagonEnabled = ggmljava.isGGMLHexagonEnabled();
         KANTVLog.g(TAG, "isGGMLHexagonEnabled: " + isGGMLHexagonEnabled);

         arrayBenchType = new String[2];
         arrayBenchType[0] = "ASR";
         arrayBenchType[1] = "LLM";

         addAIModel(KANTVAIModel.AIModelType.TYPE_ASR, "tiny.en-q8_0", "ggml-tiny.en-q8_0.bin",
                 hf_endpoint + "ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q8_0.bin"
         );
         //there are only one Whisper model currently
         AIModels[0].setSample("jfk.wav",
                 hf_endpoint + "datasets/Xenova/transformers.js-docs/resolve/main/jfk.wav");


         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen1.5-1.8B", "qwen1_5-1_8b-chat-q4_0.gguf",
                 hf_endpoint + "Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_0.gguf?download=true"
                 );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-3B", "qwen2.5-3b-instruct-q4_0.gguf",
                 hf_endpoint + "Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_0.gguf?download=true"
                 );

         //LLM + MTMD-image
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma3-4B", "gemma-3-4b-it-Q8_0.gguf", "mmproj-gemma3-4b-f16.gguf",
                 hf_endpoint + "ggml-org/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q8_0.gguf?download=true",
                 hf_endpoint + "ggml-org/gemma-3-4b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true"
         );

         //LLM (text-only)
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma-4-E2B", "gemma-4-E2B-it-Q4_0.gguf",
                 hf_endpoint + "unsloth/gemma-4-E2B-it-GGUF/resolve/main/gemma-4-E2B-it-Q4_0.gguf?download=true"
         );

         //MTMD-image(for realtime-video-inference)
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "SmolVLM2-256M",
                 "SmolVLM2-256M-Video-Instruct-f16.gguf", "mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf",
                 hf_endpoint + "ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/SmolVLM2-256M-Video-Instruct-f16.gguf?download=true",
                 hf_endpoint + "ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf?download=true"
         );

         //MTMD-audio
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-Omni-3B",
                 "Qwen2.5-Omni-3B-Q4_K_M.gguf",
                 "mmproj-Qwen2.5-Omni-3B-Q8_0.gguf",
                 hf_endpoint + "ggml-org/Qwen2.5-Omni-3B-GGUF/resolve/main/Qwen2.5-Omni-3B-Q4_K_M.gguf?download=true",
                 hf_endpoint + "ggml-org/Qwen2.5-Omni-3B-GGUF/resolve/main/mmproj-Qwen2.5-Omni-3B-Q8_0.gguf?download=true"
         );

         modelCounts = modelIndex;  //modelCounts is real counts of all AI models
         //initialize arrayModeName for UI AIResearchFragment.java to display all AI models(1 ASR model + all LLM models + others)
         arrayModelName = new String[modelCounts];
         for (int i = 0; i < modelCounts; i++) {
             arrayModelName[i] = AIModels[i].getNickname();
         }

         if (getKANTVAIModelFromName("Gemma-4-E2B") != null) {
             setDefaultModelIndex(getKANTVAIModelFromName("Gemma-4-E2B").getIndex() - NON_LLM_MODEL_COUNTS);
         }
     }
 }
