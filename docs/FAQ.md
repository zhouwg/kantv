How to troubleshooting issues in KanTV APK

1. "ASR not initialized" in realtime subtitle

pls select the default ASR model "tiny.en-q8_0" in ASR Setting and re-launch the APK


2. "ASR not initialized" in the "ggml-hexagon on Android"

this issue caused by Android permission: APK need to access storage to read/load ASR model from path /sdcard/ in Android phone.

pls re-launch the APK accordingly.

3. LLM download issue

as well known, https://huggingface.co/ cann't be directly accessed from China.

issue reports are greatly welcomed. Be sure to review the [opening issues](https://github.com/zhouwg/kantv/issues?q=is%3Aopen+is%3Aissue) before contribute to project KanTV, We use [GitHub issues](https://github.com/zhouwg/kantv/issues) for tracking requests and bugs, please see [how to submit issue in this project ](https://github.com/zhouwg/kantv/issues/1).
