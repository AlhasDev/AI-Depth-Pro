# Third-party components

- OpenFX headers under `include/openfx/` come from the Academy Software Foundation OpenFX project, licensed BSD-3-Clause. The license text is in `include/openfx/LICENSE`: <https://github.com/AcademySoftwareFoundation/openfx>.
- `include/onnxruntime_c_api.h` is an ONNX Runtime API header from Microsoft. ONNX Runtime is MIT-licensed. The license text is in `include/ONNX_RUNTIME_LICENSE`: <https://github.com/microsoft/onnxruntime>.
- The Depth Anything V2 Small architecture and official weights are Apache-2.0: <https://github.com/DepthAnything/Depth-Anything-V2>. The setup script downloads a pinned ONNX Community export from <https://huggingface.co/onnx-community/depth-anything-v2-small>. This repository does not include weights or an ONNX export.
- ONNX Runtime 1.24.4 is MIT-licensed and DirectML 1.15.4 is distributed under its NuGet package license. The setup script downloads their pinned official NuGet packages. Runtime binary files are excluded from this repository.

The repository's MIT license applies to original plugin code and documentation, not to these upstream components.
