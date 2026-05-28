# Vitrine
<img width="1050" height="959" alt="Vulkan Screenshot" src="https://github.com/user-attachments/assets/74f41c63-6b02-467f-909b-e57aae9b50e2" />

Qt6 application that renders through an [ANARI](https://www.khronos.org/anari/) device. All Vulkan work happens inside the ANARI backend — typically [Phenocryst](https://github.com/KSG-Technology-Consulting/Phenocryst), an out-of-tree Vulkan-backed ANARI rendering device. The Qt side displays the ANARI framebuffer and exposes a runtime UI for selecting the backend library and editing its renderer parameters via ANARI introspection.

## Project layout

- `src/` main application, renderer, UI, and resources
- `tests/` unit tests (GoogleTest)
- `CMake/` custom CMake find modules and project utilities

## Requirements

- CMake 3.28+
- C++ compiler with C++20 support
- Qt6 (Core, Gui, Widgets, Test)
- ANARI SDK 0.16+
- At least one installed ANARI backend library, e.g.:
  - [Phenocryst](https://github.com/KSG-Technology-Consulting/Phenocryst) (Vulkan, default)
  - `helide` (CPU/embree, ships with the ANARI SDK)
  - `visrtx` (NVIDIA OptiX, ships with the ANARI SDK)

Notes:

- Out-of-source builds are required (in-source builds are blocked).
- Some dependencies can be fetched automatically at configure time via CMake `FetchContent` (internet access required): GLM, tinyobjloader, tinygltf, stb, KTX, and GoogleTest (for tests).

## Build (Linux)

From the repository root, point `CMAKE_PREFIX_PATH` (or `anari_DIR`) at your ANARI SDK install:

1. Configure

	```bash
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_PREFIX_PATH=/path/to/anari/install
	```

2. Build

	```bash
	cmake --build build -j
	```

The main executable target is `Vitrine`.

## Run

After building, set `LD_LIBRARY_PATH` so the ANARI loader can find your backend's shared library, then run:

```bash
LD_LIBRARY_PATH=/path/to/backend/lib ./build/bin/Vitrine [--dark|--light] [--anari-library <name>]
```

Examples:

```bash
# Phenocryst (default)
LD_LIBRARY_PATH=/path/to/Phenocryst/build ./build/bin/Vitrine

# Helide
./build/bin/Vitrine --anari-library helide
```

Once running, **Options → Rendering...** opens a dialog that enumerates the available backend libraries, device subtypes, and renderer subtypes, and builds an editor panel for whatever parameters the chosen renderer advertises (e.g. `background`, `ambientRadiance`). The selection is persisted via `QSettings` under `KSG-Technology-Consulting / Vitrine`.

## Build and run tests (optional)

Enable tests during configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
LD_LIBRARY_PATH=/path/to/Phenocryst/build \
  ctest --test-dir build --output-on-failure
```

The test suite mirrors Phenocryst's smoke tests (`LoadsPhenocrystLibrary`, `ClearFrameRoundTripsBackgroundColor`) and additionally exercises the application's `AnariRenderer` end-to-end (`EmitsFrameReadyOnMainThread`).
