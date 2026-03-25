# Vulkan Sandbox
<img width="1050" height="959" alt="Vulkan Screenshot" src="https://github.com/user-attachments/assets/74f41c63-6b02-467f-909b-e57aae9b50e2" />

Vulkan + Qt6 application built with CMake.

## Project layout

- `src/` main application, renderer, UI, shaders, and resources
- `tests/` unit tests (GoogleTest)
- `CMake/` custom CMake find modules and project utilities

## Requirements

- CMake 3.28+
- C++ compiler with C++20 support
- Qt6 (Core, Gui, Widgets)
- Vulkan SDK (or Vulkan development packages)
  - `glslangValidator` must be available (used to compile shaders)

Notes:

- Out-of-source builds are required (in-source builds are blocked).
- Some dependencies can be fetched automatically at configure time via CMake `FetchContent` (internet access required): GLM, tinyobjloader, tinygltf, stb, KTX, and GoogleTest (for tests).

## Build (Linux)

From the repository root:

1. Configure

	```bash
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	```

2. Build

	```bash
	cmake --build build -j
	```

The main executable target is `MyProject`.

## Run

After building, run:

```bash
./build/bin/MyProject
```

Shader binaries are generated during the build and copied next to the executable under a `shaders/` folder.

## Build and run tests (optional)

Enable tests during configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Optional: enable C++20 Vulkan module target

```bash
cmake -S . -B build -DENABLE_CPP20_MODULE=ON
cmake --build build -j
```
