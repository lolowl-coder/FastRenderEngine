# FRE - Fast render engine

Engine with emphasis on simplicity and optimum resource consumption.

**Values:**
Simple
Fast
Crossplatform

**Buld system:**
CMake

**Already implemented:**

- Lighting: Blinn-Phong, PBR.
- Meterial textures supported: diffuse/base color, normal, metalness, roughness maps.
- Camera movement
- Vulkan renderer, Windows

**Platforms to support:**

- Windows (primary)
- Android
- Linux
- MacOS
- IOS
- PS

**Ways to build/test on different platforms:**

- Windows - no questions
- Android - emulator or real device. Harder, but possible.
- Linux - using virtual machine or real desktop
- MacOS, IOS - don't know how. Maybe virtual machine too.
- PS same as MacOS

**APIs to support:**

- Vulkan (primary)
- OpenGL
- Metal
- Not DirectX oriented

**Basic features to implement:**

- Cube maps.
- UI (imGui, etc.)
- Animation.
- Terrain.
- Water.
- Particle systems.
- Instancing.
- Shadows (including cascaded).
- SSAO.
- DOF.
- Compute (animation), geometry (quads, lines), tesselation (terrain, water) shaders.
- Multithreading. For what: rendering, loading, updating, collisions, etc.

**Lighting:**

- IBL.
- Deferred lighting.

**Resources:**

- Model formats:
  - all supported by ASSIMP
- Graphics formats:
  - all main formats supported by stb_image

**No sound.**

**Input:**

- investigate into SDL
