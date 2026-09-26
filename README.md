# AI Depth Pro

OpenFX depth effects for DaVinci Resolve, using Depth Anything V2 Small through ONNX Runtime. The plugin includes depth preview, depth of field, parallax, zoom, fog, relighting, masks, edge refinement and temporal smoothing. CPU inference works; Auto tries DirectML on Windows, then CPU. CUDA, TensorRT and Metal are not implemented.

## Install on Windows

1. Build the bundle using the steps below, or obtain a release bundle that contains `Contents/Win64/AI-Depth-Pro.ofx`, the ONNX Runtime DLLs, and `Contents/models/depth_anything_v2_vits.onnx`.
2. Copy the **entire** `AI-Depth-Pro.ofx.bundle` folder to `C:\Program Files\Common Files\OFX\Plugins\`. Administrator permission may be required.
3. Restart DaVinci Resolve. Add **AI Depth Pro** to a clip from the OpenFX effects list. If the plugin reports that the model cannot load, check that the model and runtime files are present in the bundle.

The plugin has been built and exercised with a mock OpenFX host on Windows. In-Resolve installation and visual output still need real host testing. macOS and Linux builds are unverified.

## Smooth playback: render once, then reuse

The first processing pass calculates a depth map for each frame. AI Depth Pro saves those maps in `%LOCALAPPDATA%\AI-Depth-Pro\DepthCache`; later passes reuse them, even after Resolve restarts. A 1080p map takes roughly 4 MB, so a long timeline can consume many gigabytes. Set `AI_DEPTH_PRO_CACHE_DIR` to move the cache, or use **Clear Depth Cache (RAM + Disk)** in the plugin to delete it. Changes to source pixels, dimensions, quality, engine or model invalidate the corresponding depth map.

For **smooth playback of the finished effect**, use Resolve's render cache too:

1. Open the **Edit** page and the timeline containing the clip. If `Playback > Render Cache` is grey, check that you are on the Edit page.
2. Choose **Playback > Render Cache > User**.
3. Right-click the clip on the timeline and enable **Render Cache OFX Filter > AI Depth Pro**. If the effect is on a Color page node, use that node's **Node Cache > On** instead.
4. Stop playback and wait for Resolve's cache indicator to finish before playing again.

The plugin cache skips repeated AI inference; Resolve's render cache stores the final processed frames. Editing effects or source footage may cause Resolve to rebuild its cache. OpenFX does not provide this plugin with a way to pre-render an entire timeline on demand.

## Build from source

Requirements: Windows x64, Visual Studio 2022 C++ tools, CMake 3.16+, compatible ONNX Runtime Windows x64 binaries, and a compatible Depth Anything V2 Small ONNX model. The model and runtime DLLs are **not included in this source repository**. Obtain them from their upstream distributors and review their licenses before redistributing. See [third-party notices](THIRD_PARTY_NOTICES.md).

Place `depth_anything_v2_vits.onnx` in `models/`, and `onnxruntime.dll` (plus provider DLLs used by your runtime) in `deps/onnxruntime/`. Then run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_INFERENCE_TESTS=ON
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

The bundle will be at `build/AI-Depth-Pro.ofx.bundle`. If you do not have the model yet, configure with `-DBUILD_INFERENCE_TESTS=OFF`; tests that require the model will be skipped. The plugin itself still requires a model and ONNX Runtime to produce depth maps.

## Technical notes

Raw inference is cached before filtering, so changes to depth and effect settings can reuse the model result. An in-memory LRU cache handles recent frames; a persistent 16-bit cache handles longer clips. Input pixels are fingerprinted to avoid returning a stale depth map when source content changes. Cache writes include a checksum and incomplete files are rejected.

At 1920×1080 float RGBA, guided filtering fell from 111.4 to 79.4 ms and motion estimation from 37.4 to 27.8 ms in a local benchmark (five-iteration median, one warmup). Timing depends on hardware and load. Regression, persistent-cache, mock OpenFX host, and real ONNX inference tests passed locally. See `docs/` for benchmark output.

## License

The original plugin code in this repository is available under the [MIT License](LICENSE). Third-party headers, the model, and runtime binaries have separate licenses; see [third-party notices](THIRD_PARTY_NOTICES.md). Contributions and Resolve host testing are welcome.
