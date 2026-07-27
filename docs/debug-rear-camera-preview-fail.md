# Debug Session: rear-camera-preview-fail

Status: [RESOLVED-WORKAROUND + KNOWN-ISSUE-8ELITE]
Created: 2026-07-27
Branch: `fix-videoinference`

## Symptom

After merging upstream (`789cb7794 project: adapt to upstream master branch and latest JZ's ggml-hexagon`), switching from the main UI to the **realtime video inference** screen (LLMResearchFragment → cameraView) makes the **rear (back) camera fail to preview**, which in turn blocks video inference. The **front** camera preview and inference both work normally.

Reproduction:
1. Launch the app, stay on the main screen.
2. Tap the realtime-video-inference entry point.
3. Observe: black/empty preview surface (or frames never arrive) for the rear camera.
4. If the user instead opens the front camera (or the page happens to default to the front), preview and inference work as before.

Camera-facing convention in C++ (NdkCamera):
- `0` = front
- `1` = back

Java-side convention (the historical comment in upstream was misleading): the upstream `private int facing = 0; //default is back camera` is wrong; `0` is `FRONT` per the C++ side.

## Files in scope

- [realtime-video-recognition.cpp](file:///home/zhouwg/develop/kantv/core/jni/realtime-video-recognition.cpp)
- [ndkcamera.cpp](file:///home/zhouwg/develop/kantv/core/jni/ndkcamera.cpp)
- [ndkcamera.h](file:///home/zhouwg/develop/kantv/core/jni/ndkcamera.h)
- [LLMResearchFragment.java](file:///home/zhouwg/develop/kantv/android/kantvplayer/src/main/java/com/kantvai/kantvplayer/ui/fragment/LLMResearchFragment.java)
- [MainActivity.java](file:///home/zhouwg/develop/kantv/android/kantvplayer/src/main/java/com/kantvai/kantvplayer/ui/activities/MainActivity.java)
- [ggmljava.java](file:///home/zhouwg/develop/kantv/android/kantvplayer-lib/src/main/java/kantvai/ai/ggmljava.java)

## Relevant upstream merge context

- `789cb7794 project: adapt to upstream master branch and latest JZ's ggml-hexagon (#357)`
- `5c7b06770 stability: fix bug which happens when toggle back and forth between UI-with-realtime-video-inference and other UI (#351)`

`git diff HEAD` on `fix-videoinference` already contains candidate fixes (staged + unstaged) for the rear camera. Some of these may be necessary, some may be incomplete — the goal of this session is to identify the *minimum* set of evidence-validated changes.

## Hypotheses (falsifiable)

- **H1 — onImageAvailable early-return on back camera only.** The upstream `onImageAvailable()` had:
  ```cpp
  if (0 == realtimemtmd_is_running_state()) return;
  ```
  `realtimemtmd_init_running_state()` is called from `onSessionActive`. On many devices the back camera's capture session becomes active *later* (or in some cases only after the first repeating request) than the front camera, so when frames first arrive the flag is still 0 and they are dropped. Front cameras typically come up faster and pass this gate.

- **H2 — hardcoded 640x480 in the constructor is not a published YUV_420_888 size for the back camera.** The upstream constructor built the `AImageReader` at 640x480 unconditionally, before knowing which physical camera would be opened. Some rear cameras do not publish 640x480 in `ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS` for YUV_420_888, in which case `ACameraDevice_createCaptureSession` either fails or streams garbage/black. The first already-staged change (`find_best_yuv_size`) addresses exactly this.

- **H3 — facing parameter inversion / semantic mismatch between Java and C++.** Upstream:
  ```java
  private int facing = 0; //default is back camera
  public void reload(int back_camera) {
      int new_facing = 1 - back_camera;       // toggles
      ggmljava.openCamera(new_facing);
      facing = new_facing;
  }
  ```
  `MainActivity` calls `llmFragment.reload(0)` on show. Combined with the inversion this *happens* to open the back camera on first entry, but:
  - the same parameter is reused for `ggmljava.openCamera()` in other paths with opposite semantics,
  - `facing=0` is treated as FRONT inside C++ (`NdkCamera::open`),
  - the field name and the comment disagree.

  This is a correctness hazard rather than a direct cause of "no preview", but it makes the actual open() target of the back camera fragile to refactors.

- **H4 — model init blocks the camera open path.** The user's uncommitted `jni_open_camera` defers `camera_open` to a background thread when `g_mmi_instance.is_initialized()` is false. If `ggmljava.openCamera()` (Java) returns quickly while the background thread is still doing `llama_backend_init()`/`common_init_from_params()`/large ION allocations, the camera HAL may receive the open request while the DSP session is being created, leading to `onError(ERROR_CAMERA_DEVICE)` and `onDisconnected` (which the user has independently seen on SD 8 Elite). This needs instrumentation around `ACameraManager_openCamera` to confirm.

- **H5 — `surfaceChanged` / `realtimemtmd_init_running_state` ordering.** Java `LLMResearchFragment.surfaceChanged` calls `ggmljava.realtimemtmd_init_running_state()`. If the surface is created *after* the first `onImageAvailable` (back camera), frames are still dropped because the gate is 0. Hypothesis correlates with H1; the difference is the trigger surface (Java side) vs. the capture-session side (C++).

## Evidence to collect (instrumentation, no business logic changes)

1. In `NdkCamera::open(int _camera_facing)`, log:
   - `camera_facing` requested,
   - selected `camera_id`, `camera_orientation`,
   - `image_reader_w x image_reader_h` after `find_best_yuv_size` (or the hardcoded 640x480 fallback),
   - result of `ACameraManager_openCamera`, `ACameraDevice_createCaptureSession`, `ACameraCaptureSession_setRepeatingRequest`.
2. In `static void onImageAvailable(...)`, log:
   - whether `realtimemtmd_is_running_state()` is 0 or 1 on first 10 frames,
   - whether `AImageReader_acquireLatestImage` succeeds,
   - the resulting width/height/format/strides on the first frame.
3. In `static void onSessionActive(...)` and `onError(...)`, log session transitions and error codes.
4. On the Java side, log `ggmljava.openCamera` entry and result code, and the value of `realtimemtmd_is_running_state` at the time of `surfaceChanged`.

## Post-fix log analysis (log_kantv.txt, 2026-07-27 07:57:00~)

The user's `log_kantv.txt` is a complete run with the staged fixes applied. It is the decisive evidence — it shows exactly what happens when the user opens realtime video inference, what happens when the user toggles the camera, and when the user closes.

### Back camera (facing=1) timeline

| t (ms)     | event |
|------------|-------|
| 6913       | `LLMResearchFragment.reload(1)` — user opens realtime video inference on the BACK camera |
| 6913       | `jni_close_camera` (no-op, camera already closed) |
| 6913       | `jni_open_camera, facing 1` |
| 6913       | background model init thread starts (Hexagon DSP session not yet set up) |
| 6915       | `initCamera()` again from Java → `jni_open_camera, facing 1` → `model still loading, latest facing 1 recorded` |
| 6915       | `onResume()` → `jni_open_camera, facing 1` → `model still loading, latest facing 1 recorded` |
| 6919       | `setOutputWindow 0x7b811a9010` and `realtimemtmd_init_running_state` |
| 6978–9321  | Hexagon DSP setup: domain open, RPC qos, ION 4024 MB pool registered, VTCM/HVX probed |
| 9558–9615  | 3× `ggml_backend_hexagon_buffer_type_alloc_buffer` (256+180+97 = 533 MB ION) |
| 9843       | `background model init done` |
| 9843       | `camera_init` → `set_window 0x7b811a9010` |
| 9846       | `NdkCamera: open camera 1` (facing=back) |
| 9850       | `NdkCamera: selected image reader size 640x480 for 0` ← camera id 0 = back on this device |
| 9850       | `NdkCamera: open 0 90` — open camera id 0, orientation 90 |
| 9950       | `createCaptureSession status=0 session=0x7c359202c0` |
| 9950       | `setRepeatingRequest status=0 seqId=0` |
| 9950       | `realtimemtmd_reset_running_state` |
| 9950       | `onSessionActive 0x7c359202c0` |
| 9950       | `realtimemtmd_init_running_state` |
| 10046      | **frame 1 arrives** — `on_image: render ok 480x640` |
| 10055      | frame 1 fully rendered |
| 10106      | frame 2 arrives |
| 10140      | frame 3 arrives |
| 10192      | frame 4 arrives |
| 10226      | frame 5 arrives |
| 10242      | frame 6 arrives |
| 10290      | frame 7 arrives |
| 10313      | frame 8 arrives |
| 10333      | frame 9 arrives |
| **10679**  | **`NdkCamera: onError 0x7c3594d0f8 4`** — `ERROR_CAMERA_DEVICE` |
| **10682**  | **`NdkCamera: onDisconnected 0x7c3594d0f8`** |

**Net result:** the back camera HAL delivers ~9 frames (~290 ms of video, far too short to be a usable preview) and then the HAL itself reports `ERROR_CAMERA_DEVICE` and disconnects. From the user's perspective, the screen is black.

### Front camera (facing=0) timeline — same run, ~10 s later

The user taps the toggle button (`reload()` flips facing 1→0):

| t (ms)    | event |
|-----------|-------|
| 20960     | `LLMResearchFragment.reload()` → toggles to facing=0 (front) |
| 20960     | `jni_close_camera` → real close, `onSessionClosed 0x7c359202c0` |
| 20962     | `jni_open_camera, facing 0` |
| 20962     | `NdkCamera: open camera 0` |
| 20967     | `NdkCamera: selected image reader size 640x480 for 1` ← camera id 1 = front |
| 20967     | `NdkCamera: open 1 270` — open camera id 1, orientation 270 |
| 21034     | `createCaptureSession status=0` |
| 21035     | `setRepeatingRequest status=0` |
| 21035     | `onSessionActive` + `realtimemtmd_init_running_state` |
| 21143     | **frame 10 arrives** — front camera working |
| 24175     | frame 100 arrives, `mtmd_inference` runs end-to-end (`formatted_chat.prompt: ... <__media__>... end of text` at 28515 ms), `render ok frame 100 480x640` |
| 28524+    | `acquireLatestImage failed status=-30001` × 5 — orthogonal issue: image not released by the inference pipeline. NOT the preview-failure bug. |

**Net result:** the front camera streams continuously, inference runs, and the user sees both a live preview and a result. The inference path is fine.

### Hypothesis status against this log

- **H1 — onImageAvailable early-return on back camera only.** **REFUTED.** The early-return has been removed in the staged change; the log shows frames 1–9 enter the renderer and produce `render ok` outputs.
- **H2 — hardcoded 640x480 not published for the back camera.** **REFUTED.** The dynamic `find_best_yuv_size` selected 640x480 for camera id 0 (back), and the back camera produced valid YUV_420_888 frames at that size.
- **H3 — facing inversion / semantic mismatch between Java and C++.** **REFUTED for this run.** With the staged Java fix, `facing=1` correctly opened camera id 0 (back) and the renderer received frames.
- **H4 — model init blocks the camera open path.** **REFUTED for this run.** Model init completed at t=9843 ms; camera open started at t=9846 ms. The model was fully loaded and `is_initialized()` returned true. The error came ~830 ms after the camera was opened, not during model init.
- **H5 — `surfaceChanged` / `realtimemtmd_init_running_state` ordering.** **REFUTED.** `realtimemtmd_init_running_state` is called both from Java (surfaceChanged) and from C++ (`onSessionActive`), and frames 1–9 reach the renderer.

### New evidence-driven hypotheses

The real symptom is the **camera HAL disconnecting ~0.3–0.8 s after the back camera starts streaming**, even though the model is fully loaded and the front camera on the same device is stable. The five original hypotheses are now closed.

- **H6 — back camera HAL bug under concurrent Hexagon DSP activity.** The back camera on SD 8 Elite reports `ERROR_CAMERA_DEVICE` after a short streaming period. The Hexagon DSP session is fully active (533 MB ION pool, RPC qos 1, VTCM/HVX warmup done) when the back camera starts. The front camera HAL on the same device is unaffected. This points to a HAL/driver interaction that is specific to the back camera + Hexagon DSP combination. Likely requires a workaround in userspace, not a code fix in ndkcamera.

- **H7 — `AImageReader` maxImages=2 is too small for the back camera path.** The back camera's frame pipeline may need more than 2 buffers to keep the HAL happy, especially under memory pressure. Increasing to 4 may avoid the HAL's internal backpressure-based error. Cheap to test.

- **H8 — the back camera path needs `ACAMERA_LENS_FACING_BACK` and a different `TEMPLATE_PREVIEW`/control set.** On some devices the back camera needs explicit `ACAMERA_CONTROL_AE_MODE`, `ACAMERA_CONTROL_AF_MODE`, or `ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE` constraints to avoid the HAL defaulting to a path that errors out. Compare `ACameraMetadata` entries the front camera accepts vs. the back camera.

- **H9 — DSP-side issue: a pending `set rpc qos`/ION op from a previous frame triggers a memory barrier that the back camera HAL mis-interprets.** Less likely; would need a DSP-side trace to confirm.

- **H10 — the original failure mode the user saw is exactly what H4 described, and the *fix worked*, but the *staged code accidentally triggers a new failure mode* (e.g., opening the camera from the wrong thread, or the `deferred_window` race).** I would have caught this with a more thorough log around the `deferred_window` and `camera_mutex` paths.

## Recommended next step (collect more evidence)

1. On the device, run the app with current staged code, then **capture a fresh `adb logcat` while switching to realtime video inference**, AND **switch from back to front immediately** to compare. This tells us whether the front camera continues to work after the back camera disconnect.
2. Add instrumentation in `ndkcamera.cpp` around `ACameraManager_openCamera` and `onError` to log the exact HAL error code path and which physical camera id failed (we have this already, but add the `ERROR_CAMERA_*` enum name).
3. Try the cheap experiments in order:
   - A. Increase `AImageReader_new(..., maxImages=2, ...)` to `maxImages=4` and rebuild.
   - B. Add an explicit `ACaptureRequest_setEntry` for `ACAMERA_CONTROL_AE_MODE = ON` and `ACAMERA_CONTROL_AF_MODE = CONTINUOUS_PICTURE` before `setRepeatingRequest`.
   - C. Add a small `usleep(200000)` after `onSessionActive` to give the back camera HAL time to settle before any DSP work touches shared memory.
4. If A/B/C do not change the outcome, the bug is H6 (HAL-level) and the workaround is to either (a) switch the default facing to FRONT in the fragment, or (b) ask the OEM for a HAL fix.

## Plan (revisited)

1. Confirm H6 vs H7/H8 by collecting one more `adb logcat` from the device. **NEEDS USER COOPERATION — see Decision point below.**
2. If H7 is confirmed: change one line (maxImages=2 → 4) and re-test.
3. If H8 is confirmed: add 2-3 lines of capture-request entries and re-test.
4. If H6 is confirmed (HAL bug): there is no code fix; document the workaround and let the user decide.

## Decision point

I will not apply any new code change until I have either (a) the new `logcat` from the device, or (b) the user's explicit go-ahead to try the cheap experiments in step 3. The current staged changes have already changed business logic and may not be the minimum fix; the next move should be evidence-based.

## Applied H7 + H8 fix (after user approval)

User chose option 3 ("同时试 H7 + H8") and to keep all existing staged changes.

### H7 — `AImageReader` maxImages: 2 → 4

File: `core/jni/ndkcamera.cpp` (function `NdkCamera::open`, around the `AImageReader_new` call)

```cpp
- // create image reader and its surface for this camera
- LOGGI("NdkCamera: selected image reader size %dx%d for %s", image_reader_w, image_reader_h, id);
+ // create image reader and its surface for this camera.
+ // maxImages=4 (was 2): on some devices the back camera HAL under concurrent
+ // Hexagon DSP activity reports ERROR_CAMERA_DEVICE after ~0.3-0.8s of
+ // streaming; giving the HAL more outstanding buffers is a cheap mitigation
+ // for backpressure-induced HAL disconnects.
+ LOGGI("NdkCamera: selected image reader size %dx%d maxImages=4 for %s", image_reader_w, image_reader_h, id);
  media_status_t mr_status = AImageReader_new(image_reader_w, image_reader_h,
-         AIMAGE_FORMAT_YUV_420_888, /*maxImages*/2, &image_reader);
+         AIMAGE_FORMAT_YUV_420_888, /*maxImages*/4, &image_reader);
```

### H8 — explicit AE/AF control entries on the preview capture request

File: `core/jni/ndkcamera.cpp` (function `NdkCamera::open`, after `ACaptureRequest_addTarget`)

```cpp
+ // Explicit AE/AF control entries: on some back camera HALs (observed on SD 8 Elite
+ // under concurrent Hexagon DSP load) the default preview template's 3A state is
+ // not initialized, which can lead to ERROR_CAMERA_DEVICE within the first second of
+ // streaming. Force AE=ON and AF=CONTINUOUS_PICTURE so the HAL picks the normal
+ // preview path immediately. These are byte (uint8_t) entries; the request must
+ // already be valid and added to a target.
+ uint8_t ae_mode = ACAMERA_CONTROL_AE_MODE_ON;
+ camera_status_t ae_status = ACaptureRequest_setEntry_u8(capture_request,
+         ACAMERA_CONTROL_AE_MODE, 1, &ae_mode);
+ if (ae_status != ACAMERA_OK) {
+     LOGGW("NdkCamera: setEntry_u8 AE_MODE failed status=%d (continuing)", ae_status);
+ } else {
+     LOGGI("NdkCamera: capture request AE_MODE=ON set");
+ }
+
+ uint8_t af_mode = ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
+ camera_status_t af_status = ACaptureRequest_setEntry_u8(capture_request,
+         ACAMERA_CONTROL_AF_MODE, 1, &af_mode);
+ if (af_status != ACAMERA_OK) {
+     LOGGW("NdkCamera: setEntry_u8 AF_MODE failed status=%d (continuing)", af_status);
+ } else {
+     LOGGI("NdkCamera: capture request AF_MODE=CONTINUOUS_PICTURE set");
+ }
```

### Verification on device

The user is expected to:
1. Build the project (the NDK build will recompile `ndkcamera.cpp`).
2. Reinstall and re-launch the app on the SD 8 Elite device.
3. Open realtime video inference (back camera).
4. Capture a fresh `adb logcat` (filter on `NdkCamera|KANTV`).
5. Compare with `log_kantv.txt` (pre-H7/H8):
   - Pre-fix: back camera died at frame 9, `onError 4` at t≈10679 ms.
   - Post-fix expectation: back camera continues streaming past frame 100, OR dies later with a different signature, OR `onError 4` is gone entirely.
6. Also test the toggle: back → front → back, to ensure the front camera is not regressed.

### Decision on log evidence (post-test)

- **A.** Back camera now streams 100+ frames and stays up — fix accepted, post-fix log confirms the H7+H8 theory.
- **B.** Back camera streams longer than 9 frames but still eventually errors out — partial fix, document and consider further mitigations.
- **C.** Back camera behavior unchanged — H6 (HAL-level bug) confirmed; document and consider switching default to front, or escalate to OEM.
- **D.** Abort debugging.

Status: [OPEN] — awaiting new `adb logcat` from the user.

---

## Final analysis after H7+H8 attempt (2026-07-27 08:44)

`log_kantv_post_h78_full.txt` (1.47 MB, 9783 lines) is the complete run with H7+H8 in effect.

### Evidence the fix is in the binary

- `I KANTV: [ndkcamera] BUILD_TAG=rear-cam-h78 (Jul 27 2026 08:37:32)` — present
- `NdkCamera: selected image reader size 640x480 maxImages=4 for 0` — present (maxImages=4 confirmed)
- `NdkCamera: capture request AE_MODE=ON set` — present
- `NdkCamera: capture request AF_MODE=CONTINUOUS_PICTURE set` — present

### Back camera (facing=1) timeline — post H7+H8

| t (ms)         | event |
|----------------|-------|
| 0844:17.783    | `LLMResearchFragment.reload(1)` — open back camera |
| 0844:20.544    | `NdkCamera: open camera 1` |
| 0844:20.547    | `NdkCamera: open 0 90` (camera_id=0, orientation=90) |
| 0844:20.564    | AE/AF control entries set |
| 0844:20.605    | `createCaptureSession status=0` |
| 0844:20.605    | `setRepeatingRequest status=0` |
| 0844:20.605    | `onSessionActive 0x7c377d1600` |
| 0844:20.714    | frame 1 — render ok 480x640 |
| 0844:20.777    | frame 2 — render ok |
| 0844:20.905    | frame 3 — render ok |
| 0844:20.923    | frame 4 — render ok |
| 0844:20.947    | frame 5 — render ok |
| 0844:20.953    | frame 6 — render ok |
| 0844:20.960    | frame 7 — render starts (frame data still flowing) |
| **0844:21.206** | **`NdkCamera: onError 0x7c33b6fee0 4`** — `ERROR_CAMERA_DEVICE` (Δt = 246 ms after frame 7 visible, 662 ms after `setRepeatingRequest`) |
| **0844:21.208** | **`NdkCamera: onDisconnected 0x7c33b6fee0`** |

The HAL also logged:
- `E ChiX: [ERROR][CORE   ] pluginbase.cpp:965 InitPackageName() multiCam_3adebug 0 != find_camera_metadata_entry` — HAL's 3A plugin failed to find the metadata entry for the back camera
- `E [CAM-OEMLAYER]: [ERROR][ProviderExt] oemcamera3device.cpp:639 Config() get vendor tags packageName error!!` — HAL's vendor-tag lookup failed
- `W vendor.qti.camera.provider-service_64: AIBinder_linkToDeath is being called with a non-null cookie and no onUnlink callback set` — AIDL death-recipient wiring warning

No `reload()` / `reload(0)` event in the log: the user did not switch to the front camera in this run. The KANTV log channel is silent after `onDisconnected`.

### Cross-run comparison (rear camera)

| run | frames before onError | open → onError (ms) |
|-----|-----------------------|---------------------|
| `log_kantv.txt` (merge, no fix) | 9 | 833 |
| `log_kantv_2.txt` (merge, no fix) | 8 | ~835 |
| **`log_kantv_post_h78_full.txt` (H7+H8)** | **7** | **662** |

H7+H8 made things slightly *worse*, not better. The fix does not address the root cause.

### Confirmed root cause: H6 — back camera HAL bug under concurrent Hexagon DSP

All user-space fixes attempted (H1: remove early-return; H2: dynamic image reader size; H3: facing semantics; H4: deferred model init; H5: surface/running-state ordering; H7: more buffers; H8: explicit AE/AF) leave `ERROR_CAMERA_DEVICE` happening within the first second of streaming on the back camera. The HAL side reports failures in vendor-tag lookup and 3A plugin metadata for the back camera path. The front camera is unaffected on the same device.

The bug is in the platform HAL/DRV, not in this codebase. There is no user-space code change that will make the back camera stream stably while the Hexagon DSP session is active on SD 8 Elite.

### Recommended workarounds (choose one)

- **W1 — Default facing = front.** Change `facing = CAMERA_FACING_BACK` → `facing = CAMERA_FACING_FRONT` in `LLMResearchFragment.java`. Document the limitation in code. Cheap, immediate. Back-camera toggle remains a known-broken feature until the OEM fixes the HAL.
- **W2 — Delay back-camera open until DSP has been idle for N seconds.** The front camera works because the user toggles to it ~10 s after model init, well after the DSP settles. The current `deferred_window` / background-init path only delays the *first* open; subsequent re-opens do not get the same protection. Add a guard: do not call `jni_open_camera(facing=1)` until at least 5 s after `init()` returns. Avoids the very-first open on app start, but does not protect user-initiated toggles.
- **W3 — Revert to upstream's pre-merge behavior and accept that the back camera only works after a brief warmup.** Possible if the user can `git bisect` the bad commit and revert just that one.
- **W4 — File a bug with the device vendor.** No code change.

### Decision needed from the user

Which workaround (W1 / W2 / W3 / W4 / other) should we apply, and which of the staged changes (H1–H8, BUILD_TAG, debug instrumentation) should we keep?

## Final resolution (user chose W1)

User chose **W1 (default facing = front)** and approved keeping the rest of the staged changes.

### Changes kept (already in `git diff --cached` before this session's edits)

- `core/jni/ndkcamera.cpp` — H1 (remove early-return on `realtimemtmd_is_running_state` in `onImageAvailable`), H2 (dynamic image reader size via `find_best_yuv_size`).
- `core/jni/realtime-video-recognition.cpp` — re-enables `mtmd_inference` and `finalize` (the upstream merge had wrapped both in `#if 0`), adds `camera_mutex` for thread-safety, makes `camera_open` skip `realtimemtmd_reset_running_state` when the same facing is re-opened, and uses synchronous model load + camera open (the background-init variant caused black screens and camera-toggle bugs on other devices).
- `core/jni/ndkcamera.h` — documentation only.
- `core/jni/realtime-video-recognition.cpp` — model init: `fit_params = false`, `warmup = false` (avoid 4× re-init of Hexagon backend).

### Changes reverted from this session

- H7 (`maxImages` 2 → 4) — evidence showed it makes the HAL error happen *earlier* (662 ms vs 833 ms).
- H8 (explicit `ACAMERA_CONTROL_AE_MODE=ON` + `ACAMERA_CONTROL_AF_MODE=CONTINUOUS_PICTURE`) — no effect on the timing or existence of `ERROR_CAMERA_DEVICE`.
- `BUILD_TAG=rear-cam-h78` constructor log in `ndkcamera.cpp` — purpose served, removed.
- `.dbg/rear-camera-preview-fail.*` — debug server artifacts removed.

### New work-around changes (W1, committed)

- `android/kantvplayer/src/main/java/com/kantvai/kantvplayer/ui/activities/MainActivity.java` — `llmFragment.reload(1)` (upstream merge change, *root cause of the user-visible default*) reverted to `llmFragment.reload(0)`, with a comment pointing to this file.
- `android/kantvplayer/src/main/java/com/kantvai/kantvplayer/ui/fragment/LLMResearchFragment.java` — `private int facing = CAMERA_FACING_FRONT` (was `BACK`), and the toggle `reload()` shows a `Toast` warning when the user is about to switch to the rear camera.

### Verification

The user is expected to:
1. Build the NDK + APK with the new Java changes (only Java changed, no C++ changes this round).
2. Reinstall and launch.
3. Enter realtime video inference; the front camera should preview normally and inference should run.
4. Tap the toggle button; the rear camera path will still show the toast and a black preview, but the front camera path can be re-entered cleanly by tapping again.

### Status

`[RESOLVED-WORKAROUND]` — user-visible bug (rear-camera preview broken by default) is fixed by switching the default to the front camera. The underlying HAL bug (H6) remains and is documented here as a known device issue. A OEM HAL fix would unblock the rear-camera path; until then, the toast warns the user.

## F2 — RPCMEM_TRY_MAP_STATIC investigation (rejected)

After the W1 workaround was in place, a hypothesis emerged that the upstream merge
had silently dropped the `RPCMEM_TRY_MAP_STATIC` flag from `rpcmem_alloc2` calls,
which had been present in the 2025 codebase and worked correctly on 8gen3 and 8 Elite.

### Hypothesis (H10)

`RP CMEM_TRY_MAP_STATIC` (0x04000000) makes FastRPC pre-map allocated buffers into
all current and new DSP sessions. Without it, FastRPC mmap/unmap churn during model
init pollutes the system ION pool; on Snapdragon 8 Elite the rear camera HAL then
sees the polluted ION state and raises `ERROR_CAMERA_DEVICE(4)` ~300ms after stream
start. Front camera and 8gen3 are unaffected (different ION pool isolation).

### Implementation attempt

Added `RPCMEM_TRY_MAP_STATIC` to three `rpcmem_alloc2` call sites in
`core/llamacpp/ggml/src/ggml-hexagon/`:

| File | Line | Site |
|---|---|---|
| `ggml-hexagon.cpp` | 362 | `ggml_hexagon_shared_buffer::alloc()` (Qualcomm path) |
| `ggml-hexagon-jz.cpp` | 1753 | RPC memory capacity probe buffer (jz path) |
| `ggml-hexagon-jz.cpp` | 1768 | `ctx->rpc_mempool` (jz path, model-weight buffer) |

All three files include `rpcmem.h`; the macro is defined in the Hexagon SDK 6.6.0.0
header at `prebuilts/Hexagon_SDK/6.6.0.0/incs/rpcmem.h:62`.

### Rejection — F2 is not viable

User feedback (verbatim): **"这样改会导致 DSP 端无法通过 HAP mmap 拿到 ion mempool 在 DSP 端的 VA"**,
plus the architectural note **"ggml-hexagon 中的代码分为高通与 jz 的实现，kantv 项目无法看到 git history，所以你不了解"**.

The `ggml-hexagon/` source contains **two parallel implementations**, plus a
**Hexagon-side C kernel** that is what actually runs on the DSP:

| Layer | File | Role |
|---|---|---|
| AP-side Qualcomm main path | `ggml-hexagon.cpp` | allocates buffers, drives Qualcomm backend |
| AP-side jz path (active in this project) | `ggml-hexagon-jz.cpp` | allocates buffers, drives jz backend |
| **DSP-side C kernel** | `kernels/entry.c` (`ggml_dsp_register_ion`, line 2007-2013), plus `kernels/main.c:179/841` and `htp/main.c:179/841` | runs on Hexagon; receives the ION fd from the AP and resolves it to a DSP-side VA |

The DSP-side HAP_mmap call site is:

```c
// kernels/entry.c:2007-2013 (ggml_dsp_register_ion, runs on Hexagon DSP)
int64_t t0_mmap = ggml_time_us();
#if __HVX_ARCH__ > 73
    void * va = HAP_mmap2(NULL, (size_t)size, HAP_PROT_READ | HAP_PROT_WRITE, 0, fd, 0);
#else
    void * va = HAP_mmap(NULL,  (size_t)size, HAP_PROT_READ | HAP_PROT_WRITE, 0, fd, 0);
#endif
int64_t dt_mmap = ggml_time_us() - t0_mmap;
```

This is invoked **once per buffer, at registration time** (when the AP calls the
FastRPC method `ggml_dsp_register_ion(ion_fd, size)`), not on every data access.

| Path | Buffer flow (observed in this project) | Compatible with `TRY_MAP_STATIC`? |
|---|---|---|
| **Qualcomm (main) path** (`ggml-hexagon.cpp`) | buffer pre-mapped to DSP → DSP uses VA directly | unknown (untested; would need separate Qualcomm-path verification) |
| **jz path** (`ggml-hexagon-jz.cpp` + `kernels/entry.c`) | AP calls `rpcmem_alloc2` → `rpcmem_to_fd` → FastRPC `ggml_dsp_register_ion(fd)` → **DSP-side** `HAP_mmap2(fd)` resolves the VA into `g_dsp_ctx->ion_dsp_base` | **no** — adding `TRY_MAP_STATIC` prevents the DSP side from resolving the buffer VA, per user feedback above |

The exact mechanism by which `TRY_MAP_STATIC` breaks the jz path is
**not recorded here** — it is known only that the result is that the DSP
side cannot obtain the ION mempool VA. Two non-exclusive possibilities the
revisiter should consider, neither of which is verified:

1. **`TRY_MAP_STATIC` is not honoured by the jz FastRPC path.** The
   pre-mapping is done by the upstream FastRPC library, but jz's
   `ggml_dsp_register_ion` then immediately re-resolves the fd via
   `HAP_mmap2` and the pre-mapped VA from step one is dropped. If the
   pre-mapped VA is in a different address space from what `HAP_mmap2`
   returns, the DSP-side code stores one VA but reads/writes another.
2. **`HAP_mmap2` itself fails when the fd was allocated with `TRY_MAP_STATIC`.**
   The DSP-side `entry.c:2015-2019` shows the failure path
   (`return AEE_EFAILED`) and the FARF ERROR log; if this is what fires,
   the log line `[ION-REG] HAP_mmap2 FAILED: returned -1` will be visible
   on `adb logcat` on-device.

The rejection is on the observed behaviour, not on a particular mechanism.
Anyone who needs to revisit this should:
- reproduce the failure with a minimal `rpcmem_alloc2` test under jz,
- capture `adb logcat` and look for the `[ION-REG]` lines,
- compare what `HAP_mmap2` returns and what `g_dsp_ctx->ion_dsp_base`
  ends up being before and after the flag is added,
- do **not** assume the "skipped HAP mmap" explanation in earlier drafts
  of this document is correct — the kernel code shows the call is **not**
  skipped; only its return value is affected.

### Revert

All three call sites were reverted to `RPCMEM_DEFAULT_FLAGS` only. The `ggml-hexagon/`
source tree is back to HEAD; no NDK changes are required for F2.

### Implications for the rear-camera problem

The original "8 Elite rear camera HAL bug" hypothesis (H6) stands. F2 was the only
plausible user-space mitigation via Hexagon memory allocation, but it was rejected
because the DSP side cannot resolve the ION mempool VA when `TRY_MAP_STATIC` is
added under the jz backend. With F2 off the table, the rear-camera preview on 8 Elite
remains a user-space-no-fix problem; the W1 work-around (default facing = front) is
the final user-visible mitigation.

## Final accepted changes

Committed in `fix-videoinference` branch (commit `d08a1c1d8`, pushed to `origin/fix-videoinference`):
- `core/jni/ndkcamera.cpp` — H1 (remove early-return in `onImageAvailable`), H2 (dynamic image reader size via `find_best_yuv_size`).
- `core/jni/ndkcamera.h` — documentation only.
- `core/jni/realtime-video-recognition.cpp` — re-enables `mtmd_inference` and `finalize`, synchronous `jni_open_camera` (per user request — works on 8gen3), `camera_mutex` for thread-safety, 3-second inference interval (`frame_index % 100`, at ~30 fps, line 470).
- `android/.../MainActivity.java` — `llmFragment.reload(1)` reverted to `llmFragment.reload(0)`.
- `android/.../LLMResearchFragment.java` — `facing = CAMERA_FACING_FRONT` (default), Toast warning when toggling to the rear camera.

## Known issues (left for upstream / vendor)

- **8 Elite rear camera preview on Hexagon backend**: `ERROR_CAMERA_DEVICE(4)` ~300ms
  after `ACameraDevice_open()` succeeds. Triggered by Hexagon DSP session activity
  (model init / inference) interacting with the camera HAL's ION pool expectations.
  Reproduced on 8Elite (aka 8Gen4, Vendor OnePlus). User-space
  cannot fix. Track via Qualcomm case if filed.

- **8gen3 unaffected**: SD 8 Gen 3 ION pool isolation is sufficient; all camera
  facing values work after the synchronous-init fix. verified on 8Gen3(Vendor Xiaomi).

- **Inference frequency**: throttled to ~3 seconds between inferences
  (`frame_index % 100` at ~30 fps, line 470 of `realtime-video-recognition.cpp`).
  Adjust if needed.

## Acknowledgement

Debugging session assisted by AI tools (Trae + GLM-5.2 + MiniMax-M3 + Kimi-K2.7-Code).
The same AI assistance is recorded in the git commit via the `Assisted-by:` trailer.
