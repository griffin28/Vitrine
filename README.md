# Vitrine
<img width="1104" height="1013" alt="phenocryst_ss2" src="https://github.com/user-attachments/assets/8005dac1-529e-413b-b2f8-dc9709fa990e" />

Qt6 application that renders through an [ANARI](https://www.khronos.org/anari/) device. All Vulkan work happens inside the ANARI backend — typically [Phenocryst](https://github.com/KSG-Technology-Consulting/Phenocryst), an out-of-tree Vulkan-backed ANARI rendering device. The Qt side displays the ANARI framebuffer and exposes a runtime UI for selecting the backend library and editing its renderer parameters via ANARI introspection.

## Project layout

- `src/` main application, renderer, camera abstractions, scene-file loaders, UI, and resources
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

## Camera control

The view is driven by an interactive camera with the mouse over the render area:

- **Left-drag** — orbit around the look-at center. Vertical drags pitch a full
  360° over the poles (the camera can tumble fully upside down).
- **Middle-drag** (or **Shift + left-drag**) — pan the camera in the view plane.
- **Mouse wheel** — dolly toward / away from the look-at center.

A small axis gizmo in the bottom-left corner of the render area shows the
current camera orientation, drawing the world X (red), Y (green), and Z (blue)
axes with a labeled sphere at each tip.

Cameras live in `src/camera/` behind a `Camera` abstraction: the base class
owns the shared view state (eye / center / up) and the orbit / pan / dolly
manipulators, while a subclass supplies the ANARI camera subtype and its
projection parameters. Only a `PerspectiveCamera` (subtype `perspective`,
`fovy`) is implemented today; the split is designed so additional projections
(e.g. orthographic) can be added without touching the manipulation or UI code.

## Loading models

Use **File → Open File...** to load a model at runtime. A `DataLoaderFactory` selects a loader based on the file suffix (the dialog's filters come from the same factory); unsupported types are logged and ignored. Today a single Wavefront OBJ loader is implemented: each OBJ shape becomes its own ANARI triangle-geometry surface with a `matte` material. As a temporary stand-in for real `.mtl` material parsing, every loaded surface is textured with the bundled Viking Room image via an ANARI `image2D` sampler.

## Build and run tests (optional)

Enable tests during configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
LD_LIBRARY_PATH=/path/to/Phenocryst/build \
  ctest --test-dir build --output-on-failure
```

The test suite mirrors Phenocryst's smoke tests (`LoadsPhenocrystLibrary`, `ClearFrameRoundTripsBackgroundColor`) and additionally exercises the application's `AnariRenderer` end-to-end (`EmitsFrameReadyOnMainThread`).
