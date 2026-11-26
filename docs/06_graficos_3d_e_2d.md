# 06 Gráficos 3D e 2D

O ZSDK entrega a GPU do Zeebo por um conjunto pequeno de classes: `GLContext` abre a superfície, `Renderer` traduz estados e transformações de OpenGL ES 1.1, `Texture` e `MeshBuffer` sobem dados com contabilidade de VRAM, e `SpriteBatch2D` desenha o HUD em lote. O problema que isso resolve é o de uma GPU de função fixa, sem shaders: todo o pipeline é estado global, e qualquer chamada redundante custa no ARM11 a 528 MHz. Use este capítulo quando precisar desenhar em 3D, montar um HUD ou subir assets de imagem; para o ciclo de vida do app e o laço de frame, veja [03 Core e assíncrono](./03_core_e_async.md). O capítulo não cobre áudio nem entrada, e o backend Windows não substitui teste no console: nada em gráficos foi validado em hardware Zeebo, apenas no emulador, como registra o [TODO.md](../TODO.md).

## Visão rápida

| Classe | Papel | Plataforma |
| :--- | :--- | :--- |
| `GLContext` | cria e destrói a superfície e o contexto (EGL 1.x no Zeebo, WGL no Windows) | ambas |
| `Renderer` | comandos estáticos de estado fixo, matrizes, luz e draw calls | ambas |
| `Frame` | RAII de um frame: viewport, clear, passe 2D e `SwapBuffers` no destrutor | ambas |
| `Texture` | textura 2D com contabilidade automática de VRAM | ambas |
| `MeshBuffer` | VBO/IBO com fallback para RAM do cliente e contabilidade de VRAM | ambas |
| `SpriteBatch2D` | acumula quads e retângulos e submete em um único `glDrawElements` | ambas |
| `Frustum` | seis planos na stack para descartar esferas e AABBs antes do draw | ambas |
| `ImageDecoder` | decodifica PNG, JPEG e BMP em buffer de pixels | só Zeebo |
| `Display2D` | `IDisplay` cru: clear, retângulo e texto sem OpenGL | Zeebo (stub no Windows) |
| `Bitmap2D`, `SpriteEngine2D`, `Font2D` | `IBitmap`, `ISprite` e `IFont` do BREW | só Zeebo |
| `HardwareBlitter2D` | blit 2D por `glDrawTexfOES` (`GL_OES_draw_texture`) | só Zeebo |

A divisão importa: as sete primeiras linhas são o caminho portável e é nele que o jogo deve viver. As classes de `IBitmap`/`ISprite`/`IFont` reproduzem a API 2D do BREW e não existem no backend Windows, que é um stub por área e não um porte completo.

## Pré-requisitos

- Uma `Application` já inicializada. Toda classe de gráfico recebe o contexto de plataforma opaco em `Initialize`, obtido com `Application::GetInstance()->GetNativePlatformContext()`.
- `#include "zsdk/zsdk.hpp"` inclui os 12 headers de `graphics/`. Nenhum arquivo de jogo inclui `<GL/gl.h>`, `<gles/egl.h>` ou `AEEShell.h`.
- Uma decisão sobre a resolução de render. O display do Zeebo é 640x480 por TV-Out, e o exemplo `01_spinning_cube` fixa 30 FPS no console por causa do refresh da saída de vídeo.
- Nada de `new` nem de alocação por frame no caminho de desenho. Os buffers de geometria e de sprite são criados uma vez e reutilizados.

## Conceitos

### Uma GPU de função fixa é uma máquina de estados

Sem shaders programáveis, não existe uniform para enviar câmera ou cor: a matriz de projeção entra por `glFrustumf`, a de visão por `glLoadMatrixf` e a posição de cada objeto por `glTranslatef`, `glRotatef` e `glScalef` sobre a pilha de `MODELVIEW`. `Renderer` encapsula exatamente esse conjunto, e nada além dele: os enums de `renderer_types.hpp` (`PrimitiveType`, `BlendMode`, `CompareFunc`, `FogMode`, `ShadeModel`, `StencilOp`, `FrontFace`, `TexEnvMode`, `TextureWrap`, `BlendEquationMode`, `LightType`) existem para que o jogo não precise incluir header de GL só para nomear `GL_LEQUAL`.

Consequência prática: a ordem das chamadas é o programa. Ligar iluminação, configurar a luz, desenhar, desligar, é uma sequência com efeito, e não uma configuração declarativa. O `Renderer` não guarda um grafo de cena nem reordena nada.

Para o caminho de matrizes existe uma variante em ponto fixo 16.16, que é o formato nativo do `glTranslatex` do GLES e evita conversão de float no ARM11:

```cpp
const zsdk::Fixed16 kNinety   = zsdk::FloatToFixed(90.0f);
const zsdk::Fixed16 kHalfTurn = zsdk::FloatToFixed(180.0f);

zsdk::graphics::Renderer::PushMatrix();
zsdk::graphics::Renderer::TranslateFx(zsdk::FloatToFixed(3.0f), 0, 0);
zsdk::graphics::Renderer::RotateFx(kHalfTurn, 0, zsdk::kFixedOne, 0);
zsdk::graphics::Renderer::RotateFx(kNinety, zsdk::kFixedOne, 0, 0);
zsdk::graphics::Renderer::ScaleFx(zsdk::kFixedOne, zsdk::kFixedOne, zsdk::kFixedOne);
zsdk::graphics::Renderer::DrawElementsU16(zsdk::graphics::PrimitiveType::Triangles, 6, nullptr);
zsdk::graphics::Renderer::PopMatrix();
```

Só `TranslateFx`, `RotateFx` e `ScaleFx` têm versão em `Fixed16`. Projeção, `LookAt`, iluminação e neblina continuam em float, e a ajuda para calcular seno e cosseno inteiros está em `math_fixed.hpp` (`FixedSinDeg`, `FixedCosDeg`, `FixedMul`, `FixedDiv`).

### O state cache do Renderer

Trocar estado de GPU no Q3Dimension é caro, e um laço de jogo típico repete as mesmas chamadas centenas de vezes por frame. `Renderer` mantém um cache de módulo em `renderer.cpp` com quatro booleanos e o viewport:

