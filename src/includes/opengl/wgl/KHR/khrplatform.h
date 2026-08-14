/*
 * Minimal Khronos platform type definitions required by the vendored glad
 * header. Keep this header self-contained so the Windows editor build does
 * not depend on an external OpenGL SDK installation.
 */
#ifndef __khrplatform_h_
#define __khrplatform_h_

#include <stddef.h>
#include <stdint.h>

typedef int8_t khronos_int8_t;
typedef uint8_t khronos_uint8_t;
typedef int16_t khronos_int16_t;
typedef uint16_t khronos_uint16_t;
typedef int32_t khronos_int32_t;
typedef uint32_t khronos_uint32_t;
typedef int64_t khronos_int64_t;
typedef uint64_t khronos_uint64_t;
typedef float khronos_float_t;
typedef intptr_t khronos_intptr_t;
typedef uintptr_t khronos_uintptr_t;
typedef ptrdiff_t khronos_ssize_t;

#endif /* __khrplatform_h_ */
