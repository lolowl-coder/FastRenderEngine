# FSR - Fast render engine

Engine with amphasis on simplicity and optimum resource consumption.

**Values:**
Simple
Fast
Crossplatform

**Buld system:**
CMake

**Platforms to support:**

- Windows
- Android
- Linux
- MacOS
- IOS
- PS

**Ways to build/test on different platforms:**

- Windows - no questions
- Android. Here I need some emulator or real device. Harder, but possible.
- Linux - using virtual machine or real desktop
- MacOS, IOS - don't know how. Maybe virtual machine too.
- PS same as MacOS

**APIs to support:**

- Vulkan (primary)
- OpenGL
- Metal
- Not DirectX oriented

**Basic features to implement:**

- Multithreading
- Main thread for rendering
- Other threads for loading, updating, collisions, etc.

**Lighting:**

- Blinn-Phong
- PBR
- Deferred

**Resources:**

- Model formats:
  - all supported by ASSIMP
- Graphics formats:
  - all main formats supported by stb_image

**No sound.**

**Input:**

- investigate into SDL