| Campo | Setter que consulta o cache |
| :--- | :--- |
| `blendMode` | `SetBlendMode` |
| `depthTest` | `SetDepthTest` |
| `cullFace` | `SetCullFace` |
| `texture2D` | `SetTexture2DEnabled` |
| viewport (`x`, `y`, `w`, `h`) | `SetViewport` |

Esses cinco setters comparam o valor pedido com o último enviado e retornam sem chamar o GL quando não mudou. Todo o resto passa direto: `SetDepthFunc`, `SetAlphaTest`, `SetStencilFunc`, `SetStencilOp`, `SetColorMask`, `SetBlendEquation`, `SetPolygonOffset`, `SetFog` e todas as matrizes vão ao driver em toda chamada. A recomendação que sai disso é agrupar o desenho por estado: primeiro tudo opaco, depois tudo com alpha; primeiro o que usa textura, depois o que não usa.

O cache é otimista, não onisciente. Ele acredita que ninguém mexeu no estado por fora do `Renderer`. Quando código externo chama GL cru (`DebugOverlay`, `SpriteBatch2D::Flush`, os helpers de 2D do próprio `Renderer`), o cache fica mentindo e o remédio é `ResetStateCache()`, que zera os cinco campos e invalida o viewport. `Begin2D`, `Begin2DVirtual`, `End2D` e o fim de `DebugOverlay::Render` já fazem isso internamente, e é por isso que um `SetDepthTest(true)` depois de um passe 2D sempre chega ao driver.

### O frame e o passe 2D

Um frame no ZSDK tem quatro fases: viewport, clear, desenho e apresentação. `Frame` agrupa as quatro em um escopo, o que garante o swap mesmo quando `OnRender` retorna cedo:

```cpp
void SceneApp::OnRender() {
    zsdk::graphics::Frame frame(m_gl, zsdk::Color::Rgba(16, 20, 32, 255));
    frame.Begin2D(m_batch, 640.0f, 480.0f);
    m_batch.DrawRect(16.0f, 16.0f, 220.0f, 28.0f, zsdk::Color::Rgba(10, 14, 22, 200));
    m_batch.DrawRect(16.0f, 16.0f, 220.0f, 2.0f, zsdk::Color::Rgba(90, 160, 240, 255));
    m_batch.DrawRect(16.0f, 44.0f, 220.0f, 44.0f, zsdk::Color::Rgba(10, 14, 22, 160));
    m_batch.Flush();
    zsdk::graphics::Renderer::DrawText2D(24.0f, 22.0f, 1.0f, "READY",
                                         zsdk::Color::Rgba(235, 245, 255, 255));
}
```

O construtor ajusta o viewport para `GetRenderWidth()`/`GetRenderHeight()` e limpa cor e profundidade. `Begin2D` desliga `GL_DEPTH_TEST` e abre a passagem em coordenadas virtuais; o destrutor fecha o passe e chama `SwapBuffers`. Como o swap já acontece ali, não chame `SwapBuffers` de novo no mesmo frame: dois swaps com VSync ligado dobram a espera e cortam o frame rate pela metade.

`Renderer::Begin2D(w, h)` monta uma projeção ortográfica com origem no canto superior esquerdo e `y` crescendo para baixo, que é o sistema de coordenadas de HUD e de menu. `Begin2DVirtual(vw, vh, realW, realH)` faz o mesmo com uma resolução virtual (640x480, por exemplo) e ajusta o viewport por letterbox, preservando a proporção. Nos dois casos as matrizes de projeção e de modelview são empilhadas e desempilhadas em `End2D`, e o teste de profundidade não é religado: quem quiser 3D depois do HUD precisa chamar `SetDepthTest(true)` e reconfigurar a projeção.

`GetSafeAreaRect(w, h, marginPx = 8)` devolve o retângulo seguro para TV CRT, com a margem de 4 a 8 px que o Developer Guide pede em todas as bordas.

### Vértice interleaved, malha e textura

`vertex_formats.hpp` define três structs com `static_assert` de tamanho e um método `InterleavedLayout()` que devolve offsets em bytes:

| Struct | Conteúdo | Tamanho |
| :--- | :--- | :--- |
| `VertexP3N3C4` | posição 3 floats, normal 3 floats, cor RGBA8 | 28 B |
| `VertexP3N3U2` | posição 3 floats, normal 3 floats, UV 2 floats | 32 B |
| `VertexP3C4U2` | posição 3 floats, cor RGBA8, UV 2 floats | 24 B |

`InterleavedLayout` existe como tipo próprio (`renderer_types.hpp`) com `strideBytes`, `posOffset`, `posComponents`, `normalOffset`, `colorOffset`, `colorComponents`, `uvOffset` e `uvComponents`. Offset negativo desliga o atributo, e é assim que `BindInterleavedBuffer` sabe quais `glEnableClientState` emitir: um `VertexP3N3U2` desabilita a cor por vértice, um `VertexP3C4U2` desabilita a normal.

O layout interleaved serve a dois propósitos: uma única submissão de buffer em vez de quatro, e localidade de cache para o ARM11. Os índices são sempre `uint16_t`, o que limita uma malha a 65 536 vértices e mantém o IBO em 2 bytes por índice.

`Texture` cobre seis formatos, e a escolha define tanto a VRAM quanto a banda de leitura:

| Formato | Bytes por pixel | Observação |
| :--- | :--- | :--- |
| `Alpha8` | 1 | `GL_ALPHA`, útil só como máscara |
| `RGB565` | 2 | o formato natural do framebuffer do console |
| `RGBA4444` | 2 | 4 bits por canal, com alpha |
| `RGBA5551` | 2 | 1 bit de alpha, bom para recorte com alpha test |
| `RGB888` | 3 | uploads precisam de cuidado com alinhamento |
| `RGBA8888` | 4 | o mais caro e o mais simples de gerar |

No Zeebo a textura precisa ser potência de 2 e ter no máximo 1024x1024. Não é uma checagem que rejeita: `Create` emite `ZSDK_LOG_WARN` e tenta subir do mesmo jeito, então o aviso aparece no log e a textura funciona ou não no driver. `IsPowerOfTwo` e `NextPowerOfTwo` são estáticos justamente para você ajustar antes do upload.

### Assinatura / configuração

`GLContext` é a única classe que fala EGL ou WGL, e `GLContextConfig` é um agregado de sete `uint8_t` na ordem `redBits`, `greenBits`, `blueBits`, `alphaBits`, `depthBits`, `stencilBits`, `swapInterval`:

