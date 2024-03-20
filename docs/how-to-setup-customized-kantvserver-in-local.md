
The computing power and network bandwidth of default kantvserver is very low due to insufficient fund, so setup a local End-2-End development env is strongly recommended.

 - setup a http server(by apache or nginx) in local development env

 - modify kant server address in app

![1370107702](https://github.com/zhouwg/kantv/assets/6889919/1e994269-28be-4513-9f74-3973269b8832)

 - upload required files to local http server like this(dependent files for whisper.cpp could be found at GGML's official website)
```
   apk ->                              http(s)://local_http_server/kantv/apk/kantv-latest.apk
   apk version ->                      http(s)://local_http_server/kantv/apk/kantv-version.txt
   jfk.wav     ->                      http(s)://local_http_server/kantv/ggml/jfk.wav
   ggml-tiny-q5_1.bin                  http(s)://local_http_server/kantv/ggml/ggml-tiny-q5_1.bin
   ...
   ...
   ...


```
