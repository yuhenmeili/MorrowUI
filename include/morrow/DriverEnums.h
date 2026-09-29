//
// Created by 0060328 on 25-9-17.
//

#ifndef DRIVERENUMS_H
#define DRIVERENUMS_H
#include <cstddef>
#include <cstdint>

enum class TextureFormat : uint16_t {
    // 8-bits per element
    R8, R8_SNORM, R8UI, R8I, STENCIL8,ALPHA8,

    // 16-bits per element
    R16F, R16UI, R16I,
    RG8, RG8_SNORM, RG8UI, RG8I,
    RGB565,
    RGB9_E5, // 9995 is actually 32 bpp but it's here for historical reasons.
    RGB5_A1,
    RGBA4,
    DEPTH16,

    // 24-bits per element
    RGB8, SRGB8, RGB8_SNORM, RGB8UI, RGB8I,
    DEPTH24,

    // 32-bits per element
    R32F, R32UI, R32I,
    RG16F, RG16UI, RG16I,
    R11F_G11F_B10F,
    RGBA8, SRGB8_A8, RGBA8_SNORM,
    UNUSED, // used to be rgbm
    RGB10_A2, RGBA8UI, RGBA8I,
    DEPTH32F, DEPTH24_STENCIL8, DEPTH32F_STENCIL8,

    // 48-bits per element
    RGB16F, RGB16UI, RGB16I,

    // 64-bits per element
    RG32F, RG32UI, RG32I,
    RGBA16F, RGBA16UI, RGBA16I,

    // 96-bits per element
    RGB32F, RGB32UI, RGB32I,

    // 128-bits per element
    RGBA32F, RGBA32UI, RGBA32I,

    // compressed formats

    // Mandatory in GLES 3.0 and GL 4.3
    EAC_R11, EAC_R11_SIGNED, EAC_RG11, EAC_RG11_SIGNED,
    ETC2_RGB8, ETC2_SRGB8,
    ETC2_RGB8_A1, ETC2_SRGB8_A1,
    ETC2_EAC_RGBA8, ETC2_EAC_SRGBA8,

    // Available everywhere except Android/iOS
    DXT1_RGB, DXT1_RGBA, DXT3_RGBA, DXT5_RGBA,
    DXT1_SRGB, DXT1_SRGBA, DXT3_SRGBA, DXT5_SRGBA,

    // ASTC formats are available with a GLES extension
    RGBA_ASTC_4x4,
    RGBA_ASTC_5x4,
    RGBA_ASTC_5x5,
    RGBA_ASTC_6x5,
    RGBA_ASTC_6x6,
    RGBA_ASTC_8x5,
    RGBA_ASTC_8x6,
    RGBA_ASTC_8x8,
    RGBA_ASTC_10x5,
    RGBA_ASTC_10x6,
    RGBA_ASTC_10x8,
    RGBA_ASTC_10x10,
    RGBA_ASTC_12x10,
    RGBA_ASTC_12x12,
    SRGB8_ALPHA8_ASTC_4x4,
    SRGB8_ALPHA8_ASTC_5x4,
    SRGB8_ALPHA8_ASTC_5x5,
    SRGB8_ALPHA8_ASTC_6x5,
    SRGB8_ALPHA8_ASTC_6x6,
    SRGB8_ALPHA8_ASTC_8x5,
    SRGB8_ALPHA8_ASTC_8x6,
    SRGB8_ALPHA8_ASTC_8x8,
    SRGB8_ALPHA8_ASTC_10x5,
    SRGB8_ALPHA8_ASTC_10x6,
    SRGB8_ALPHA8_ASTC_10x8,
    SRGB8_ALPHA8_ASTC_10x10,
    SRGB8_ALPHA8_ASTC_12x10,
    SRGB8_ALPHA8_ASTC_12x12,

    // RGTC formats available with a GLES extension
    RED_RGTC1, // BC4 unsigned
    SIGNED_RED_RGTC1, // BC4 signed
    RED_GREEN_RGTC2, // BC5 unsigned
    SIGNED_RED_GREEN_RGTC2, // BC5 signed

