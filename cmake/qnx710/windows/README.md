# Windows host + QNX SDP 7.1

This directory contains the Windows-specific QNX 7.1 cross-compilation helpers for `morrow.gui`.

Files:

- `qnx.toolchain.cmake` - dedicated Windows host QNX 7.1 toolchain file
- `qcc-wrapper.cmd` / `qpp-wrapper.cmd` - compiler wrappers for CLion/compiler probing
- `make-wrapper.cmd` - optional GNU Make wrapper

## Preferred SDK location

- `D:/WorkTools/QNX/qnx710`

Fallback names that are auto-detected:

- `qnx710`
- `sdp710`

## Recommended configure example

```powershell
cmake -S E:/WorkSpace/Client/morrow.gui `
  -B E:/WorkSpace/Client/morrow.gui/cmake-build-qnx710-win-debug `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE=E:/WorkSpace/Client/morrow.gui/cmake/qnx710/windows/qnx.toolchain.cmake `
  -DQNX_ARCH=aarch64le
```

