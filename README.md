
# vulkan-terrain-engine

Real-time procedural terrain rendering engine built with C++17 and Vulkan, featuring GPU compute generation, dynamic chunk LOD, atmospheric day-night cycles, and procedural foliage scatter.
--
## Demo









<video
  src="https://github.com/user-attachments/assets/affe8e69-aa08-4c5c-a552-05b8daaeeeac"
  width="100%"
  controls></video>




---

## Features

- **GPU Compute Generation**: Heights and surface normals are computed directly on the graphics card using multi-octave gradient noise, domain warping, and ridge synthesis.
- **Dynamic Chunk LOD**: Infinite grid of terrain chunks updated around the camera with automatic distance-based mesh decimation (LOD 0 to LOD 3) to keep frame rates steady.
- **Procedural Biomes**:
  - **Alpine Mountains**: Jagged rocky peaks, high plateaus with snow cover, evergreen pine belts, and pebble riverbeds.
  - **Rolling Hills**: Smooth pastoral ridges, lush meadows, and agricultural valleys.
  - **Desert Canyons**: Multi-layered sandstone cliffs, stepped mesas, and eroded scrublands.
  - **Tropical Islands**: Volcanic basalt peaks surrounded by coastal jungle canopies and sandy beaches.
  - **Multi-Biome World**: Seamless macro continental blending connecting all environments into a single coherent map.
- **Atmospheric Day-Night Cycle**:
  - Continuous 24-hour celestial progression with smooth transitions between sunrise, noon, sunset, dusk, and deep night.
  - Procedural starfield with twinkling stars and Milky Way dust band.
  - Silvery directional moonlight and warm golden sunlight.
  - Procedural volumetric cloud dome with realistic wind drift.
- **Instanced 3D Foliage and Scatter**:
  - Biome-specific models including conifer pine trees, deciduous oaks, tropical palms, saguaro cacti, boulders, and grass tufts.
  - Placement rules based on elevation, slope, and water height with root sinking to prevent floating assets on steep hills.
  - Real-time density controls.
- **Diagnostics and Controls**:
  - On-screen diagnostics overlay showing real-time frame rates, coordinates, chunk counts, and active parameters.
  - Visualization modes for LOD levels, normal maps, slope steepness, and wireframe mesh rendering.

---

## Controls

| Key | Description |
| :--- | :--- |
| **W, A, S, D** | Fly camera forward, left, backward, right |
| **Shift** | Turbo speed boost |
| **Space / E** | Fly upward |
| **Ctrl / Q** | Fly downward |
| **Tab / Esc** | Lock or release mouse cursor |
| **T** | Pause or resume day-night cycle |
| **[ / ]** | Scrub time backward or forward by 30 minutes |
| **1 - 5** | Switch presets (1: Mountains, 2: Hills, 3: Canyons, 4: Islands, 5: Multi-Biome) |
| **E** | Toggle 3D foliage and scatter rendering |
| **U / I** | Decrease or increase foliage density |
| **O / P** | Lower or raise water elevation |
| **Z / X** | Decrease or increase terrain height amplitude |
| **C / V** | Decrease or increase terrain noise frequency |
| **- / =** | Decrease or increase view distance |
| **L** | Toggle dynamic LOD vs manual LOD lock |
| **J / K** | Step through manual LOD levels when locked |
| **M** | Cycle shading modes (Realistic, LOD Colors, Normals, Slope) |
| **F** | Toggle wireframe rendering |
| **H** | Toggle HUD overlay display |

---

## Project Structure

