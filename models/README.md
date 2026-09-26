# Depth model

Place a compatible Depth Anything V2 Small ONNX file here as `depth_anything_v2_vits.onnx`. The model is required for depth inference and is excluded from this source repository. The bundled model used for local testing had SHA-256 `AFB6A5C28F3B6BF1618C6E43F02073EF9DFDC70E937502D51603E57B0A1DF10C`; other compatible exports may differ.

You can also set `AI_DEPTH_PRO_MODELS_DIR` to a directory containing the model, or supply an explicit model file path in the plugin. There is no automatic downloader or gradient fallback. See the repository README for build and installation instructions.