    // BPTC formats available with a GLES extension
    RGB_BPTC_SIGNED_FLOAT, // BC6H signed
    RGB_BPTC_UNSIGNED_FLOAT, // BC6H unsigned
    RGBA_BPTC_UNORM, // BC7
    SRGB_ALPHA_BPTC_UNORM, // BC7 sRGB
};

//! Sampler minification filter
enum class SamplerMinFilter : uint8_t {
    // don't change the enums values
    NEAREST = 0, //!< No filtering. Nearest neighbor is used.
    LINEAR = 1, //!< Box filtering. Weighted average of 4 neighbors is used.
    NEAREST_MIPMAP_NEAREST = 2, //!< Mip-mapping is activated. But no filtering occurs.
    LINEAR_MIPMAP_NEAREST = 3, //!< Box filtering within a mip-map level.
    NEAREST_MIPMAP_LINEAR = 4, //!< Mip-map levels are interpolated, but no other filtering occurs.
    LINEAR_MIPMAP_LINEAR = 5 //!< Both interpolated Mip-mapping and linear filtering are used.
};

//! Sampler magnification filter
enum class SamplerMagFilter : uint8_t {
    // don't change the enums values
    NEAREST = 0, //!< No filtering. Nearest neighbor is used.
    LINEAR = 1, //!< Box filtering. Weighted average of 4 neighbors is used.
};

//! Pixel Data Format
enum class PixelDataFormat : uint8_t {
    R,                  //!< One Red channel, float
    R_INTEGER,          //!< One Red channel, integer
    RG,                 //!< Two Red and Green channels, float
    RG_INTEGER,         //!< Two Red and Green channels, integer
    RGB,                //!< Three Red, Green and Blue channels, float
    RGB_INTEGER,        //!< Three Red, Green and Blue channels, integer
    RGBA,               //!< Four Red, Green, Blue and Alpha channels, float
    RGBA_INTEGER,       //!< Four Red, Green, Blue and Alpha channels, integer
    UNUSED,             // used to be rgbm
    DEPTH_COMPONENT,    //!< Depth, 16-bit or 24-bits usually
    DEPTH_STENCIL,      //!< Two Depth (24-bits) + Stencil (8-bits) channels
    ALPHA,               //! One Alpha channel, float
    COMPRESSED_RGB8_ETC2,
    COMPRESSED_RGBA8_ETC2_EAC
};

//! Pixel Data Type
enum class PixelDataType : uint8_t {
    UBYTE,                //!< unsigned byte
    BYTE,                 //!< signed byte
    USHORT,               //!< unsigned short (16-bit)
    SHORT,                //!< signed short (16-bit)
    UINT,                 //!< unsigned int (32-bit)
    INT,                  //!< signed int (32-bit)
    HALF,                 //!< half-float (16-bit float)
    FLOAT,                //!< float (32-bits float)
    COMPRESSED,           //!< compressed pixels, @see CompressedPixelDataType
    UINT_10F_11F_11F_REV, //!< three low precision floating-point numbers
    USHORT_565,           //!< unsigned int (16-bit), encodes 3 RGB channels
    UINT_2_10_10_10_REV,  //!< unsigned normalized 10 bits RGB, 2 bits alpha
};

// 顶点属性类型枚举
enum class VertexAttributeType {
    Position,  // Vector3
    Color,     // Vector4
    UV,        // Vector2
    Normal,    // Vector3
    Tangent,    // Vector4
    BatchID     // float
};

// 顶点属性描述
struct VertexAttribute {
    VertexAttributeType type;
    size_t offset;
    size_t size;
};

/**
 * Face culling mode
 */
enum class CullFaceMode : uint8_t {
    NONE,           //!< No culling
    FRONT,          //!< Cull front faces
    BACK,           //!< Cull back faces (default GL behavior)
    FRONT_AND_BACK  //!< Cull both faces
};

/**
 * Backend-independent blend factors.
 */
enum class BlendFactor : uint8_t {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA
};

/**
 * Primitive types
 */
enum class PrimitiveType : uint8_t {
    // don't change the enums values (made to match GL)
    POINTS         = 0,    //!< points
    LINES          = 1,    //!< lines
    LINE_STRIP     = 3,    //!< line strip
    TRIANGLES      = 4,    //!< triangles
    TRIANGLE_STRIP = 5     //!< triangle strip
};
#endif //DRIVERENUMS_H
