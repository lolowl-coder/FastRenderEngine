# FRE - Fast Render Engine

Graphics engine with emphasis on simplicity and optimum resource consumption.

**Values:**
- Fast and Simple;
- Recent techniques research.

**System requirements:**
- Windows platform
- RTX GPU

**How to build:**

Prerequisites:
- CMake;
- Vulkan SDK;
- Visual Studio;
- CUDA toolkit;
- OptiX.

Building engine and samples:
- Use build*.bat files. E. g. under Win32 run buildDebugWin32.bat

**How to run samples:**

After successful build you can run App executable.
E. g. in Visual Studio set App as startup project and run it.

**Performance analysis:**

NVIDIA NSight Systems - overall performace.
To enable STAT_CPU, STAT_GPU time ranges in NSight Systems:
- define COLLECT_STAT in Samples/AppLib/Includes/Stat.hpp;
- enable NVTX traces collection in NSight Systems;
- run App from under NSight Systems;
- look at "NVTX" markes;
NVIDIA NSight Compute - use for kernel analysis.

**Already implemented:**

Ray tracing using Vulkan KHR extension:
- GI (Global Illumination);
- Area lights;
- Soft shadows;
- HDR rendering;
- Monte Carlo integration;
- Integrated OptiX AI denoiser for real-time denoising of ray-traced output;
- Progressive ray tracing accumulation;
- PBR workflow with physically-based materials and multiple texture maps;
- MSAA.

Core:
- Vulkan renderer;
- GLSL Shaders support;
- Model loading (formats are derived from ASSIMP library);
- PBR Meterials. Textures supported: base color, normal, metalness, roughness maps (formats are derived from stb_image);
- Camera movement;
- Conservative rendering.

**New features planed:**

Ray tracing:
- Improve antialiasing performance;
- Implement samples reuse;
- Hair rendering;
- Animation;
- Transparency.
