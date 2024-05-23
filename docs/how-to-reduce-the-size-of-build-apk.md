1. build apk

    - use Release build in Android Studio

    - build APK from source code by command line(default is Release build)

    ```
        . build/envsetup.sh
        lunch 1
        ./build-all.sh android

    ```

2. mv files in directory assets to /sdcard/kantv/ manually

    ```
zhouwg:$ cd cdeosplayer/kantv/src/main/assets/
zhouwg:$ ls
config.json            jfk.wav    mnist-4.png  mnist-7.png                ncnnres  res
ggml-tiny.en-q8_0.bin  kantv.bin  mnist-5.png  mnist-ggml-model-f32.gguf  qnnlib   test.m3u
zhouwg:$ du -h
3.0M	./res
49M	./qnnlib
170M	./ncnnres
264M	.

    ```
