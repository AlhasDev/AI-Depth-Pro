# Third-party components

- OpenFX headers under `include/openfx/` come from the Academy Software Foundation OpenFX project, licensed BSD-3-Clause. The license text is in `include/openfx/LICENSE`: <https://github.com/AcademySoftwareFoundation/openfx>.
- `include/onnxruntime_c_api.h` is an ONNX Runtime API header from Microsoft. ONNX Runtime is MIT-licensed. The license text is in `include/ONNX_RUNTIME_LICENSE`: <https://github.com/microsoft/onnxruntime>.
- The Depth Anything V2 Small architecture and official weights are Apache-2.0: <https://github.com/DepthAnything/Depth-Anything-V2>. This repository does not include weights or an ONNX export.
- ONNX Runtime and DirectML binary files are excluded from this repository. Check the license and version of the exact distribution you choose before packaging a release.

The repository's MIT license applies to original plugin code and documentation, not to these upstream components.