| Plataforma | O que `Initialize` faz |
| :--- | :--- |
| Zeebo | `EGL_Init`, `GLES_Init`, `AEECLSID_DISPLAY`, `IDISPLAY_GetDeviceBitmap`, `IDIB`, `eglGetDisplay`, `eglInitialize`, `eglChooseConfig`, `eglCreateWindowSurface` sobre o `IDIB`, `eglCreateContext`, `eglMakeCurrent` |
| Windows | `GetDC`, `ChoosePixelFormat` com `PFD_DOUBLEBUFFER`, `wglCreateContext`, `wglMakeCurrent`, dimensões do `GetClientRect` |

A ordem de destruição segue o Developer Guide 7.3.1: `Shutdown` chama `GLES_Release` antes de `eglDestroyContext` e `eglTerminate`, e só depois libera `IDIB`, `IBitmap`, `IDisplay` e `IEGL10`, porque o `IGLES11` precisa continuar válido durante o seu próprio release. Também é em `Shutdown` que a textura compartilhada da fonte 8x8 é liberada. Chamar `Shutdown` duas vezes é seguro; no Zeebo, `Initialize` com um contexto já aberto devolve `Result::AlreadyInitialized`.

## Uso passo a passo

Os blocos abaixo montam, na ordem, uma cena mínima completa: contexto, duas lajes em 3D com uma luz, uma textura gerada em memória e um HUD. Cada bloco continua o anterior no mesmo arquivo.

### Declarar a cena e os dados da malha

A cena é uma laje de 2x2 unidades no plano XZ, com normal para cima e cor branca, para a luz ser a única responsável pelo tom:

```cpp
#include "zsdk/zsdk.hpp"

static const zsdk::graphics::VertexP3N3C4 kSlabVertices[4] = {
    {-1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 200, 205, 210, 255},
    { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 200, 205, 210, 255},
    { 1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 200, 205, 210, 255},
    {-1.0f, 0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 200, 205, 210, 255}
};
static const uint16_t kSlabIndices[6] = {0, 1, 2, 0, 2, 3};

class SceneApp : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override;
    void OnUpdate(uint32_t deltaTimeMs) override;
    void OnRender() override;
    void OnShutdown() override;

private:
    zsdk::graphics::GLContext     m_gl;
    zsdk::graphics::MeshBuffer    m_vbo;
    zsdk::graphics::MeshBuffer    m_ibo;
    zsdk::graphics::Texture       m_checker;
    zsdk::graphics::SpriteBatch2D m_batch;
    float m_angleDeg = 0.0f;
};
```

O giro é atualizado no `OnUpdate`, em graus por segundo, porque `Rotate` recebe graus e não radianos:

```cpp
void SceneApp::OnUpdate(uint32_t deltaTimeMs) {
    /* 18 graus por segundo, independente do frame rate */
    const float dt = static_cast<float>(deltaTimeMs) * 0.001f;
    m_angleDeg += 18.0f * dt;
    if (m_angleDeg >= 360.0f) {
        m_angleDeg -= 360.0f;
    }
    if (m_angleDeg < 0.0f) {
        m_angleDeg += 360.0f;
    }
}
```

### Abrir o contexto e subir a malha

`OnInit` abre o contexto e sobe os dois buffers. Um `Result` de erro interrompe a inicialização: no Zeebo ele vira `ZSDK_LOG_ERROR` e o app não sobe, no Windows a janela é destruída:

```cpp
zsdk::Result SceneApp::OnInit() {
    using namespace zsdk;
    auto* app = Application::GetInstance();
    graphics::GLContextConfig cfg = {5, 6, 5, 0, 16, 0, 1};

    Result res = m_gl.Initialize(app->GetNativePlatformContext(), cfg);
    if (Failed(res)) {
        return res;
    }
    res = m_vbo.Create(graphics::BufferType::Vertex, graphics::BufferUsage::Static,
                       sizeof(kSlabVertices), kSlabVertices);
    if (Failed(res)) {
        return res;
    }
    res = m_ibo.Create(graphics::BufferType::Index, graphics::BufferUsage::Static,
                       sizeof(kSlabIndices), kSlabIndices);
    if (Failed(res)) {
        return res;
    }
    res = CreateCheckerTexture(m_checker);
    if (Failed(res)) {
        return res;
    }
    return m_batch.Initialize(64u);
}
```

`MeshBuffer::Create` tenta o VBO de hardware e, se `glGenBuffers` falhar ou o `glBufferData` retornar erro, cai para um bloco de RAM do cliente (`GetRamData()` não nulo, `IsHardwareVbo()` falso). O desenho funciona nos dois casos, porque `BindInterleavedBuffer` usa `GetPointerAtOffset`, que devolve ponteiro de RAM quando não há VBO e offset em bytes quando há.

### Gerar a textura em memória

A textura abaixo é montada na memória, o que dispensa asset e funciona igual nas duas plataformas. A função fica no mesmo arquivo, antes de `OnInit`:

```cpp
static uint16_t Rgba4444(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return static_cast<uint16_t>(((r & 0xF0u) << 8) | ((g & 0xF0u) << 4) |
                                 (b & 0xF0u) | (a >> 4));
}

static zsdk::Result CreateCheckerTexture(zsdk::graphics::Texture& tex) {
    uint16_t pixels[8 * 8];
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const bool dark = ((x ^ y) & 1) != 0;
            pixels[y * 8 + x] = dark ? Rgba4444(40, 44, 60, 255)
                                     : Rgba4444(230, 235, 245, 255);
        }
    }
    return tex.Create(8, 8, zsdk::graphics::TextureFormat::RGBA4444, pixels);
}
```

O valor de 16 bits segue a ordem que `GL_UNSIGNED_SHORT_4_4_4_4` espera: R nos bits 12 a 15, G nos 8 a 11, B nos 4 a 7 e A nos 0 a 3. `Texture::Create` copia os pixels na hora, então o array da stack pode sair de escopo no fim da função.

### Câmera, clear e projeção

