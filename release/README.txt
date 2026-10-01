AI Depth Pro - Windows x64 preview

1. Extract the entire ZIP to a folder.
2. Close DaVinci Resolve.
3. Double-click install.bat and accept the Windows administrator prompt.
4. Start Resolve and add AI Depth Pro from the OpenFX effects list.

This package includes the compiled plugin, model and runtime DLLs.
Visual Studio, CMake and a separate model download are not required.
Install location: C:\Program Files\Common Files\OFX\Plugins\AI-Depth-Pro.ofx.bundle

Requires Windows x64 and the Microsoft Visual C++ 2015-2022 x64 runtime.
If Resolve reports a missing VCRUNTIME or MSVCP DLL, install Microsoft's runtime:
https://aka.ms/vs/17/release/vc_redist.x64.exe

Preview: automated tests pass, but real Resolve host testing is pending.
For smooth playback, enable Resolve's User render cache for the effect.
Project and instructions: https://github.com/AlhasDev/AI-Depth-Pro
Original code: MIT. See licenses/ and THIRD_PARTY_NOTICES.md for bundled components.
