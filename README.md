# AI Depth Pro

[![Windows CI](https://github.com/AlhasDev/AI-Depth-Pro/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/AlhasDev/AI-Depth-Pro/actions/workflows/windows-ci.yml)

OpenFX depth effects for DaVinci Resolve, using Depth Anything V2 Small through ONNX Runtime. The plugin includes depth preview, depth of field, parallax, zoom, fog, relighting, masks, edge refinement and temporal smoothing. CPU inference works; Auto tries DirectML on Windows, then CPU. CUDA, TensorRT and Metal are not implemented.

## Install on Windows

1. Install Visual Studio 2022 with **Desktop development with C++** and CMake.
2. Close DaVinci Resolve, then double-click `install.bat`. It downloads and verifies the dependencies, builds the plugin, runs every test, and requests administrator permission only for the final copy to `C:\Program Files\Common Files\OFX\Plugins\`.
3. Restart DaVinci Resolve and add **AI Depth Pro** to a clip from the OpenFX effects list.

If you only want a local bundle, run `install.bat --build-only`. The result will be in `build\AI-Depth-Pro.ofx.bundle`.

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

Requirements: Windows x64, Visual Studio 2022 C++ tools, CMake 3.16+, PowerShell 5.1+, and internet access for the first dependency setup. The model and runtime DLLs are **not included in this source repository**.

From PowerShell, install the pinned model and Windows x64 runtime, then build and test:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-dependencies.ps1
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_INFERENCE_TESTS=ON
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

The setup downloads roughly 300 MB on a fresh machine, checks cryptographic hashes, and skips files that are already verified. The bundle will be at `build/AI-Depth-Pro.ofx.bundle`. Run `scripts/verify.ps1` to configure, build, and test in one step after dependencies are ready. See [third-party notices](THIRD_PARTY_NOTICES.md) before redistributing the model or runtime binaries.

## Technical notes

Raw inference is cached before filtering, so changes to depth and effect settings can reuse the model result. An in-memory LRU cache handles recent frames; a persistent 16-bit cache handles longer clips. Input pixels are fingerprinted to avoid returning a stale depth map when source content changes. Cache writes include a checksum and incomplete files are rejected.

At 1920×1080 float RGBA, guided filtering fell from 111.4 to 79.4 ms and motion estimation from 37.4 to 27.8 ms in a local benchmark (five-iteration median, one warmup). Timing depends on hardware and load. Regression, persistent-cache, mock OpenFX host, and real ONNX inference tests passed locally. See `docs/` for benchmark output.

## License

The original plugin code in this repository is available under the [MIT License](LICENSE). Third-party headers, the model, and runtime binaries have separate licenses; see [third-party notices](THIRD_PARTY_NOTICES.md). Contributions and Resolve host testing are welcome.