```cpp
void SceneApp::OnRender() {
    using zsdk::graphics::Renderer;
    const int32_t w = m_gl.GetRenderWidth();
    const int32_t h = m_gl.GetRenderHeight();
    const float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.333f;

    Renderer::SetViewport(0, 0, w, h);
    Renderer::Clear(zsdk::Color::Rgba(16, 20, 32, 255), true);
    Renderer::SetPerspective(55.0f, aspect, 0.5f, 120.0f);
    Renderer::ResetModelView();
    Renderer::LookAt(zsdk::Vec3{6.0f, 7.0f, 12.0f},
                     zsdk::Vec3{0.0f, 0.0f, 0.0f},
                     zsdk::Vec3{0.0f, 1.0f, 0.0f});
```

`SetPerspective` calcula o frustum a partir de `fovY`, `aspect`, `nearZ` e `farZ` e escreve em `GL_PROJECTION` com `glLoadIdentity` antes. `ResetModelView` carrega a identidade e zera o contador de profundidade da pilha. `LookAt` monta a matriz de visão e a carrega com `glLoadMatrixf`, sem empilhar: chamar `LookAt` de novo substitui a visão em vez de acumular.

### Desenhar o 3D com matriz por objeto

```cpp
    Renderer::SetDepthTest(true);
    Renderer::SetCullFace(true);
    Renderer::SetLighting(true);
    Renderer::ConfigureLight(0, zsdk::Vec3{5.0f, 9.0f, 6.0f},
                             zsdk::Color::Rgba(70, 70, 85, 255),
                             zsdk::Color::Rgba(240, 240, 235, 255),
                             zsdk::Color::Rgba(200, 200, 205, 255));
    Renderer::BindInterleavedBuffer(m_vbo, zsdk::graphics::VertexP3N3C4::InterleavedLayout());
    m_ibo.Bind();

    const zsdk::Vec3 eye{6.0f, 7.0f, 12.0f};
    const zsdk::Vec3 center{0.0f, 0.0f, 0.0f};
    const zsdk::Vec3 up{0.0f, 1.0f, 0.0f};
    zsdk::graphics::Frustum frustum;
    frustum.ExtractFromCamera(eye, center, up, 55.0f, aspect, 0.5f, 120.0f);
    const zsdk::Vec3 slabs[2] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.5f, -6.0f}};
    for (int i = 0; i < 2; ++i) {
        if (!frustum.TestSphere(slabs[i], 2.0f)) {
            continue;
        }
        Renderer::PushMatrix();
        Renderer::Translate(slabs[i].x, slabs[i].y, slabs[i].z);
        Renderer::Rotate(m_angleDeg * (i == 0 ? 1.0f : -0.5f), 0.0f, 1.0f, 0.0f);
        Renderer::DrawElementsU16(zsdk::graphics::PrimitiveType::Triangles, 6, nullptr);
        Renderer::PopMatrix();
    }
    zsdk::graphics::MeshBuffer::Unbind(zsdk::graphics::BufferType::Index);
    Renderer::UnbindVertexAttributes();
    Renderer::SetLighting(false);
```

O `nullptr` em `DrawElementsU16` não é descuido: com um IBO ligado, o GL trata o ponteiro de índices como offset em bytes no buffer, e passar um endereço de RAM faz a chamada ler no lugar errado. `PushMatrix` avisa no log quando a pilha passa de 16 níveis, que é o limite do GLES, e o aviso aparece uma única vez por execução.

`Frustum::ExtractFromCamera` reaproveita os mesmos parâmetros da câmera em vez de exigir a matriz montada. Se você já tem a view-projection em memória, `ExtractFromViewProj(const float* vp16)` aceita os 16 floats em ordem de coluna. O objeto `Frustum` tem 96 bytes e vive na stack, sem alocação.

### O HUD em 2D

```cpp
    Renderer::SetDepthTest(false);
    m_batch.BeginVirtual(640.0f, 480.0f, w, h);
    m_batch.DrawRect(8.0f, 8.0f, 624.0f, 32.0f, zsdk::Color::Rgba(8, 10, 16, 200));
    m_batch.Flush();
    for (int i = 0; i < 4; ++i) {
        m_batch.DrawQuad(&m_checker, 12.0f + static_cast<float>(i) * 40.0f, 12.0f,
                         32.0f, 24.0f);
    }
    m_batch.Flush();
    Renderer::DrawText2D(180.0f, 14.0f, 1.0f, "ZSDK 05",
                         zsdk::Color::Rgba(235, 245, 255, 255));
    m_batch.End();
}
```

O primeiro `Flush` separa os retângulos sem textura dos quads texturizados: `DrawQuad` chama `Flush` sozinho quando a textura muda, mas deixar explícito evita depender desse detalhe. `Renderer::DrawText2D` usa o atlas 8x8 compartilhado de 128x64 pixels `GL_ALPHA` (8 KiB de VRAM) e emite um `glDrawArrays` a cada 16 caracteres.

### Fechar o frame e liberar

Em `OnRender` manual, o swap é a última chamada. Em `OnShutdown`, a ordem é a inversa da criação: primeiro o que depende do contexto, depois o contexto.

```cpp
    m_gl.SwapBuffers();
}

void SceneApp::OnShutdown() {
    m_batch.Shutdown();
    m_checker.Destroy();
    m_ibo.Destroy();
    m_vbo.Destroy();
    m_gl.Shutdown();
}
```

`SwapBuffers` renderiza o `DebugOverlay` antes de chamar `eglSwapBuffers` (ou `SwapBuffers` do Win32), então o overlay nunca fica por baixo do HUD do jogo. Para completar o app, `ZSDK_IMPLEMENT_APP(SceneApp, "ZSDK_Scene", 30, true)` define os pontos de entrada BREW e Win32.

## Referência da API

