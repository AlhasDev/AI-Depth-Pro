Run `scripts/setup-dependencies.ps1` from the repository root to install the pinned Windows x64 runtime files. The script downloads ONNX Runtime DirectML 1.24.4 and DirectML 1.15.4 from their official NuGet packages, verifies the packages and installed DLLs, and skips files that already match.

The runtime DLLs are excluded from this source repository. Review the third-party notices before publishing a binary bundle. See https://onnxruntime.ai/docs/install/.
