# morrow.gui
A cross-platform 3D GUI framework for automotive cockpits. Optimized for QNX and Windows, MorrowUI features an extensible architecture that supports custom platform integration and high-fidelity 3D model rendering.

platform backends:
- QNX: EGL/GLES (`OPENGL_EGL`)
- Linux: GLFW + desktop OpenGL (`OPENGL_GLFW`)
- Windows: GLFW + desktop OpenGL (`OPENGL_GLFW`)

## Features

1. Multiple windows
2. Multi-threaded architecture
3. Component-based design
4. GUI layout and animation
5. Custom shaders
6. Dynamic font rendering
7. Right-handed coordinate system
8. GUI shadow support
9. Batch rendering
10. Touch input support

## Platform Support

- `QNX`
- `Linux`
- `Windows`

Platform-specific CMake/toolchain helper files are organized under:

- `cmake/`

Windows host + QNX SDP 8.0 instructions are available at:

- `cmake/qnx/windows/README.md`

`src/CMakeLists.txt` now uses explicit platform branches:

- `if(QNX)`
- `elseif(Linux)`
- `elseif(Windows)`

## Prerequisites

### Common

- CMake >= 3.10
- C++17 compiler

### Linux

Install GLFW and pkg-config:

```bash
sudo apt update
sudo apt install -y libglfw3-dev pkg-config
```

### Windows

- CLion + MinGW (or MSVC with equivalent adaptation)
- Prebuilt GLFW library is expected under `libs/GLFW`

### QNX (SDP 8.0 example)

- Installed SDP, e.g. `/home/lance/qnx800`
- Valid QNX license
- Environment script available, e.g. `/home/lance/qnx800/qnxsdp-env.sh`

## Build Instructions

### 1) Linux (native)

```bash
cmake -S . -B cmake-build-linux -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-linux -j
```

Run sample:

```bash
./cmake-build-linux/ImageDemo
```

### 2) QNX (cross-compile)

```bash
cmake -S . -B cmake-build-qnx -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/home/lance/workspace/morrow.gui/qnx/qnx.toolchain.cmake \
  -DCMAKE_C_COMPILER=/home/lance/workspace/morrow.gui/qnx/qcc-wrapper.sh \
  -DCMAKE_CXX_COMPILER=/home/lance/workspace/morrow.gui/qnx/qpp-wrapper.sh \
  -DCMAKE_MAKE_PROGRAM=/home/lance/workspace/morrow.gui/qnx/make-wrapper.sh
cmake --build cmake-build-qnx -j
```

> Linux/QNX helper files above remain unchanged. For Windows-hosted QNX cross-compilation,
> use the new files under `cmake/qnx/windows/` instead of the legacy `qnx/` directory.

### 3) Windows

Use CLion with a Windows toolchain and configure/build normally.

If CMake cannot find GLFW on Windows, place the proper Windows `glfw` library in:

- `libs/GLFW`

## CLion Notes

- Keep separate CMake profiles/build directories per platform.
- Do not run QNX binaries on Linux directly.
  - If you run a QNX binary on Linux, process may exit with code `127` because Linux cannot load the QNX runtime loader.

Recommended build dirs:

- Linux: `cmake-build-linux`
- QNX: `cmake-build-qnx`
- Windows: `cmake-build-win`

## Usage

Sample programs are under `samples/` and are built as executable targets (for example `ImageDemo`, `GLTFDemo`, `ShadowDemo`).
