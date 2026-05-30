# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

Out-of-source builds are required (the top-level `CMakeLists.txt` errors out if `PROJECT_SOURCE_DIR == PROJECT_BINARY_DIR`).

```bash
# Configure (Release)
# Point CMAKE_PREFIX_PATH (or anari_DIR) at your ANARI SDK install.
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/anari/install

# Build
cmake --build build -j

# Run — the binary lives under build/bin/, NOT build/.
# Set ANARI_LIBRARY_PATH so the loader can find the Phenocryst .so/.dll.
ANARI_LIBRARY_PATH=/path/to/anari_library_phenocryst/dir \
  ./build/bin/Vitrine [--dark|--light] [--anari-library <name>]
```

Tests (GoogleTest, fetched via FetchContent):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_APP_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
# Run a single test binary directly:
./build/tests/unit_tests --gtest_filter=<TestSuite.TestName>
```

`textures/` is copied next to the executable at build time via the
`texture_cpy` target — assets are loaded by path relative to
`QCoreApplication::applicationDirPath()`, so running the binary from elsewhere
will fail to find them. Models are no longer bundled: the user loads an OBJ
at runtime via **File → Open File...**, which calls
`AnariRenderer::loadSceneFromFile`. That method delegates to a `DataLoader`
chosen by `DataLoaderFactory` based on the file suffix (see `src/loader/`).
Shader compilation has been removed: Phenocryst owns its own shaders, and the
application no longer ships GLSL.

## Architecture

This is a Qt6 application that renders through an **ANARI** device. All
Vulkan work happens behind the ANARI library — typically Phenocryst
(`anari_library_phenocryst`), an out-of-tree Vulkan-backed ANARI device. The
Qt side never touches `vk*` symbols.

Module layout (each is an OBJECT library linked into the `Vitrine` executable):

- **`src/main.cpp`** — Constructs `QApplication`, applies the dark/light QSS,
  parses `--anari-library` from the command line, and hands the result to
  `AppMainWindow`. No `QVulkanInstance` lives here anymore.
- **`src/core/`** — `AppUtils.h` (style helpers, the `Vertex` struct + hash
  used to feed ANARI triangle geometry), `LogLevel.h` (shared enum used by
  the log widget and the ANARI status sink), and `AnariUtils.{h,cpp}` which
  hosts `AnariStatusSink` (translates ANARI status callbacks into
  `LogLevel`/`QString` for the log widget) and the introspection helpers
  used by the backend dialog.
- **`src/renderer/AnariRenderer.{h,cpp}`** — `QObject` that runs on the
  **GUI thread** (see "Threading" below for the why). Owns `ANARILibrary` /
  `ANARIDevice` and an `AnariScene` struct (defined in `AnariRenderer.h`) that
  bundles the `ANARIFrame` / `ANARIWorld` / `ANARIRenderer` / `ANARICamera`
  plus the scene's `ANARISurface` / `ANARIVolume` / `ANARILight` handles and
  knows how to release them (`releaseContent` for just the geometry-level
  content on reload, `releaseSceneObjects` for full teardown).
  `loadSceneFromFile` (formerly `setSceneFromObj`) asks `DataLoaderFactory`
  for a loader by suffix, drains any in-flight frame, releases the prior
  scene content, then delegates the parse to the loader — whose
  `statusMessage` signal is chained straight into the renderer's. Render loop
  is a two-state pump driven by a `QTimer` (~16 ms): one tick kicks
  `anariRenderFrame` and sets `m_frameInFlight`; subsequent ticks call
  `anariFrameReady(ANARI_NO_WAIT)` and only `anariMapFrame` + memcpy into a
  `QImage(Format_RGBA8888)` + `anariUnmapFrame` + emit `frameReady(QImage)`
  once the backend reports ready. The actual rendering still happens in
  parallel on backend-owned threads (embree workers, Vulkan queues, CUDA
  streams); the GUI thread only spends time on the map+copy.
- **`src/loader/`** — Pluggable scene-file loaders, each an OBJECT library
  linked into the executable. `DataLoader` is an abstract `QObject` base
  exposing `loadSceneFromFile(ANARIDevice, AnariScene&, path)` and a
  `statusMessage(int, QString)` signal (mirrors `AnariRenderer::statusMessage`
  so it can be chained signal-to-signal into the log). `ObjDataLoader` parses
  a Wavefront OBJ with tinyobjloader: each OBJ **shape** becomes its own ANARI
  triangle geometry (`vertex.position` / `.color`, plus `.normal` and
  `.attribute0` texcoords when present) + `matte` material + `ANARISurface`,
  and all surfaces are attached to the world's `surface` array. It also loads
  the bundled Viking Room PNG with stb_image and binds it to every material's
  `color` through a single shared `image2D` `ANARISampler`. `DataLoaderFactory`
  maps a file suffix to a loader (`createLoader`) and advertises the
  open-dialog name filters (`fileFilters`, a `QStringList`). Loaders copy mesh
  and image data into device-managed ANARI arrays (`anariNewArray*` +
  `anariMapArray`/memcpy/`anariUnmapArray`), so their CPU-side buffers don't
  need to outlive the geometry.
- **`src/ui/`** — `AnariFrameWidget` (plain `QWidget` that blits the latest
  `QImage`), `AnariBackendDialog` (modal dialog for picking the ANARI
  backend library / device subtype / renderer subtype and editing
  introspected renderer parameters), `AppMainWindow` (shell: menus,
  settings, owns the renderer thread, embeds the frame widget + log
  widget), and `CollapsibleLogWidget` (in-app log panel that receives
  ANARI status messages).
- **`src/textures/`** — Single-asset module (currently the Viking Room
  texture) with a `texture_cpy` custom target the executable depends on.
  `ObjDataLoader` loads `textures/viking_room.png` at runtime (relative to the
  executable dir) and binds it to every loaded surface via an `image2D`
  sampler — a temporary stand-in until the loader parses real materials from
  the `.mtl`. Models are no longer bundled; OBJ geometry is loaded at runtime
  via **File → Open File...**.
- **`src/resources/`** — Qt `.qrc` resources: app icons under `images/`, and
  the embedded qdarkstyle stylesheets (dark + light) compiled in via AUTORCC.

### Important cross-cutting details

- **No direct Vulkan calls in app code**: everything goes through ANARI.
  Phenocryst owns its own `VkInstance` / `VkDevice` and cannot share a
  `QVulkanInstance` — that's why the Qt frame display path is CPU-readback +
  `QPainter`, not a swapchain.
- **Backend selection**: the **Options → Rendering** dialog enumerates
  libraries (built-in list plus anything found via `ANARI_LIBRARY_PATH`),
  device subtypes (`anariGetDeviceSubtypes`), and renderer subtypes
  (`anariGetObjectSubtypes`). The parameter panel is built from
  `anariGetObjectInfo(... "parameter", ANARI_PARAMETER_LIST)` — so it
  reflects whatever the loaded backend advertises, not a hard-coded list.
  Phenocryst today exposes only `background` (FLOAT32_VEC4) and
  `ambientRadiance` (FLOAT32).
- **CLI overrides**: `--anari-library <name>` overrides the library
  persisted in `QSettings`. The full backend configuration is stored under
  `AppMainWindow/anari/backend/{library,deviceSubtype,rendererSubtype}` and
  `AppMainWindow/anari/backend/parameters` (array) in the
  `KSG-Technology-Consulting / Vitrine` org/app namespace.
- **Status messages**: `AnariStatusSink` wraps a `std::function<void(LogLevel,
  QString)>` and hands its `dispatch` function pointer to `anariLoadLibrary`.
  The renderer thread receives the C callback and re-emits it as a Qt signal
  (`statusMessage`) which the main window routes into `CollapsibleLogWidget`.
  `DataLoader`s emit their own `statusMessage(int, QString)`; `loadSceneFromFile`
  connects it to the renderer's identically-shaped signal so loader messages
  reach the same log path.
- **Threading**: `AnariRenderer` lives on the **GUI thread** as a child
  of `AppMainWindow`. Some ANARI backends statically embed CPU ray tracers
  (helide ships `embree_for_helide`) whose task schedulers crash when first
  initialized from a `QThread`-managed worker — they assume a stable
  main-thread-like caller. Running everything on the GUI thread avoids
  that whole class of bug, and `ANARI_NO_WAIT` polling keeps the event
  loop responsive: render kicks and completion polls return immediately,
  the only meaningful work the GUI thread does per frame is the
  pixel memcpy.
- **Asset paths**: bundled assets resolve relative to
  `QCoreApplication::applicationDirPath()` (e.g. `textures/...`). Mirror
  `texture_cpy` for any new bundled assets. User-supplied models are loaded by
  absolute path through **File → Open File...** and are not bundled.

## CMake custom find modules

Most third-party deps resolve through the custom `Find*.cmake` files under
`CMake/` (added to `CMAKE_MODULE_PATH` in the top-level CMakeLists). These
prefer system packages and fall back to `FetchContent` for GLM,
tinyobjloader, tinygltf, stb, and KTX — so a first configure with no
internet may fail on a fresh machine. `find_package(anari 0.16.0 REQUIRED)`
expects the ANARI SDK on `CMAKE_PREFIX_PATH` (or `anari_DIR` pointing at
the package config dir).