### Contexto e frame

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `GLContext::Initialize` | `Result Initialize(void* platformContext, const GLContextConfig& config)` | `IShell*` no Zeebo, `HWND` no Windows |
| `GLContext::Shutdown` | `void Shutdown()` | idempotente; libera fonte compartilhada e EGL |
| `GLContext::MakeCurrent` | `Result MakeCurrent()` | vincula o contexto à thread corrente |
| `GLContext::SwapBuffers` | `Result SwapBuffers()` | overlay primeiro, swap depois |
| `GLContext::SetSwapInterval` | `Result SetSwapInterval(int32_t interval)` | `1` liga VSync; `NotSupported` se a extensão faltar |
| `GLContext::SetRenderResolution` | `Result SetRenderResolution(int32_t renderWidth, int32_t renderHeight)` | Zeebo usa `IEGLSurfaceManip`; passar o tamanho nativo desliga a escala |
| `GLContext::SetSurfaceScale` | `Result SetSurfaceScale(const Rect& srcRect, const Rect& dstRect, bool enable = true)` | desvincula a superfície durante a troca; `NotSupported` no Windows |
| `GLContext::EnableSurfaceScale` | `Result EnableSurfaceScale(bool enable)` | só Zeebo |
| `GLContext::SetSurfaceRotate` | `Result SetSurfaceRotate(bool rot90, bool hMirror, bool vMirror, bool enable = true)` | só Zeebo |
| `GLContext::GetPowerLevel` | `Result GetPowerLevel(int32_t& outPowerLevel) const` | `IEGLGetPowerLevel`; no Windows devolve `Ok` com `0` |
| `GLContext::HasExtension` | `bool HasExtension(const char* name) const` | exige contexto corrente; compara token inteiro |
| `GLContext::GetWidth` / `GetHeight` | `int32_t` | tamanho nativo da superfície |
| `GLContext::GetRenderWidth` / `GetRenderHeight` | `int32_t` | tamanho interno, igual ao nativo sem escala |
| `Frame::Frame` | `Frame(GLContext& gl, const Color& clearColor, bool clearDepth = true)` | viewport e clear no construtor |
| `Frame::Begin2D` | `void Begin2D(SpriteBatch2D& batch, float virtualW, float virtualH)` | desliga depth test |
| `Frame::End2D` | `void End2D()` | fecha o passe; chamado também pelo destrutor |

### Estado de rasterização

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `SetViewport` | `void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)` | com cache |
| `SetScissor` | `void SetScissor(bool enabled, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0)` | sem cache |
| `Clear` | `void Clear(const Color& color, bool clearDepth = true, bool clearStencil = false)` | cor em RGBA8 convertida para float |
| `SetBlendMode` | `void SetBlendMode(BlendMode mode)` | `Opaque` desliga `GL_BLEND`, `Alpha` usa `SRC_ALPHA/ONE_MINUS_SRC_ALPHA`, `Additive` usa `SRC_ALPHA/ONE` |
| `SetBlendEquation` | `void SetBlendEquation(BlendEquationMode mode)` | resolvida por `eglGetProcAddress` (`glBlendEquationOES` ou `EXT`) no Zeebo e `wglGetProcAddress` no Windows |
| `SetDepthTest` | `void SetDepthTest(bool enabled)` | com cache |
| `SetDepthFunc` | `void SetDepthFunc(CompareFunc func)` | sem cache |
| `SetDepthMask` | `void SetDepthMask(bool writeEnabled)` | sem cache |
| `SetAlphaTest` | `void SetAlphaTest(bool enabled, CompareFunc func = CompareFunc::Greater, float ref = 0.5f)` | recorte de sprites com alpha |
| `SetCullFace` | `void SetCullFace(bool enabled)` | `GL_BACK` fixo; com cache |
| `SetFrontFace` | `void SetFrontFace(FrontFace face)` | `CounterClockwise` é o padrão do GL |
| `SetStencilTest` / `SetStencilFunc` / `SetStencilOp` | `void (bool)`, `void (CompareFunc, int32_t, uint32_t)`, `void (StencilOp, StencilOp, StencilOp)` | requer framebuffer com stencil |
| `SetColorMask` | `void SetColorMask(bool r, bool g, bool b, bool a)` | útil para escrever só no depth buffer |
| `SetPolygonOffset` | `void SetPolygonOffset(bool enabled, float factor = 0.0f, float units = 0.0f)` | evita z-fighting de decalque |
| `SetClipPlane` | `void SetClipPlane(uint8_t planeIndex, bool enabled, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f)` | no-op no Zeebo: o Q3Dimension não tem user clip planes |
| `SetDither` | `void SetDither(bool enabled)` | dithering de 16 bits |
| `SetPerspectiveCorrectionFast` | `void SetPerspectiveCorrectionFast(bool fast)` | troca `GL_NICEST` por `GL_FASTEST` no hint |
| `ResetStateCache` | `static void ResetStateCache()` | obrigatório após GL cru |

### Matrizes e câmera

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `SetOrtho` | `void SetOrtho(float left, float right, float bottom, float top, float nearZ = -1.0f, float farZ = 1.0f)` | projeção 2D |
| `SetFrustum` | `void SetFrustum(float left, float right, float bottom, float top, float nearZ, float farZ)` | projeção 3D genérica |
| `SetPerspective` | `void SetPerspective(float fovYDegrees, float aspect, float nearZ, float farZ)` | atalho de `SetFrustum` |
| `ResetModelView` | `void ResetModelView()` | identidade e contador da pilha em 1 |
| `LookAt` | `void LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)` | carrega a visão, não empilha |
| `PushMatrix` / `PopMatrix` | `void PushMatrix()`, `void PopMatrix()` | acima de 16 níveis emite `ZSDK_LOG_WARN` |
| `LoadMatrix` / `MultMatrix` | `void LoadMatrix(const float* mat4x4ColumnMajor)`, `void MultMatrix(const float* mat4x4ColumnMajor)` | ordem de coluna; `nullptr` é ignorado |
| `Translate` / `Rotate` / `Scale` | `void Translate(float, float, float)`, `void Rotate(float angleDegrees, float x, float y, float z)`, `void Scale(float, float, float)` | versão float |
| `TranslateFx` / `RotateFx` / `ScaleFx` | `void TranslateFx(Fixed16, Fixed16, Fixed16)`, `void RotateFx(Fixed16 angleDeg, Fixed16, Fixed16, Fixed16)`, `void ScaleFx(Fixed16, Fixed16, Fixed16)` | `glTranslatex` e companhia |
| `ResetTextureMatrix` | `void ResetTextureMatrix()` | zera a matriz de coordenadas de textura |
| `TranslateTexture` / `ScaleTexture` | `void TranslateTexture(float u, float v)`, `void ScaleTexture(float su, float sv)` | anima UV sem tocar no buffer |

