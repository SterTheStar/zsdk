/**
 * @file renderer_types.hpp
 * @brief Shared enums and descriptor structs for zsdk::graphics::Renderer.
 *
 * Isolates all OpenGL ES 1.1 state enums, lighting descriptors, and interleaved
 * vertex layout metadata from GL headers.
 */

#ifndef ZSDK_GRAPHICS_RENDERER_TYPES_HPP
#define ZSDK_GRAPHICS_RENDERER_TYPES_HPP

#include "zsdk/core/types.hpp"

namespace zsdk {
namespace graphics {

/** @brief Geometric primitive topology for draw calls. */
enum class PrimitiveType : uint8_t {
    Triangles = 0,
    TriangleStrip,
    Lines,
    Points
};

/** @brief Framebuffer alpha blending presets. */
enum class BlendMode : uint8_t {
    Opaque = 0,
    Alpha,
    Additive
};

/** @brief Comparison function used by depth, alpha, and stencil tests. */
enum class CompareFunc : uint8_t {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/** @brief Fixed-function fog attenuation curve. */
enum class FogMode : uint8_t {
    Linear = 0,
    Exp,
    Exp2
};

/** @brief Polygon shading interpolation mode. */
enum class ShadeModel : uint8_t {
    Smooth = 0,
    Flat
};

/** @brief Stencil buffer update operation. */
enum class StencilOp : uint8_t {
    Keep = 0,
    Zero,
    Replace,
    Incr,
    Decr,
    Invert
};

/** @brief Winding order defining front-facing triangles. */
enum class FrontFace : uint8_t {
    CounterClockwise = 0,
    Clockwise
};

/** @brief Fixed-function texture environment combine mode. */
enum class TexEnvMode : uint8_t {
    Modulate = 0,
    Replace,
    Decal,
    Add,
    Combine,
    Dot3
};

/** @brief Texture addressing wrap modes. */
enum class TextureWrap : uint8_t {
    ClampToEdge = 0,
    Repeat
};

/** @brief Blend equation modes (Appendix B: BlendEquationEXT). */
enum class BlendEquationMode : uint8_t {
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

/** @brief Fixed-function light source classification. */
enum class LightType : uint8_t {
    Directional = 0,
    Point,
    Spot
};

/**
 * @brief Full fixed-function OpenGL ES 1.1 light source descriptor.
 */
struct LightDesc {
    LightType type;
    Vec3 position;       /**< World/eye-space position (Point/Spot) or direction vector (Directional). */
    Vec3 direction;      /**< Spotlight cone axis (Spot only). */
    Color ambient;       /**< Ambient RGBA contribution. */
    Color diffuse;       /**< Diffuse RGBA contribution. */
    Color specular;      /**< Specular RGBA contribution. */
    float spotCutoffDeg; /**< Spotlight half-cone angle in degrees [0..90] or 180 for omnidirectional. */
    float spotExponent;  /**< Spotlight falloff exponent [0..128]. */
    float constAtten;    /**< Constant attenuation factor. */
    float linearAtten;   /**< Linear attenuation factor. */
    float quadAtten;     /**< Quadratic attenuation factor. */
};

/**
 * @brief Describes an interleaved vertex buffer layout.
 *
 * Attribute offsets are in bytes from the start of each vertex; a negative
 * offset indicates that the attribute is disabled.
 */
struct InterleavedLayout {
    int32_t strideBytes;     /**< Total byte stride between consecutive vertices. */
    int32_t posOffset;       /**< Byte offset of position attribute (< 0 disables). */
    uint8_t posComponents;   /**< Number of float components for position (2 or 3). */
    int32_t normalOffset;    /**< Byte offset of 3-float normal attribute (< 0 disables). */
    int32_t colorOffset;     /**< Byte offset of RGBA8 color attribute (< 0 disables). */
    uint8_t colorComponents; /**< Number of uint8_t components for color (typically 4). */
    int32_t uvOffset;        /**< Byte offset of texture coordinate attribute (< 0 disables). */
    uint8_t uvComponents;    /**< Number of float components for UV (typically 2). */
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_RENDERER_TYPES_HPP */
