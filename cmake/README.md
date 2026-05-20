# CMake platform configs

This directory stores platform-specific CMake/toolchain helper files.

Current layout:

- `cmake/qnx710/windows/` - dedicated Windows host + QNX SDP 7.1 cross-compilation helpers
- `cmake/qnx800/windows/` - dedicated Windows host + QNX SDP 8.0 cross-compilation helpers
- `cmake/qnx/qnx710.toolchain.cmake` - compatibility entry file that forwards to `cmake/qnx710/windows/qnx.toolchain.cmake`
- `cmake/qnx/qnx800.toolchain.cmake` - compatibility entry file that forwards to `cmake/qnx800/windows/qnx.toolchain.cmake`

Existing legacy Linux/QNX helper files under `qnx/` are intentionally left unchanged.