### Iluminação, materiais e neblina

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `SetLighting` | `void SetLighting(bool enabled)` | liga `GL_LIGHTING` e `GL_COLOR_MATERIAL` |
| `ConfigureLight` | `void ConfigureLight(uint8_t lightIndex, const Vec3& position, const Color& ambient, const Color& diffuse, const Color& specular)` | luz pontual em espaço de olho |
| `ConfigureLightEx` | `void ConfigureLightEx(uint8_t lightIndex, const LightDesc& desc)` | `Directional`, `Point` e `Spot` com atenuação |
| `SetLightEnabled` | `void SetLightEnabled(uint8_t lightIndex, bool enabled)` | avisa a partir da segunda luz: no Zeebo a iluminação roda na CPU |
| `SetMaterial` | `void SetMaterial(const Color& ambient, const Color& diffuse, const Color& specular, float shininess)` | aplicado em `GL_FRONT_AND_BACK` |
| `SetGlobalAmbientLight` | `void SetGlobalAmbientLight(const Color& color)` | `GL_LIGHT_MODEL_AMBIENT` |
| `SetShadeModel` | `void SetShadeModel(ShadeModel model)` | `Smooth` ou `Flat` |
| `SetNormalizeNormals` | `void SetNormalizeNormals(bool enabled)` | necessário com escala não uniforme |
| `SetFog` / `ConfigureFog` | `void SetFog(bool enabled)`, `void ConfigureFog(FogMode mode, const Color& color, float start, float end, float density = 1.0f)` | `Linear`, `Exp` ou `Exp2` |

### Vértices, buffers e draw calls

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `BindInterleavedBuffer` | `void BindInterleavedBuffer(const MeshBuffer& vbo, const InterleavedLayout& layout)` | um buffer, N atributos |
| `BindPositionBuffer` / `BindNormalBuffer` | `void BindPositionBuffer(const MeshBuffer& vbo, uint8_t components = 3)`, `void BindNormalBuffer(const MeshBuffer& vbo)` | VBOs separados |
| `BindColorBuffer` / `BindTexCoordBuffer` | `void BindColorBuffer(const MeshBuffer& vbo, uint8_t components = 4)`, `void BindTexCoordBuffer(const MeshBuffer& vbo, uint8_t components = 2)` | VBOs separados |
| `BindRawVertexArrays` | `void BindRawVertexArrays(const void* positions, uint8_t posComponents, const void* normals, const void* colors, const void* texCoords, int32_t strideBytes)` | arrays de RAM, desvincula o VBO de vértice antes de usar ponteiros |
| `UnbindVertexAttributes` | `void UnbindVertexAttributes()` | desliga os quatro client states |
| `DrawArrays` | `void DrawArrays(PrimitiveType type, int32_t firstVertex, int32_t vertexCount)` | registra a draw call no overlay |
| `DrawElementsU16` | `void DrawElementsU16(PrimitiveType type, int32_t indexCount, const uint16_t* indices)` | `nullptr` quando há IBO ligado |
| `SetMatrixPaletteEnabled` / `LoadPaletteMatrixFromModelView` / `BindSkinningBuffers` / `UnbindSkinningBuffers` | `GL_OES_matrix_palette` | só Zeebo |
| `SetPointSpritesEnabled` / `BindPointSizeBuffer` / `UnbindPointSizeBuffer` | `GL_OES_point_sprite`, `GL_OES_point_size_array` | buffer de tamanho só Zeebo |
| `SetActiveTextureUnit` | `void SetActiveTextureUnit(uint8_t unit)` | muda `glActiveTexture` e `glClientActiveTexture` juntos |
| `SetTextureEnvMode` | `void SetTextureEnvMode(TexEnvMode mode)` | `Combine` e `Dot3` usam `GL_COMBINE` |
| `MeshBuffer::Create` | `Result Create(BufferType type, BufferUsage usage, uint32_t sizeBytes, const void* data)` | `data` nulo aloca sem copiar |
| `MeshBuffer::UpdateData` | `Result UpdateData(uint32_t offsetBytes, uint32_t sizeBytes, const void* data)` | usa `glBufferSubData` ou `memcpy` no fallback |
| `MeshBuffer::GetPointerAtOffset` | `const void* GetPointerAtOffset(int32_t offsetBytes) const` | RAM ou offset, conforme o caso |
| `MeshBuffer::GetSizeBytes` / `IsValid` / `IsHardwareVbo` / `GetRamData` | acessores | VRAM contabilizada em `RecordVramAlloc` |

