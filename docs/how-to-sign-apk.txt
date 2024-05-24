1. create keystore using keytool

keytool -genkeypair -v -keystore kantvapk.keystore -alias release-keystore -keyalg RSA -validity 30


2. check keystore

keytool -list -v -keystore kantvapk.keystore


3. sign APK using jarsigner

3.1 generate kantv-all64-release-unsigned.apk

    ```
    . build/envsetup.sh
    lunch 1
    ./build-all.sh android

    ```

3.2 sign APK

    ```
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore kantvapk.keystore kantv-all64-release-unsigned.apk release-keystore -signedjar kantv-1.x.x-signed.apk
    ```
