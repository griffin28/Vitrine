# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

Out-of-source builds are required (in-source builds are blocked by CMake).

```bash
# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run
./build/bin/MyProject

# Command-line options
./build/bin/MyProject --dark        # Dark theme (default)
./build/bin/MyProject --light       # Light theme
./build/bin/MyProject --gpu <index> # Select GPU by index
```

## Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Shaders

GLSL shaders live in `src/shaders/`. CMake compiles them to SPIR-V via `glslangValidator` and copies the `.spv` files to `build/bin/shaders/`. Rebuilding the project recompiles shaders automatically. Compiled `.spv` files are not committed.

## Architecture

**Entry point:** `src/main.cpp` — creates `QApplication` and `QVulkanInstance`, enables validation layers in debug, detects Vulkan API version (prefers 1.4, falls back to 1.3/1.2), and launches `AppMainWindow`.

**Initialization flow:**
```
main() → AppMainWindow → VulkanWindow (QVulkanWindow)
                       → VulkanRenderer::initResources()          # pipeline, descriptors, shaders
                       → VulkanRenderer::initSwapChainResources() # framebuffers, command buffers
                       → VulkanRenderer::startNextFrame()         # per-frame render loop
```

**Key components:**

| File | Purpose |
|------|---------|
| `src/renderer/VulkanRenderer.cpp` | Core Vulkan rendering (~1,700 lines). Manages the full pipeline: vertex/index/uniform buffers, descriptor sets (UBO at binding 0, sampler at binding 1), MSAA, texture loading with mipmap generation, OBJ model loading, and per-frame command buffer recording. |
| `src/core/AppUtils.h` | `Vertex` and `UniformBufferObject` structs, GPU selection logic (`pickPhysicalDevice`), and theme stylesheet loaders. |
| `src/ui/AppMainWindow.cpp` | Main window, menus, Vulkan properties dialog, MSAA selection (1x–16x), and settings persistence via `QSettings`. |
| `src/ui/VulkanWindow.h` | Thin `QVulkanWindow` subclass; instantiates `VulkanRenderer`. |
| `src/ui/CollapsibleLogWidget.h` | Thread-safe, color-coded log panel embedded in the main window. |

**GPU selection** (`core/AppUtils.h` → `pickPhysicalDevice`): requires Vulkan 1.3+, graphics + compute queues, prefers NVIDIA. Overridable with `--gpu`.

**Rendering features:** MSAA (configurable sample count, minSampleShading 0.3), depth testing, back-face culling, alpha blending, per-swapchain-image uniform buffers.

**External dependencies** are fetched automatically via `FetchContent`: GLM, tinyobjloader, tinygltf, stb, KTX. System dependencies: Qt6 and Vulkan SDK (including `glslangValidator`).

**CMake helpers** in `CMake/ProjectUtils.cmake` define `add_shaders_target()` and `add_slang_shader_target()` for shader compilation.
