
- create **test.m3u**(recommend name and it's hardcoded in source code) like this:

```
  #EXTM3U
  #EXTINF:-1,hls
  http(s)://local_http_server/kantv/media/test.hls
  #EXTINF:-1,dash
  http(s)://local_http_server/kantv/media/test.dash
  #EXTINF:-1,rtmp
  http(s)://local_http_server/kantv/media/test.rtmp
  #EXTINF:-1,webrtc
  http(s)://local_http_server/kantv/media/test.rtc
  #EXTINF:-1,hevc(h265)
  http(s)://local_http_server/kantv/media/test.hevc
  #EXTINF:-1,h266
  http(s)://local_http_server/kantv/media/test.h266
  #EXTINF:-1,av1
  http(s)://local_http_server/kantv/media/test.av1
  #EXTINF:-1,testvideo-1 (pls attention following path is start with /)
  /test.mp4
  #EXTINF:-1,testvideo-2
  /video/test.ts

```

or just fetch favourite playlist from <a href="https://github.com/iptv-org/iptv">IPTV</a> and rename it to test.m3u(pls attention that users/developers from Mainland China should review <a href="https://github.com/cdeos/kantv/issues/27">this issue</a>)

 - upload test.m3u to local http server like this

```
 test.m3u                  ->   http(s)://local_http_server/kantv/epg/test.m3u
```