### 2D, sprites e culling

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Renderer::Begin2D` | `void Begin2D(int32_t viewportWidth, int32_t viewportHeight)` | ortográfica com `y` para baixo |
| `Renderer::Begin2DVirtual` | `void Begin2DVirtual(float virtualWidth, float virtualHeight, int32_t realWidth, int32_t realHeight)` | letterbox por proporção |
| `Renderer::End2D` | `void End2D()` | desempilha matrizes e desliga blend |
| `Renderer::GetSafeAreaRect` | `Rect GetSafeAreaRect(int32_t viewportWidth, int32_t viewportHeight, int32_t marginPx = 8)` | margem de TV CRT |
| `Renderer::DrawRect2D` | `void DrawRect2D(float x, float y, float width, float height, const Color& color)` | sem textura, 1 draw call |
| `Renderer::DrawTexturedQuad2D` | `void DrawTexturedQuad2D(const Texture& tex, float x, float y, float width, float height, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, const Color& tint = Color::Rgba(255, 255, 255, 255))` | 1 draw call por quad |
| `Renderer::DrawText2D` | `void DrawText2D(float x, float y, float scale, const char* text, const Color& color)` | 1 draw call por 16 caracteres |
| `Renderer::DrawBillboard3D` | `void DrawBillboard3D(const Texture& tex, const Vec3& worldPos, float width, float height, const Vec3& cameraRight, const Vec3& cameraUp, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, const Color& tint = Color::Rgba(255, 255, 255, 255))` | quad 3D alinhado à câmera |
| `Renderer::DrawBlobShadow` | `void DrawBlobShadow(const Vec3& groundPos, float radius, const Color& color = Color::Rgba(0, 0, 0, 110))` | leque de 10 vértices |
| `Renderer::GetSharedFontTextureId` / `ReleaseSharedFontTexture` | `uint32_t`, `void` | atlas 128x64 `GL_ALPHA` de 8 KiB |
| `SpriteBatch2D::Initialize` | `Result Initialize(uint16_t maxSprites = 128u)` | uma alocação só, em `MemorySubsystem::Graphics` |
| `SpriteBatch2D::Begin` / `BeginVirtual` / `End` | `void Begin(int32_t, int32_t)`, `void BeginVirtual(float, float, int32_t, int32_t)`, `void End()` | `End` faz `Flush` e `Renderer::End2D` |
| `SpriteBatch2D::Flush` | `void Flush()` | um `glDrawElements` por lote |
| `SpriteBatch2D::DrawQuad` | `void DrawQuad(const Texture* tex, float x, float y, float width, float height, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, const Color& tint = Color::Rgba(255, 255, 255, 255))` | `tex` nulo desenha sem textura |
| `SpriteBatch2D::DrawRotated` | `void DrawRotated(const Texture* tex, float centerX, float centerY, float width, float height, float angleRad, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, const Color& tint = Color::Rgba(255, 255, 255, 255))` | ângulo em radianos, centro do quad |
| `SpriteBatch2D::DrawRect` | `void DrawRect(float x, float y, float width, float height, const Color& color)` | atalho de `DrawQuad` com `nullptr` |
| `Frustum::ExtractFromCamera` | `void ExtractFromCamera(const Vec3& eye, const Vec3& target, const Vec3& up, float fovYDegrees, float aspect, float nearZ, float farZ)` | 6 planos normalizados |
| `Frustum::ExtractFromViewProj` | `void ExtractFromViewProj(const float* vp16)` | matriz coluna por coluna |
| `Frustum::TestSphere` / `TestAabb` | `bool TestSphere(const Vec3& center, float radius) const`, `bool TestAabb(const Vec3& minPt, const Vec3& maxPt) const` | descarte conservador |

### Textura e imagem

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Texture::Create` | `Result Create(uint16_t width, uint16_t height, TextureFormat format, const void* pixelData, TextureFilter filter = TextureFilter::Linear)` | largura ou altura zero devolve `InvalidArg` |
| `Texture::CreateCompressed` | `Result CreateCompressed(uint16_t width, uint16_t height, uint32_t glInternalFormat, uint32_t imageSize, const void* compressedData, TextureFilter filter = TextureFilter::Linear)` | formato interno vem como enum cru do GL |
| `Texture::CreateFromAtitc` | `Result CreateFromAtitc(const void* fileData, uint32_t fileSize, TextureFilter filter = TextureFilter::Linear)` | cabeçalho de 5 `uint32`; arquivo tiled devolve `NotSupported` |
| `Texture::UpdateSubImage` | `Result UpdateSubImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixelData)` | sem realocar; `NotInitialized` se não criada |
| `Texture::GenerateMipmaps` | `Result GenerateMipmaps(TextureFilter filter = TextureFilter::Linear)` | fixa `GL_LINEAR_MIPMAP_NEAREST` |
| `Texture::SetWrap` | `Result SetWrap(TextureWrap wrapS, TextureWrap wrapT)` | `ClampToEdge` ou `Repeat` |
| `Texture::CopyFromFramebuffer` | `Result CopyFromFramebuffer(int32_t x, int32_t y, uint16_t width, uint16_t height, TextureFilter filter = TextureFilter::Linear)` | `glCopyTexImage2D` em `GL_RGB`, contabilizado como `RGB565` |
| `Texture::ReadFramebuffer` | `static Result ReadFramebuffer(int32_t x, int32_t y, uint16_t width, uint16_t height, TextureFormat format, void* outPixels)` | origem no canto inferior esquerdo |
| `Texture::Bind` | `void Bind(uint8_t textureUnit = 0) const` | unidade acima de 0 chama `glActiveTexture` |
| `Texture::GetVramBytes` / `GetNativeId` / `IsValid` | acessores | VRAM em bytes |
| `ImageDecoder::DecodeMemory` | `Result DecodeMemory(const void* data, uint32_t size, DecodedImage& outImage, ImageFormat format = ImageFormat::Auto, TextureFormat desiredFormat = TextureFormat::RGBA8888)` | buffer em RAM rastreada, não em VRAM |
| `ImageDecoder::DecodeFile` | `Result DecodeFile(const char* filePath, DecodedImage& outImage, TextureFormat desiredFormat = TextureFormat::RGBA8888)` | via `ISHELL_LoadImage` |
| `ImageDecoder::FreeImage` | `static void FreeImage(DecodedImage& image)` | zera o struct |
| `ImageDecoder::DetectFormat` | `static ImageFormat DetectFormat(const void* data, uint32_t size)` | assinatura PNG, JPEG ou `BM` |

## Limites e armadilhas

### O orçamento de 32 MB, em números

O `MemoryTracker` soma RAM e VRAM em um teto só, com `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` em `32 MiB`. `Texture` e `MeshBuffer` alimentam esse contador a cada criação e destruição, então o painel do overlay mostra o consumo real do subsistema `Graphics`. Os números abaixo são aritmética sobre o que o código aloca:

| Item | Custo | Origem |
| :--- | :--- | :--- |
| framebuffer 640x480 `RGB565` | 614 400 B por buffer (600 KiB) | `640 * 480 * 2`, dois buffers com double buffering |
| textura 512x512 `RGBA8888` | 1 MiB | 4 B por pixel |
| textura 512x512 `RGB565` | 512 KiB | 2 B por pixel |
| textura 512x512 ATITC RGBA | 256 KiB | 16 384 blocos de 16 B |
| textura 1024x1024 `RGBA8888` | 4 MiB | limite superior de dimensão no Zeebo |
| atlas de fonte 8x8 embutido | 8 KiB | `kFontAtlasVramBytes` |
| `SpriteBatch2D` com 128 sprites | 13 824 B (13,5 KiB) | 128 * 4 * 24 de vértice mais 128 * 6 * 2 de índice |
| uma laje de 4 vértices do exemplo | 112 B de VBO e 12 B de IBO | `VertexP3N3C4` |

Duas texturas 1024x1024 em `RGBA8888` já ocupam 8 MiB, um quarto do teto, antes de qualquer áudio, código ou heap de jogo. Por isso o formato importa: a mesma arte em `RGB565` custa metade, e em ATITC custa um oitavo do `RGBA8888`.

### O custo de uma draw call no Q3Dimension

O `Renderer` não tem batcher de 3D: cada `DrawArrays` ou `DrawElementsU16` vira um `glDrawArrays` ou `glDrawElements`, mais uma chamada a `DebugOverlay::RecordDrawCall` que incrementa o contador mostrado como `D:` no painel. O exemplo `01_spinning_cube` chega a 1024 cubos por frame, ou seja, 1024 draw calls e 36 864 índices submetidos a cada quadro, com o cache de estado absorvendo as chamadas repetidas de blend, profundidade, culling e textura.

