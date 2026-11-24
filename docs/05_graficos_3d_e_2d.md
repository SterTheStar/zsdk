# 05 — Gráficos 3D (OpenGL ES 1.1) e 2D

Headers incluídos: `graphics/gl_context.hpp`, `graphics/renderer.hpp`, `graphics/renderer_types.hpp`, `graphics/vertex_formats.hpp`, `graphics/texture.hpp`, `graphics/mesh_buffer.hpp`, `graphics/sprite_batch.hpp`, `graphics/frustum.hpp`, `graphics/display2d.hpp`, `graphics/image_decoder.hpp`, `graphics/sprite2d.hpp`.

---

## 1. Contexto EGL / WGL (`GLContext`)

Gerencia a criação da superfície OpenGL ES 1.1 no Zeebo (`EGL_Init`, `GLES_Init`, `IDisplay`/`IDIB`, `eglCreateWindowSurface`, `eglCreateContext`) e WGL no Windows:

```cpp
zsdk::graphics::GLContextConfig cfg = {5, 6, 5, 0, 16, 0, 1}; // RGB565, Depth 16, VSync ON
m_gl.Initialize(app->GetNativePlatformContext(), cfg);
// ... no OnRender():
m_gl.SwapBuffers(); // Renderiza DebugOverlay automaticamente antes do swap!
```

* **`SetSwapInterval(0 ou 1)`:** Controla VSync (`IEGLOESSwapInterval` no Zeebo / `wglSwapIntervalEXT` no Windows).
* **`SetSurfaceScale` / `SetSurfaceRotate` / `GetPowerLevel`:** Extensões Qualcomm Q3Dimension (`IEGLSurfaceManip` / `IEGLGetPowerLevel`). Conforme o Developer Guide 8.3, `SetSurfaceScale`/`EnableSurfaceScale`/`SetSurfaceRotate` desvinculam temporariamente a superfície (`eglMakeCurrent(..., EGL_NO_SURFACE, ...)`) antes de aplicar a escala.

---

## 2. Pipeline `Renderer` Completo (OpenGL ES 1.1)

O `Renderer` isola totalmente `<gles/gles_1_1/gl.h>` e `<GL/gl.h>` do código do jogo:

* **Viewport, Scissor & Clear:** `SetViewport`, `SetScissor`, `Clear(color, clearDepth, clearStencil)`.
* **Estados de Rasterização & State Cache:** `SetBlendMode` (`Opaque`, `Alpha`, `Additive`), `SetBlendEquation` (`Add`, `Subtract`, `ReverseSubtract`, `Min`, `Max`), `SetDither`, `SetPerspectiveCorrectionFast`, `SetDepthTest`, `SetDepthFunc`, `SetDepthMask`, `SetAlphaTest`, `SetCullFace`, `SetFrontFace`, `SetStencilTest`, `SetStencilFunc`, `SetStencilOp`, `SetColorMask`, `SetPolygonOffset` (`SetClipPlane` é *no-op* no Zeebo, pois User Clip Planes não são suportados na Tabela 7-1).
  * `SetBlendMode`, `SetDepthTest`, `SetCullFace` e `SetTexture2DEnabled` usam cache de estado interno para evitar chamadas redundantes ao driver OpenGL ES. Use `ResetStateCache()` caso código externo altere estados GL diretamente (o `Begin2D`/`End2D` e `DebugOverlay::Render` já fazem isso automaticamente).
* **Câmera, Matrizes & Ponto Fixo:**
  * `SetOrtho`, `SetFrustum`, `SetPerspective(fovY, aspect, nearZ, farZ)`
  * `ResetModelView`, `LookAt(eye, target, up)`, `PushMatrix`, `PopMatrix` (avisa se `MODELVIEW` ultrapassar 16 níveis), `LoadMatrix`, `MultMatrix`, `Translate`, `Rotate`, `Scale`
  * Fast-path ARM11: `TranslateFx`, `RotateFx`, `ScaleFx` (`Fixed16`)
* **Iluminação & Materiais:**
  * `SetShadeModel`, `SetNormalizeNormals`, `SetGlobalAmbientLight`, `SetLighting`, `SetLightEnabled` (emite aviso ao habilitar mais de 1 luz simultânea, pois iluminação no Zeebo roda na CPU)
  * `ConfigureLight(...)` e `ConfigureLightEx(index, LightDesc)` (suporta `LightType::Directional`, `Point`, `Spot` com cutoff/expoente e atenuação constante/linear/quadrática)
  * `SetMaterial(ambient, diffuse, specular, shininess)`
* **Fog & Multi-Texturing:**
  * `SetFog`, `ConfigureFog(FogMode, color, start, end, density)`
  * `SetTexture2DEnabled`, `SetActiveTextureUnit(0/1)`, `SetTextureEnvMode(Modulate/Replace/Decal/Add/Combine/Dot3)`
  * `SetPointSpritesEnabled(bool)` & `BindPointSizeBuffer(vbo)` (`OES_point_sprite` / `OES_point_size_array`)
  * Hardware Skinning (`OES_matrix_palette`): `SetMatrixPaletteEnabled`, `LoadPaletteMatrixFromModelView`, `BindSkinningBuffers`, `UnbindSkinningBuffers`
  * `ResetTextureMatrix`, `TranslateTexture`, `ScaleTexture` (animação de UVs na GPU)

