/**
 * @file renderer.hpp
 * @brief Complete OpenGL ES 1.1 fixed-function renderer abstraction & telemetry.
 *
 * Isolates all GL types and headers from user code while providing 1:1 coverage
 * of OpenGL ES 1.1 (camera, matrices, Fixed16, depth, stencil, scissor, alpha test,
 * clip planes, polygon offset, fog, multi-texturing, lighting, materials,
 * interleaved VBOs, raw arrays, and 2D HUD/sprite rendering).
 */

#ifndef ZSDK_GRAPHICS_RENDERER_HPP
#define ZSDK_GRAPHICS_RENDERER_HPP

#include "zsdk/core/types.hpp"
#include "zsdk/core/math_fixed.hpp"
#include "zsdk/graphics/renderer_types.hpp"

namespace zsdk {
namespace graphics {

class MeshBuffer;
class Texture;

/**
 * @brief Thin, zero-overhead OpenGL ES 1.1 command wrapper.
 */
class Renderer {
public:
    /* Viewport, Scissor & Clearing */
    static void ResetStateCache();
    static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
    static void SetScissor(bool enabled, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);
    static void Clear(const Color& color, bool clearDepth = true, bool clearStencil = false);

    /* Blending, Depth, Alpha Test, Culling, Stencil, Color Mask, Polygon Offset & Clip Planes */
    static void SetBlendMode(BlendMode mode);
    static void SetDepthTest(bool enabled);
    static void SetDepthFunc(CompareFunc func);
    static void SetDepthMask(bool writeEnabled);
    static void SetAlphaTest(bool enabled, CompareFunc func = CompareFunc::Greater, float ref = 0.5f);
    static void SetCullFace(bool enabled);
    static void SetFrontFace(FrontFace face);
    static void SetStencilTest(bool enabled);
    static void SetStencilFunc(CompareFunc func, int32_t ref, uint32_t mask);
    static void SetStencilOp(StencilOp fail, StencilOp zfail, StencilOp zpass);
    static void SetColorMask(bool r, bool g, bool b, bool a);
    static void SetPolygonOffset(bool enabled, float factor = 0.0f, float units = 0.0f);
    static void SetClipPlane(uint8_t planeIndex, bool enabled, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f);

    /* Projection, Camera & ModelView Matrix Stack */
    static void SetOrtho(float left, float right, float bottom, float top, float nearZ = -1.0f, float farZ = 1.0f);
    static void SetFrustum(float left, float right, float bottom, float top, float nearZ, float farZ);
    static void SetPerspective(float fovYDegrees, float aspect, float nearZ, float farZ);
    static void ResetModelView();
    static void LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    static void PushMatrix();
    static void PopMatrix();
    static void LoadMatrix(const float* mat4x4ColumnMajor);
    static void Translate(float x, float y, float z);
    static void Rotate(float angleDegrees, float x, float y, float z);
    static void Scale(float x, float y, float z);

    /* Fixed-Point 16.16 Matrix Fast-Path (ARM11) */
    static void TranslateFx(Fixed16 x, Fixed16 y, Fixed16 z);
    static void RotateFx(Fixed16 angleDeg, Fixed16 x, Fixed16 y, Fixed16 z);
    static void ScaleFx(Fixed16 x, Fixed16 y, Fixed16 z);

    /* Fog */
    static void SetFog(bool enabled);
    static void ConfigureFog(FogMode mode, const Color& color, float start, float end, float density = 1.0f);

    /* Fixed-function OpenGL ES 1.x Lighting & Materials */
    static void SetShadeModel(ShadeModel model);
    static void SetNormalizeNormals(bool enabled);
    static void SetGlobalAmbientLight(const Color& color);
    static void SetLighting(bool enabled);
    static void SetLightEnabled(uint8_t lightIndex, bool enabled);
    static void ConfigureLight(uint8_t lightIndex, const Vec3& position,
                               const Color& ambient, const Color& diffuse, const Color& specular);
    static void ConfigureLightEx(uint8_t lightIndex, const LightDesc& desc);
    static void SetMaterial(const Color& ambient, const Color& diffuse, const Color& specular, float shininess);

    /* Multi-Texturing, Texture Matrix, Color & Primitive Sizing */
    static void SetTexture2DEnabled(bool enabled);
    static void SetActiveTextureUnit(uint8_t unit);
    static void SetTextureEnvMode(TexEnvMode mode);
    static void ResetTextureMatrix();
    static void TranslateTexture(float u, float v);
    static void ScaleTexture(float su, float sv);
    static void SetColor(const Color& color);
    static void SetLineWidth(float width);
    static void SetPointSize(float size);

    /* VBO, Interleaved & Raw Vertex Attribute Binding */
    static void BindPositionBuffer(const MeshBuffer& vbo, uint8_t components = 3);
    static void BindNormalBuffer(const MeshBuffer& vbo);
    static void BindColorBuffer(const MeshBuffer& vbo, uint8_t components = 4);
    static void BindTexCoordBuffer(const MeshBuffer& vbo, uint8_t components = 2);
    static void BindInterleavedBuffer(const MeshBuffer& vbo, const InterleavedLayout& layout);
    static void BindRawVertexArrays(const void* positions, uint8_t posComponents,
                                    const void* normals, const void* colors,
                                    const void* texCoords, int32_t strideBytes);
    static void UnbindVertexAttributes();

    static void DrawArrays(PrimitiveType type, int32_t firstVertex, int32_t vertexCount);
    static void DrawElementsU16(PrimitiveType type, int32_t indexCount, const uint16_t* indices);

    /* Lightweight 2D HUD / Sprite / Menu helpers (zero external font assets) */
    static void Begin2D(int32_t viewportWidth, int32_t viewportHeight);
    static void Begin2DVirtual(float virtualWidth, float virtualHeight, int32_t realWidth, int32_t realHeight);
    static void End2D();
    static void DrawRect2D(float x, float y, float width, float height, const Color& color);
    static void DrawText2D(float x, float y, float scale, const char* text, const Color& color);
    static void DrawTexturedQuad2D(const Texture& tex, float x, float y, float width, float height,
                                   float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f,
                                   const Color& tint = Color::Rgba(255, 255, 255, 255));

    /* 3D Billboards, Blob Shadows & Shared 8 KB Font Atlas */
    static void DrawBillboard3D(const Texture& tex, const Vec3& worldPos,
                                float width, float height,
                                const Vec3& cameraRight, const Vec3& cameraUp,
                                float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f,
                                const Color& tint = Color::Rgba(255, 255, 255, 255));
    static void DrawBlobShadow(const Vec3& groundPos, float radius,
                               const Color& color = Color::Rgba(0, 0, 0, 110));
    static uint32_t GetSharedFontTextureId();
    static void     ReleaseSharedFontTexture();
};

} // namespace graphics
} // namespace zsdk

#endif /* ZSDK_GRAPHICS_RENDERER_HPP */