O que o cache não evita é o custo por objeto: cada cubo faz `PushMatrix`, dois `Rotate`, um `Translate` e um `PopMatrix` em float antes do draw. Em 1024 cubos isso são 5 120 chamadas de matriz por frame só para montar as transformações. Com iluminação ligada, o cálculo de luz por vértice também sai da GPU e roda na CPU, razão do aviso em `SetLightEnabled` quando a segunda luz é habilitada.

O repositório não traz medição de custo por draw call no console, e o [TODO.md](../TODO.md) registra que nada foi validado em hardware Zeebo: os FPS que aparecem no overlay vêm do `FrameTimer` dentro do emulador. Para medir no console, use o `D:` do painel e compare com o tempo de frame antes de concluir qualquer coisa. As alavancas que existem no código são agrupar por textura no 2D (`SpriteBatch2D`), manter geometria estática em um único `MeshBuffer`, descartar por `Frustum` antes de tocar em matriz e evitar trocar estado fora de ordem.

### O que o hardware não faz

`SetClipPlane` é no-op no Zeebo: o Q3Dimension não suporta user clip planes, e a chamada existe só para o código compilar nas duas plataformas. Não há framebuffer object, então render para textura se faz com `Texture::CopyFromFramebuffer` (o `glCopyTexImage2D` copia da tela já desenhada), e o resultado é `GL_RGB`, sem alpha. Mipmap trilinear (`GL_LINEAR_MIPMAP_LINEAR`) é emulado em software, e é por isso que `GenerateMipmaps` fixa `GL_LINEAR_MIPMAP_NEAREST`.

Extensões precisam de teste antes do uso. `GLContext::HasExtension` faz comparação de token inteiro em `GL_EXTENSIONS` e devolve `false` sem contexto corrente. Antes de criar textura ATITC, verifique `GL_ATI_texture_compression_atitc` e caia para `Texture::Create` quando faltar. O [TODO.md](../TODO.md) registra que nenhum caminho da SDK ainda escolhe fallback com base em `HasExtension`: a decisão é sua.

### Diferenças do backend Windows

| Recurso | Zeebo | Windows |
| :--- | :--- | :--- |
| `SetSurfaceScale`, `EnableSurfaceScale`, `SetSurfaceRotate` | `IEGLSurfaceManip` | `Result::NotSupported` |
| `GetPowerLevel` | `IEGLGetPowerLevel` | devolve `Ok` com `0` |
| `SetRenderResolution` | escala por hardware para o tamanho nativo | só troca o tamanho de render e estica no `SwapBuffers` com uma textura auxiliar |
| `SetClipPlane` | no-op | aplica `glClipPlanef` até 6 planos |
| `SetSwapInterval` | `IEGLOESSwapInterval` | `wglSwapIntervalEXT` |
| `ImageDecoder`, `Bitmap2D`, `SpriteEngine2D`, `Font2D`, `HardwareBlitter2D` | implementados | sem implementação: usar em build Windows quebra no link |
| `Display2D` | `IDisplay` | stub que responde 640x480 e não desenha |

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| nada aparece em 3D, o HUD desenha normal | IBO ligado e `DrawElementsU16` recebeu ponteiro de RAM | passe `nullptr` com buffer ligado, ou `MeshBuffer::Unbind(BufferType::Index)` antes |
| o 3D some depois do HUD | `End2D` desempilha a projeção, mas não religa o depth test | chame `SetDepthTest(true)`, `SetPerspective` e `LookAt` de novo para o próximo passe |
| objetos chapados e sem luz | iluminação ligada com layout sem normal (`VertexP3C4U2`) | use `VertexP3N3C4` ou `VertexP3N3U2`; sem array de normal todos os vértices usam a normal corrente |
| HUD com textura preta ou errada | o cache acha que `GL_TEXTURE_2D` está ligado depois de GL cru | chame `Renderer::ResetStateCache()` depois de qualquer chamada GL externa |
| sprites em ordem estranha ou muitos draw calls | cada troca de textura faz `Flush` | desenhe todos os sprites da mesma textura em sequência e agrupe por atlas |
| `DrawQuad` não desenha nada | chamado fora do par `Begin`/`End` | `DrawQuad` retorna sem fazer nada quando `m_inPass` é falso |
| texto com acento vira espaço | `DrawText2D` aceita só 0x20 a 0x7F | escreva em ASCII ou desenhe os acentuados com `DrawTexturedQuad2D` |
| `SetLightEnabled(1, true)` gera aviso no log | iluminação roda na CPU no Zeebo | mantenha uma luz e cozinhe o resto na cor dos vértices |
| aviso de textura "not power-of-2" no log | dimensões fora de 2^n ou acima de 1024 | arredonde com `Texture::NextPowerOfTwo` e ajuste as UVs |
| `CreateFromAtitc` devolve `NotSupported` | arquivo tiled, ou extensão ATITC ausente | rejeite o tiled no pipeline de assets e caia para `Create` |
| `UpdateSubImage` devolve `NotInitialized` | textura nunca criada ou já destruída | crie com `Create` antes; `UpdateSubImage` só vale para textura não comprimida |
| frame rate cai pela metade | `SwapBuffers` chamado depois de um `Frame` já ter feito o swap | um swap por frame, no destrutor do `Frame` ou na última linha de `OnRender` |
| `DecodeMemory` devolve `NotSupported` | assinatura desconhecida ou decodificador BREW indisponível | passe `ImageFormat` explícito e confirme o build para Zeebo |

## Veja também

- [03 Core e assíncrono](./03_core_e_async.md) para o laço de frame, `FrameTimer` e o `IAppDelegate` que chama `OnRender`.
- [04 Debug e profiler](./04_debug_e_profiler.md) para o overlay que mostra `D:`, `V:`, RAM e VRAM, e para `SetStrictBudget`.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md) para o orçamento de 32 MB e a divisão entre serviço portável e backend.
- [11 Referência da API](./11_referencia_da_api.md) para o mapa completo de headers e a ABI C99 de gráficos.
- [examples/01_spinning_cube/main.cpp](../examples/01_spinning_cube/main.cpp) para uma cena completa com 1024 cubos, culling, menu e HUD.
