
#### Fetch source codes

```
git clone https://github.com/kantv-ai/kantv.git

cd kantv

git checkout master

cd kantv

```

#### Setup dev env according to [how to build](./how-to-build.md)

#### Add a specified LLM to project

add a specified LLM in the source file https://github.com/kantv-ai/kantv/blob/master/android/kantvplayer-lib/src/main/java/kantvai/ai/KANTVAIModelMgr.java#L228

#### Build Android APK

Build APK from source code by command line

```
        . build/envsetup.sh
        lunch 1
        ./build-all.sh android
```

#### Run Android APK on Android phone
