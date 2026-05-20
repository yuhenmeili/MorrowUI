# Windows host + QNX SDP 8.0

This directory contains the Windows-specific QNX 8.0 cross-compilation helpers for `morrow.gui`.

Files:

- `qnx.toolchain.cmake` - dedicated Windows host QNX 8.0 toolchain file
- `qcc-wrapper.cmd` / `qpp-wrapper.cmd` - compiler wrappers for CLion/compiler probing
- `make-wrapper.cmd` - optional GNU Make wrapper

## Preferred SDK location

- `D:/WorkTools/QNX/qnx800`

Fallback name that is auto-detected:

- `qnx800`

## Recommended configure example

```powershell
cmake -S E:/WorkSpace/Client/morrow.gui `
  -B E:/WorkSpace/Client/morrow.gui/cmake-build-qnx800-win-debug `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE=E:/WorkSpace/Client/morrow.gui/cmake/qnx800/windows/qnx.toolchain.cmake `
  -DQNX_ARCH=aarch64le
```

