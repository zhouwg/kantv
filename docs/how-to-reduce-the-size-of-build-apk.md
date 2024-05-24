1. mv assets/models to /sdcard/kantv/ manually, it will reduce the size of buid apk significantly

    ```

zhouwg:$ cd cdeosplayer/kantv/src/main/assets/
zhouwg:$ ls
config.json  jfk.wav  kantv.bin  mnist-4.png  mnist-5.png  mnist-7.png  models  qnnlib  res  test.m3u
zhouwg:$ du -h
212M	./models
3.0M	./res
49M	    ./qnnlib
264M	.
zhouwg:$

    ```

2. build APK from source code by command line(default is Release build)

    ```
    . build/envsetup.sh
    lunch 1
    ./build-all.sh android

    ```
