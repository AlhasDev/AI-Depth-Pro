# Depth model

Run `scripts/setup-dependencies.ps1` from the repository root to download the pinned Depth Anything V2 Small ONNX model as `depth_anything_v2_vits.onnx`. The script verifies SHA-256 `AFB6A5C28F3B6BF1618C6E43F02073EF9DFDC70E937502D51603E57B0A1DF10C` before installing it. The model is required for depth inference and is excluded from this source repository.

You can also set `AI_DEPTH_PRO_MODELS_DIR` to a directory containing a compatible model, or supply an explicit model file path in the plugin. There is no gradient fallback. See the repository README for build and installation instructions.
