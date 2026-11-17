# Vitrine
<img width="1053" height="937" alt="Vitrine GUI" src="https://github.com/user-attachments/assets/5354cd4b-26a1-4bfb-84e1-3800d24655fb" />

Very early stages of a Qt6 application that renders through an [ANARI](https://www.khronos.org/anari/) device. Primary use case (for now) is to assist with the development of [Phenocryst](https://github.com/KSG-Technology-Consulting/Phenocryst), an out-of-tree Vulkan-backed ANARI rendering device. The Qt side displays the ANARI framebuffer and exposes a runtime UI for selecting the backend library and editing its renderer parameters via ANARI introspection.

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
axes with a labeled sphere at each tip. It can be hidden via the preferences
dialog (see below).

**Options → Cameras...** opens a dialog for editing the camera directly: the
projection subtype (perspective or orthographic), the eye / look-at / up
vectors, the near / far clip planes, and either the vertical field of view
(perspective) or the image-plane height (orthographic). Switching subtype
recreates the underlying ANARI camera on the fly. The configuration is
persisted via `QSettings` and re-applied on launch.

Cameras live in `src/camera/` behind a `Camera` abstraction: the base class
owns the shared view state (eye / center / up) and the orbit / pan / dolly
manipulators, while a subclass supplies the ANARI camera subtype and its
projection parameters. `PerspectiveCamera` (subtype `perspective`, `fovy`) and
`OrthographicCamera` (subtype `orthographic`, `height`) are implemented; the
base/derived split means an additional projection needs only a new `Camera`
subclass plus one dialog entry — the manipulation and UI code are untouched.

## Lights

**Anari → Lights...** opens a dialog for adding, removing and editing scene
lights. Three types are offered, each gated on the backend advertising the
matching `ANARI_KHR_LIGHT_*` extension:

- **Directional** — color, direction, irradiance (W/m²).
- **Point** — color, position, intensity (W/sr) and a world-space **radius**
  (0 = ideal point light; sent to the backend as the ANARI `radius` parameter).
- **Spot** — color, position, direction, intensity (W/sr), opening angle and
  falloff angle.

The light list is persisted via `QSettings` and re-applied on launch. When
**Show Light Gizmos** is enabled in the preferences, each light is drawn over
the render: directional lights as an arrow of travel, point lights as a filled
disc with rays around it (the disc is drawn with the light's `radius` value as
its on-screen pixel radius, and is omitted when the radius is 0), and spot
lights as a marker with an axis and a camera-facing circle at the cone rim.

## Preferences

**Edit → Preferences...** opens the preferences dialog where user-facing
options are edited. Preferences toggle the camera axis-overlay gizmo and the
light gizmos on or off. Preferences are persisted via `QSettings` and re-applied on launch;
the dialog is designed to grow — additional preferences are added as new rows.

## Loading models

Use **File → Open File...** to load a model at runtime. A `DataLoaderFactory` selects a loader based on the file suffix (the dialog's filters come from the same factory); unsupported types are logged and ignored. Today a single Wavefront OBJ loader is implemented: each OBJ shape becomes its own ANARI triangle-geometry surface with a `matte` material. Materials come from the `.mtl` (`Kd`/`Ks`/`Ns`), and a `map_Kd` diffuse texture, resolved relative to the `.mtl`, is bound through an ANARI `image2D` sampler.

## Build and run tests (optional)

Enable tests during configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
LD_LIBRARY_PATH=/path/to/Phenocryst/build \
  ctest --test-dir build --output-on-failure
```

The test suite mirrors Phenocryst's smoke tests (`LoadsPhenocrystLibrary`, `ClearFrameRoundTripsBackgroundColor`) and additionally exercises the application's `AnariRenderer` end-to-end (`EmitsFrameReadyOnMainThread`).
