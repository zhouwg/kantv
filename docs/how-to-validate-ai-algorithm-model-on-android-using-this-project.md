
for GGML inference framework, pls refer to https://github.com/zhouwg/kantv/commit/1bbf5345011ff452796a813f95794a27a97dd23f



for NCNN inference framework, modify following files to add new inference bench type / new inference backend / new realtime inference type

<a href="https://github.com/zhouwg/kantv/blob/master/cdeosplayer/kantv/src/main/res/values/arrays.xml#L77">arrays.xml</a>


<a href="https://github.com/zhouwg/kantv/blob/master/cdeosplayer/cdeosplayer-lib/src/main/java/cdeos/media/player/CDEUtils.java#L4027">CDEUtils.java</a>


<a href="https://github.com/zhouwg/kantv/blob/master/cdeosplayer/kantv/src/main/java/com/cdeos/kantv/ui/fragment/AIResearchFragment.java">AIResearchFragment.java</a>


<a href="https://github.com/zhouwg/kantv/blob/master/cdeosplayer/kantv/src/main/java/com/cdeos/kantv/ui/fragment/AIAgentFragment.java">AIAgentFragment.java</a>


<a href="https://github.com/zhouwg/kantv/blob/master/core/ncnn/jni/ncnn-jni.h">ncnn-jni.h</a>


<a href="https://github.com/zhouwg/kantv/blob/master/core/ncnn/jni/ncnn-jni-impl.cpp">ncnn-jni-impl.cpp</a>
