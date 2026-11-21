# NDI video prototype

The offscreen capture path is implemented and tested on this Mac. The network backend is optional and uses the official SDK headers plus a dynamically loaded NDI runtime. The SDK/runtime are not installed here; the network branch has **not** been compiled against the official headers or tested with an NDI receiver. Capture diagnostics never advertise or transmit an NDI source.

## What works now

- Shared live slide rendering through `QQuickRenderControl`, with no visible/native capture window or assigned monitor.
- Metal GPU composition and CPU readback at 1280×720 or 1920×1080. The UI accepts 30 or 60 fps; only 30 fps has been checked.
- Full slides, including decoded video, or transparent-canvas overlays preserving all authored text, images, video, and graphics.
- Explicit conversion from premultiplied GPU pixels to straight RGBA for NDI. Channel order, vertical orientation, transparent corners, antialiased text, clear text, and transparent blackout have automated pixel checks.
- Grayscale text antialiasing to prevent LCD color fringes in transparent broadcast text. The setting applies consistently across the application.
- A dedicated sender thread with one pending frame and one frame in flight. Superseded pending frames are dropped instead of accumulating latency or memory.
- Separate capture timing, late-capture estimates, sender queue drops, and receiver counts. In diagnostic mode, processed frames are local only.
- Video-only SDK integration behind a build-time SDK check, using dynamic symbol resolution and synchronous submission on the sender thread. No proprietary headers or libraries are vendored.

The first implementation uses CPU readback, and synchronizes/renders on the GUI thread. It is a technical prototype, not a production throughput guarantee. Zero-copy GPU submission, moving rendering off the GUI thread, NDI audio, long-duration soak tests, receiver compatibility, and network recovery remain work to do.

## Try capture without NDI installed

1. Take a slide live with **Go live**.
2. Open **Outputs → NDI network output → Configure prototype**.
3. Choose Full slide or Transparent canvas, resolution, and frame rate.
4. Choose **Test capture**. The status explicitly says no network transmission.
5. Stop when finished. Closing the panel leaves capture running; closing the app stops it.

The hardware rendering path uses Qt's `GuiPrivate` QRhi API. It must be rebuilt and packaged with the **same Qt build** used to compile it. The tested development installation is Qt 6.12.0; private QRhi APIs have no cross-version binary compatibility guarantee. Software-only Qt Quick rendering is not supported by this capture prototype.

Transparent canvas removes the slide’s base color and ambient background. Authored layers keep their position, stacking, opacity, and intrinsic transparency. An opaque full-canvas image or video covers the canvas; use transparent artwork or smaller media layers for lower thirds. Clear text hides only text, while Blackout clears the entire transparent output.

## Enable actual NDI sending

Obtain the official SDK/runtime from [NDI's developer download](https://ndi.video/for-developers/ndi-sdk/download/). That download currently asks for personal/company details. Review the vendor's terms for your intended use; this project does not supply or accept them on your behalf.

Point CMake at the SDK root containing `include/Processing.NDI.Lib.h`, for example:

```sh
cmake -S . -B build -DNDI_SDK_DIR="/Library/NDI SDK for Apple"
cmake --build build
```

If the SDK uses another structure, set `NDI_INCLUDE_DIR` to the directory that directly contains `Processing.NDI.Lib.h`.

The runtime is loaded on the sending thread. On macOS the usual runtime path is `/usr/local/lib/libndi.dylib`, and SDK library paths are also searched. On Windows the implementation checks `NDI_RUNTIME_DIR_V6` and `NDI_RUNTIME_DIR_V5` for `Processing.NDI.Lib.x64.dll`. You can also choose the exact file with **Locate runtime**, or set `NDI_RUNTIME_PATH` to its full path.

Restart the rebuilt app, take content live, and select **Start NDI**. The runtime/SDK must match the process architecture. The network name is UTF-8, sanitized, and bounded. NDI audio is intentionally absent in this video prototype; the UI labels this limitation.

## Repeatable checks

```sh
./scripts/check-ndi.sh
```

The script uses FFmpeg only to generate the video fixture. Capture itself uses Qt Multimedia and Qt Quick. The application uses an isolated verification profile. It writes:

- `captures/ndi-prototype-report.json`: measured capture-only results, detected displays, and explicit SDK/network/hot-plug validation flags.
- `captures/ndi-full-720.png`, `ndi-full-1080.png`: actual offscreen frames.
- `captures/ndi-transparent-text.png`: straight-alpha text check.
- `captures/ndi-transparent-canvas.png`: transparent canvas with text, an alpha graphic, and video.
- `captures/ndi-settings.png`: prototype controls.

The queue test deliberately stalls the consumer and checks bounded storage, frame dropping, and shutdown. It does not simulate an NDI protocol or claim SDK interoperability.

For real display routing (temporarily shows test output on a non-operator display):

```sh
build/Selah.app/Contents/MacOS/Selah --fresh --physical-smoke "$PWD/captures"
```

This checks audience and stage fullscreen **sequentially** on the second display, audience blackout, operator screen retention, and concurrent offscreen capture. Results go to `captures/physical-display-report.json`. It skips if no non-operator screen is available. It does not physically disconnect a cable or test three simultaneous screens.

## Still required before declaring NDI ready

- Build the SDK-enabled branch against the official headers.
- Receive the real source in an NDI receiver and verify full-color video, transparency over a moving background, source naming, frame pacing, reconnects, and shutdown.
- Test 1080p30 with audience + stage outputs under realistic sustained load. Measure CPU/GPU, memory, capture cadence, and end-to-end latency.
- Physically disconnect/reconnect each output and repeat on Windows.
- Integrate and validate audio synchronization before enabling audio in NDI controls.

API references: [Qt render control](https://doc.qt.io/qt-6/qquickrendercontrol.html), [Qt QRhi compatibility](https://doc.qt.io/qt-6/qrhi.html), [NDI frame formats](https://docs.ndi.video/all/developing-with-ndi/sdk/frame-types), [NDI sending](https://docs.ndi.video/all/developing-with-ndi/sdk/ndi-send), [NDI distribution/licensing](https://docs.ndi.video/all/developing-with-ndi/sdk/licensing).
