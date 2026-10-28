<div align="center">

# MorrowUI

_A cross-platform 3D GUI framework for automotive cockpits._

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](docs/INTEGRATION.md)
[![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.10-064F8C?logo=cmake&logoColor=white)](#build-instructions)
[![Platform](https://img.shields.io/badge/Platform-QNX%20%7C%20Windows%20%7C%20Linux-2D6CDF)](#platform-support)
[![License](https://img.shields.io/badge/License-MIT-3FB950)](LICENSE)

### ▶ [MORROW Showreel — Live Demo Gallery](https://yuhenmeili.github.io/videos/?cat=morrowui)

</div>

---

## About

MorrowUI uses a **retained-mode UI model** (widget tree + component composition,
not immediate mode) and keeps the path from widget state to GPU draw calls as
short as possible: every frame it collects renderables directly, builds
batches, and encodes GPU commands — with no scene snapshot in between.

The engine ships as a static library. Its only public surface is the
`include/morrow/` header directory plus the `morrow` library; GL/EGL/GLFW and
all implementation headers are unreachable from standard integrations
(enforced by build-time gates). See [docs/INTEGRATION.md](docs/INTEGRATION.md)
for the API contract and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the
architecture and design constraints.

## Features

**Rendering core**

- Retained widget tree with component-based design and a short widget-to-draw path
- Batch rendering: batch-compatibility keys, an SSBO instanced path, and an
  order-safe fallback, with batch-break statistics
- Single-threaded / render-thread modes: fixed-capacity command-buffer frame
  slots with fence-based recycling
- On-demand rendering (request-render gate) — zero frame cost for static UI

**Controls & layout**

- `MR*` control family: text/texture/check/radio/segmented/menu buttons,
  sliders, progress bars, spin boxes, scroll containers, lists, trees,
  popups & menus, rich text, single-/multi-line input
- Layout containers: Center / Margin / HBox / VBox / Grid / Split / Tab, with drag-and-drop

**Effects**

- Frosted glass: shared Kawase backdrop-blur chain with 3-level radius
  quantization, Standard/LowCost/Off quality tiers, and dirty-signature
  caching that makes static backgrounds free
- Shadows (underlay channel), flowing light, bounce, gear-shift effects,
  parallax, global color modulation
- CPU and GPU 2D particle systems

**Text**

- Runtime SDF glyph atlas (stb_truetype) — any font size without re-rasterization,
  dirty-region atlas uploads
- Rich text, single-/multi-line/password input, text metrics from the same
  source as rendering

**Media & assets**

- Video stream component (`FrameProvider` per-frame callback) and OES external
  textures (EGLImage / pmem zero-copy)
- png / jpg / Basis / KTX2 textures (magic-byte sniffing, one-shot in-memory construction)
- Texture atlases (file / in-memory assembly / frame animation) and
  application-registered static atlases

**3D**

- GLTF/GLB loading (PBR + IBL), skinning animation, async loading, bounding-box normalization
- `MR3DSceneView`: offscreen-rendered 3D scenes composited into the UI, with orbit-camera interaction

**Tooling**

- F3 debug overlay: FPS, batch/draw statistics, batch-break reasons (compile-time removable)
- Object registry with JSON snapshot export (`Engine::writeObjectSnapshot`)
- MorrowEditor scene editor (Windows): scene tree / inspector / asset browser /
  undo-redo / standalone scene runtime

---

## ✨ Showcase

<div align="center">

**[▶ Watch all live demos in the MORROW Showreel](https://yuhenmeili.github.io/videos/?cat=morrowui)**

Controls · Frosted glass · Scrolling · Typography · Sliders & progress · Gear effects · Particles · Video streaming · GLTF 3D · Animation · Scene components · 3D AVM (real vehicle)

</div>

All demo recordings live in the [MORROW Showreel](https://yuhenmeili.github.io/videos/)
gallery: a dark card wall with hover-to-preview and click-to-play in the modal
viewer. The gallery ingests videos automatically — a new recording only needs
to be dropped into the site repo under `public/videos/MorrowUI/` followed by one
run of the ingest script; posters and metadata are generated and published
automatically. **This README does not maintain per-demo video links.**

Deep links (share straight to a view):

- [`videos/?cat=morrowui`](https://yuhenmeili.github.io/videos/?cat=morrowui) — jump straight to the MorrowUI category
- [`videos/?v=<title>`](https://yuhenmeili.github.io/videos/?v=液态玻璃) — open a specific video directly (e.g. "液态玻璃" / Liquid Glass)

## Demos

Every sample under `samples/` is executable documentation for one engine
capability; assets are copied next to the binaries at build time. Each demo has
a walkthrough in `docs/samples/<Demo>.md` (index:
[docs/README.md](docs/README.md) — in Chinese).

| Demo | Focus | Key components |
|---|---|---|
| **AnimationEffectsDemo** | Animation toolkit: anchor scale, bounce, brake pedal, flowing light, frame animation | `MRAnchorPointScale` `MRBounce` `MRFlowingLight` `Tween` |
| **BackblurDemo** | Kawase backdrop blur: segmented rendering, shared blur chain, tint fallback | `BackdropBlur` `BackdropBlurManager` |
| **ControlsDemo** | Interactive controls: buttons, selection, menus, numeric input | `MRButton` `MRCheckBox` `MRRadioButton` `MRSpinBox` |
| **DebugDemo** | Debug facilities: batch-statistics acceptance, on-demand rendering, F3 overlay, object snapshots | `BatchStatistics` `ObjectRegistry` |
| **GLTFDemo** | GLTF/GLB 3D model loading and interaction | `MR3DSceneView` `Scene3DAsyncLoader` `OrbitController` |
| **GearsDemo** | Gear-shift effects: iris, opening, shine, selection | `MRGearsIris` `MRGearsOpening` `MRGearsShine` `MRGearsSelect` |
| **IBLPrecomputeDemo** | IBL map precomputation (CLI, no window) | `IBLPrecompute` |
| **ImageDemo** | Images: rounded corners, UV cropping, batched animation, shadows, atlas regions | `MRImage` `Shadow` `TextureAtlas` |
| **ListNavigationDemo** | List selection, tree expansion, long-list scrolling | `MRItemList` `MRTree` `MRScrollContainer` |
| **PMemoryDemo** | QNX PMEM/OES zero-copy texture ingestion | `Texture(OES)` `loadPmemData` |
| **Particles2DDemo** | CPU vs. GPU 2D particle systems | `MRCPUParticles2D` `MRGPUParticles2D` |
| **RangeControlsDemo** | Sliders & progress bars: linking, vertical, tween loops | `MRSlider` `MRProgressBar` `Tween` |
| **SceneEffectsDemo** | Parallax background, global color modulation, tooltip, dialog | `MRParallaxBackground` `MRCanvasModulate` |
| **TextDemo** | Text input (multi-line/single-line/password), alignment, rich text | `MRTextEdit` `MRLineEdit` `MRLabel` `MRRichTextLabel` |
| **VideoStreamPlayerDemo** | Video playback: in-memory frame source, speed control, event echo | `MRVideoStreamPlayer` |

Build and run a demo:

```bash
cmake --build cmake-build-linux --target ImageDemo -j
./cmake-build-linux/ImageDemo
```

---

## Platform Support

| Platform | Backend | Macro branch |
|---|---|---|
| QNX | EGL / GLES (screen) | `OPENGL_EGL` |
| Linux | GLFW + desktop OpenGL | `OPENGL_GLFW` |
| Windows | GLFW + desktop OpenGL | `OPENGL_GLFW` |

Platform-specific CMake / toolchain helpers live in `cmake/`; for
Windows-hosted QNX cross-compilation see
[cmake/qnx/windows/README.md](cmake/qnx/windows/README.md).

## Build Instructions

### Common

- CMake >= 3.10
- C++17 compiler
- Options: `MORROW_ENABLE_BASISU` (default ON; turn OFF to shrink the binary
  when assets are png/jpg only), `MORROW_ENABLE_DEBUG_OVERLAY` and
  `MORROW_ENABLE_OBJECT_DIAGNOSTICS` (ON in Debug, OFF in Release by default)

### 1) Linux

Install GLFW and pkg-config, then:

```bash
sudo apt install -y libglfw3-dev pkg-config
cmake -S . -B cmake-build-linux -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-linux -j
```

### 2) QNX (cross-compile)

Requires an installed QNX SDP (e.g. `/home/lance/qnx800`) with the environment
script sourced:

```bash
cmake -S . -B cmake-build-qnx -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=<repo>/qnx/qnx.toolchain.cmake \
  -DCMAKE_C_COMPILER=<repo>/qnx/qcc-wrapper.sh \
  -DCMAKE_CXX_COMPILER=<repo>/qnx/qpp-wrapper.sh \
  -DCMAKE_MAKE_PROGRAM=<repo>/qnx/make-wrapper.sh
cmake --build cmake-build-qnx -j
```

> For Windows-hosted QNX cross-compilation, use the newer files under
> `cmake/qnx/windows/` instead of the legacy `qnx/` directory.

### 3) Windows

The MSYS2 CLANG64 toolchain (`clang` + `ninja`) is recommended; CLion with the
same toolchain works as well:

```bash
pacman -S mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-ninja
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-win
```

If CMake cannot find GLFW, place the platform's prebuilt GLFW library under
`libs/GLFW`.

> QNX binaries cannot run on Linux/Windows directly (incompatible loader; the
> process exits with code 127). Keep a separate build directory per platform
> (`cmake-build-linux` / `cmake-build-qnx` / `build-win`).

## Debug Overlay

The FPS / batch-statistics overlay is compiled in by default but starts hidden:

- Press `F3` at runtime to toggle it (Windows desktop and MorrowEditor);
- Set `EngineOptions::debugOverlayVisible = true` to show it on startup;
- Configure `-DMORROW_ENABLE_DEBUG_OVERLAY=OFF` to remove the overlay from the
  runtime build entirely.

## Documentation

| Document | Contents |
|---|---|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Architecture: layering, frame loop, batching, GPU resource model, threading boundaries, invariants (Chinese) |
| [docs/INTEGRATION.md](docs/INTEGRATION.md) | Integration guide: public API contract, build options, migration & compatibility promises (Chinese) |
| [docs/README.md](docs/README.md) | Samples index: how to run each demo and which components it exercises (Chinese) |
| `docs/performance/` | Performance optimization directions and roadmap |
| `editor/` | MorrowEditor scene editor (Windows) |

## License

[MIT](LICENSE)