```
vulkan-terrain/
├── CMakeLists.txt
├── include/
│   ├── app/
│   │   └── Application.hpp
│   ├── camera/
│   │   └── Camera.hpp
│   ├── core/
│   │   ├── VulkanBuffer.hpp
│   │   ├── VulkanContext.hpp
│   │   ├── VulkanPipeline.hpp
│   │   └── VulkanSwapchain.hpp
│   ├── renderer/
│   │   ├── FoliageRenderer.hpp
│   │   ├── Renderer.hpp
│   │   ├── SkyRenderer.hpp
│   │   └── UIOverlay.hpp
│   └── terrain/
│       ├── ChunkManager.hpp
│       ├── ComputeTerrainGenerator.hpp
│       ├── TerrainChunk.hpp
│       └── TerrainTypes.hpp
├── shaders/
│   ├── foliage.frag
│   ├── foliage.vert
│   ├── sky.frag
│   ├── sky.vert
│   ├── terrain.comp
│   ├── terrain.frag
│   ├── terrain.vert
│   ├── ui.frag
│   └── ui.vert
└── src/
    ├── app/
    │   └── Application.cpp
    ├── camera/
    │   └── Camera.cpp
    ├── core/
    │   ├── VulkanBuffer.cpp
    │   ├── VulkanContext.cpp
    │   ├── VulkanPipeline.cpp
    │   └── VulkanSwapchain.cpp
    ├── main.cpp
    ├── renderer/
    │   ├── FoliageRenderer.cpp
    │   ├── Renderer.cpp
    │   ├── SkyRenderer.cpp
    │   └── UIOverlay.cpp
    └── terrain/
        ├── ChunkManager.cpp
        ├── ComputeTerrainGenerator.cpp
        └── TerrainChunk.cpp
```

---

## Prerequisites

To build and run the engine, you will need:

1. **C++17 Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
2. **CMake**: Version 3.22 or higher.
3. **Vulkan SDK**: LunarG Vulkan SDK (1.2 or higher) including the `glslc` shader compiler.
4. **GLFW3**: Windowing and input handling library.
5. **GLM**: Header-only mathematics library for graphics software.

---

## Building and Running

### macOS (Apple Silicon & Intel)

Install required packages via Homebrew:
```bash
brew install cmake glfw glm
# Install LunarG Vulkan SDK from https://vulkan.lunarg.com/
```

Make sure your Vulkan SDK environment is active:
```bash
source ~/VulkanSDK/*/setup-env.sh
```

Configure and build:
```bash
cmake -B build -S .
cmake --build build
```

Run the application:
```bash
./build/terrain
```

---

### Linux (Ubuntu / Debian / Fedora / Arch)

Install dependencies on Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake libvulkan-dev vulkan-tools libglfw3-dev libglm-dev glslc
```

On Fedora:
```bash
sudo dnf install gcc-c++ cmake vulkan-loader-devel glfw-devel glm-devel glslc
```

On Arch Linux:
```bash
sudo pacman -S base-devel cmake vulkan-devel glfw-x11 glm shaderc
```

Build and run:
```bash
cmake -B build -S .
cmake --build build -j$(nproc)
./build/terrain
```

---

### Windows (Visual Studio / MSVC)

1. Download and install the [Vulkan SDK](https://vulkan.lunarg.com/). Make sure the SDK bin folder containing `glslc.exe` is added to your system `PATH`.
2. Install [GLFW](https://www.glfw.org/) and [GLM](https://github.com/g-truc/glm) using [vcpkg](https://vcpkg.io/) or CMake package managers:
   ```cmd
   vcpkg install glfw3:x64-windows glm:x64-windows
   ```
3. Generate project files and build using CMake:
   ```cmd
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```
4. Run the executable:
   ```cmd
   .\build\Release\terrain.exe
   ```

---

## Extending the Engine

### Adding a New Biome

1. Open `shaders/terrain.comp` and add your height calculation in the compute pipeline. You can use standard fractal Brownian motion (fbm) or custom domain warps.
2. Open `shaders/terrain.frag` and add a new color calculation function that defines the surface textures, slope rock exposures, and elevation bands.
3. Open `include/terrain/TerrainTypes.hpp` and add your new preset to `TerrainPreset` enum.
4. Add the corresponding CPU height formula in `src/renderer/FoliageRenderer.cpp` if you want scatter objects to automatically align with your new terrain surface.

### Adding New Foliage and Scatter Models

1. Open `src/renderer/FoliageRenderer.cpp` and increase `NUM_MODELS`.
2. In `FoliageRenderer::createModels()`, define your custom vertex and index data (or procedural primitives like cylinders, cones, and polyhedra).
3. In `FoliageRenderer::updateInstances()`, write placement rules based on height, slope, moisture, and random hashing to determine where instances spawn.