### VBOs Separados, Interleaved e Arrays Dinâmicos

```cpp
// Opção A: VBOs interleaved (recomendado para cache do ARM11)
zsdk::graphics::MeshBuffer vbo;
vbo.Create(zsdk::graphics::BufferType::Vertex, zsdk::graphics::BufferUsage::Static,
           sizeof(verts), verts);
zsdk::graphics::Renderer::BindInterleavedBuffer(vbo, zsdk::graphics::VertexP3N3U2::Layout());
zsdk::graphics::Renderer::DrawArrays(zsdk::graphics::PrimitiveType::Triangles, 0, count);
zsdk::graphics::Renderer::UnbindVertexAttributes();
```

Formatos prontos em `vertex_formats.hpp`: `VertexP3N3C4` (28B), `VertexP3N3U2` (32B), `VertexP3C4U2` (24B).

---

## 3. Texturas (`Texture`) e Buffers (`MeshBuffer`) com Tracking de VRAM

* **`Texture`:** `Create(w, h, format, pixels, filter)` (`Alpha8`, `RGB565`, `RGBA4444`, `RGBA5551`, `RGB888`, `RGBA8888`), `UpdateSubImage`, `SetWrap(ClampToEdge/Repeat)`, `CreateCompressed`, `CreateFromAtitc` (remove o header de 20 bytes `ATITC_HEADER` automaticamente), `GenerateMipmaps` (usa `GL_LINEAR_MIPMAP_NEAREST` acelerado em hardware em vez de trilinear lento), `CopyFromFramebuffer` (`glCopyTexImage2D`), `ReadFramebuffer` (`glReadPixels`).
  * Validação Zeebo: `IsPowerOfTwo()` / `NextPowerOfTwo()`, com aviso automático (`ZSDK_LOG_WARN`) caso a textura não seja potência de 2 ou ultrapasse `1024x1024` (Seção 7.1.3 do Developer Guide).
* Contabiliza automaticamente toda alocação/liberação de VRAM no `MemoryTracker`.

---

## 4. HUD 2D, Decodificação de Imagens e Sprites Nativos

* **2D via OpenGL ES (`Renderer::Begin2D` / `Begin2DVirtual` / `End2D`):**
  ```cpp
  // Ou use Begin2DVirtual(640.0f, 480.0f, realW, realH) + GetSafeAreaRect(640, 480, 8) para TV CRT!
  Renderer::Begin2D(w, h);
  Renderer::DrawRect2D(10, 10, 200, 40, Color::Rgba(0, 0, 0, 180));
  Renderer::DrawTexturedQuad2D(minhaTex, 20, 60, 64, 64);
  Renderer::DrawText2D(20, 20, 1.0f, "ZSDK HUD", Color::Rgba(255, 255, 255, 255));
  Renderer::End2D();
  ```
  * A fonte embutida 8x8 (`128x64` `GL_ALPHA`, 8 KB VRAM) é compartilhada entre `Renderer::DrawText2D` e `DebugOverlay` via `GetSharedFontTextureId()` / `ReleaseSharedFontTexture()`.
* **`SpriteBatch2D` (`graphics/sprite_batch.hpp`) — Otimizado para 32 MB RAM:**
  * Aloca um buffer fixo e único em `Initialize(maxSprites = 128)` (~13,5 KB via `MemoryTracker::Alloc`), sem alocações por frame.
  * Agrupa `DrawQuad`, `DrawRotated` e `DrawRect` de mesma textura em uma única chamada `glDrawElements` no `Flush()` / `End()`.
  ```cpp
  SpriteBatch2D batch;
  batch.Initialize(128);
  batch.BeginVirtual(640.0f, 480.0f, w, h);
  batch.DrawQuad(&atlasTex, 100.0f, 120.0f, 32.0f, 32.0f);
  batch.DrawRotated(&atlasTex, 200.0f, 150.0f, 32.0f, 32.0f, angleRad);
  batch.End();
  ```
* **Frustum Culling 3D (`Frustum` em `graphics/frustum.hpp`), Billboards & Blob Shadows:**
  * `Frustum` ocupa apenas 96 bytes na stack (zero heap): `ExtractFromViewProj(vp16)` ou `ExtractFromCamera(eye, target, up, fovY, aspect, nearZ, farZ)`, e testes `TestSphere(center, radius)` / `TestAabb(minPt, maxPt)`.
  * `Renderer::DrawBillboard3D(tex, worldPos, w, h, camRight, camUp, ...)`: quad 3D alinhado à câmera.
  * `Renderer::DrawBlobShadow(groundPos, radius, color)`: sombra circular suave em `GL_TRIANGLE_FAN` (8 segmentos, sem textura extra).
* **`ImageDecoder` (`graphics/image_decoder.hpp`):** Decodifica PNG, JPEG e BMP nativos via `IImageDecoder`/`IForceFeed` (com fallback `IImage`) em buffers de pixels rastreados.
* **`Display2D`, `Bitmap2D`, `SpriteEngine2D`, `Font2D`, `HardwareBlitter2D`:** Wrappers diretos para `IDisplay`, `IBitmap`, `ISprite`, `IFont` e `GL_OES_draw_texture` (`glDrawTexfOES`).
