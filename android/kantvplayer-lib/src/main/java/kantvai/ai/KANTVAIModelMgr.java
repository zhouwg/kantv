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

         boolean isStableDiffusionHexagonEnabled = ggmljava.isStableDiffusionHexagonEnabled();
         boolean isGGMLHexagonEnabled = ggmljava.isGGMLHexagonEnabled();
         KANTVLog.g(TAG, "isGGMLHexagonEnabled: " + isGGMLHexagonEnabled);

         arrayBenchType = new String[5];
         arrayBenchType[0] = "memcpy";
         arrayBenchType[1] = "mulmat";
         arrayBenchType[2] = "ASR";
         arrayBenchType[3] = "LLM";
         arrayBenchType[4] = "Text2Image";

         addAIModel(KANTVAIModel.AIModelType.TYPE_ASR, "tiny.en-q8_0", "ggml-tiny.en-q8_0.bin",
                 hf_endpoint + "ggerganov/whisper.cpp/resolve/main/ggml-tiny.en-q8_0.bin",
                 43550795 //the built-in and default ASR model, size is 42 MiB
         );
         //there are only one Whisper model currently
         AIModels[0].setSample("jfk.wav", 43550795,
                 hf_endpoint + "datasets/Xenova/transformers.js-docs/resolve/main/jfk.wav");


         //there are only one StableDiffusion model currently
         addAIModel(KANTVAIModel.AIModelType.TYPE_TEXT2IMAGE, "sd-v1.4", "sd-v1-4.ckpt",
                 hf_endpoint + "CompVis/stable-diffusion-v-1-4-original/resolve/main/sd-v1-4.ckpt",
                 4265380512L); // size of the StableDiffusion model, about 4.0 GiB


         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen1.5-1.8B", "qwen1_5-1_8b-chat-q4_0.gguf",
                 hf_endpoint + "Qwen/Qwen1.5-1.8B-Chat-GGUF/resolve/main/qwen1_5-1_8b-chat-q4_0.gguf?download=true",
                 1120235360L // size of LLM model, in bytes
                 );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-3B", "qwen2.5-3b-instruct-q4_0.gguf",
                 hf_endpoint + "Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_0.gguf?download=true",
                 1997879712L // size of LLM model, in bytes
                 );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-4B", "Qwen3-4B-Q8_0.gguf",
                 hf_endpoint + "ggml-org/Qwen3-4B-GGUF/resolve/main/Qwen3-4B-Q8_0.gguf?download=true",
                 4280404640L // size of LLM model, in bytes
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-8B", "Qwen3-8B-Q8_0.gguf",
                 hf_endpoint + "ggml-org/Qwen3-8B-GGUF/resolve/main/Qwen3-8B-Q8_0.gguf?download=true",
                 8709518464L  // size of LLM model, in bytes
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen3-14B", "Qwen3-14B-Q4_K_M.gguf",
                 hf_endpoint + "Qwen/Qwen3-14B-GGUF/resolve/main/Qwen3-14B-Q4_K_M.gguf?download=true",
                 9001752960L // size of LLM model, in bytes
         );

         //LLM + MTMD-image
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma3-4B", "gemma-3-4b-it-Q8_0.gguf", "mmproj-gemma3-4b-f16.gguf",
                 hf_endpoint + "ggml-org/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q8_0.gguf?download=true",
                 hf_endpoint + "ggml-org/gemma-3-4b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true",
                 4130226336L,//size of the main model in bytes, 4.13 GiB
                 851251104L //size of the mmproj model in bytes, 851 MiB
         );

         //LLM + MTMD-image
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Gemma3-12B", "gemma-3-12b-it-Q4_K_M.gguf", "mmproj-gemma3-12b-f16.gguf",
                 hf_endpoint + "ggml-org/gemma-3-12b-it-GGUF/resolve/main/gemma-3-12b-it-Q4_K_M.gguf?download=true",
                 hf_endpoint + "ggml-org/gemma-3-12b-it-GGUF/resolve/main/mmproj-model-f16.gguf?download=true",
                 7300574976L,
                 854200224L
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM,
             "Llama-3.1-Nemotron-Nano-4B",
             "Llama-3.1-Nemotron-Nano-4B-v1.1-Q4_K_M.gguf",
             hf_endpoint + "lmstudio-community/Llama-3.1-Nemotron-Nano-4B-v1.1-GGUF/resolve/main/Llama-3.1-Nemotron-Nano-4B-v1.1-Q4_K_M.gguf?download=true",
             2778285312L // size of LLM model, in bytes
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM,
                 "Phi-4-mini-reasoning",
                 "Phi-4-mini-reasoning-Q4_0.gguf",
                 hf_endpoint + "unsloth/Phi-4-mini-reasoning-GGUF/resolve/main/Phi-4-mini-reasoning-Q4_0.gguf?download=true",
                 2331443104L // size of LLM model, in bytes
         );

         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM,
                 "DeepSeek-R1-Distill-Qwen-7B",
                 "DeepSeek-R1-Distill-Qwen-7B-Q4_K_M.gguf",
                 hf_endpoint + "unsloth/DeepSeek-R1-Distill-Qwen-7B-GGUF/resolve/main/DeepSeek-R1-Distill-Qwen-7B-Q4_K_M.gguf?download=true",
                 4683073248L // size of LLM model, in bytes
         );

         //MTMD-image(for realtime-video-inference)
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "SmolVLM2-256M",
                 "SmolVLM2-256M-Video-Instruct-f16.gguf", "mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf",
                 hf_endpoint + "ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/SmolVLM2-256M-Video-Instruct-f16.gguf?download=true",
                 hf_endpoint + "ggml-org/SmolVLM2-256M-Video-Instruct-GGUF/resolve/main/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf?download=true",
                 327811552L,
                 190033440L
         );

         //MTMD-audio
         addAIModel(KANTVAIModel.AIModelType.TYPE_LLM, "Qwen2.5-Omni-3B",
                 "Qwen2.5-Omni-3B-Q4_K_M.gguf",
                 "mmproj-Qwen2.5-Omni-3B-Q8_0.gguf",
                 hf_endpoint + "ggml-org/Qwen2.5-Omni-3B-GGUF/resolve/main/Qwen2.5-Omni-3B-Q4_K_M.gguf?download=true",
                 hf_endpoint + "ggml-org/Qwen2.5-Omni-3B-GGUF/resolve/main/mmproj-Qwen2.5-Omni-3B-Q8_0.gguf?download=true",
                 2104931648L,
                 1538031328L
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
     }
 }
