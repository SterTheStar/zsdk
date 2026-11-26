# 09 Primeiros passos

Este tutorial leva do clone do repositório até o cubo girando na janela do Windows e, no fim, empacotado em `.mod` e `.mif` para o console. Pelo caminho você lê o `main.cpp` do exemplo, muda uma linha e confere a diferença na tela, e vê o mesmo app escrito em C99, Lua 5.4 e Rust no_std. Cada etapa termina com algo que você consegue verificar antes de seguir para a próxima.

Reserve cerca de uma hora. A primeira compilação consome uns vinte minutos, entre instalar o toolchain e ver o primeiro executável; o resto do tempo vai em leitura, uma alteração e o empacotamento. Não é preciso conhecer BREW nem OpenGL, apenas C++ básico.

## O que precisa estar instalado

| Item | Versão ou caminho | Para que serve |
| :--- | :--- | :--- |
| `cmake` | 3.16 ou mais novo | o `CMakeLists.txt` da raiz exige essa versão mínima |
| `ninja` | qualquer do PATH | gerador usado em todos os comandos deste tutorial |
| Visual Studio 2022 | carga de trabalho de C++ desktop | compilador MSVC x64 do alvo Windows |
| `arm-none-eabi-gcc` e `arm-none-eabi-g++` | qualquer toolchain ARM EABI | alvo Zeebo, a partir da etapa 5 |
| SDK BREW/Zeebo | `C:/Users/Esther/AppData/Local/Programs/brew-sdk` | headers BREW, `cifc` e `elf2mod`; troque com `-DBREW_SDK_ROOT=...` |
| Python 3 | opcional | roda `tools/check_bindings.py` durante o build |
| `cargo` | opcional | compila o exemplo Rust |

O `TODO.md` registra que o build Windows foi cross-compilado com MinGW-w64 e que o MSVC de verdade continua sem teste. Se o `vcvars64.bat` do seu Visual Studio estiver em outro diretório, ajuste o caminho do comando abaixo.

## Etapa 1: conferir o ambiente e compilar o exemplo C++

Confira as duas ferramentas que o build usa primeiro:

```powershell
cmake --version
ninja --version
```

O `cmake --version` precisa responder 3.16 ou acima. Se o `ninja` não estiver no PATH, a geração para antes de compilar, reclamando que não achou o programa de build correspondente ao gerador Ninja.

Agora compile o alvo Windows a partir da raiz do repositório, com o ambiente do MSVC carregado:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake -S . -B build_windows -G Ninja -DZSDK_BUILD_ZEEBO=OFF -DZSDK_BUILD_EXAMPLES=ON && cmake --build build_windows'
```

A compilação gera dois artefatos que interessam aqui:

- `build_windows/zsdk.lib`: a biblioteca estática com o núcleo, a ponte C99 e o backend Windows.
- `build_windows/apps/01_spinning_cube/01_spinning_cube.exe`: o app. Ele fica dentro de `apps/<alvo>`, porque `zsdk_add_brew_app` define `RUNTIME_OUTPUT_DIRECTORY` assim nos dois alvos.

Durante o build aparece também a mensagem `Verifying ZSDK C ABI & Lua bindings parity`, do alvo `zsdk_check_bindings`, que roda em todo build quando o CMake encontra o Python 3. Se o Python não estiver instalado, o alvo nem é criado e o build segue.

Não existe `CMakeLists.txt` dentro de `examples/01_spinning_cube`: o diretório tem só o `main.cpp`, e quem cria o executável, linka `zsdk` com `opengl32`, `user32`, `gdi32` e `winmm` e registra o app é a raiz do projeto.

## Etapa 2: rodar no Windows e reconhecer a tela

```powershell
.\build_windows\apps\01_spinning_cube\01_spinning_cube.exe
```

A janela abre com 640 por 480 pixels de área cliente e o título `ZSDK_SpinningCube`, que vem do segundo argumento de `ZSDK_IMPLEMENT_APP` no fim do `main.cpp`. O esperado, na ordem em que você percebe:

- Um fundo azul muito escuro, o `Color::Rgba(16, 20, 32, 255)` do `Renderer::Clear`.
- Um cubo iluminado girando devagar, com seis faces de cores diferentes: ciano à frente, laranja atrás, verde à esquerda, magenta à direita, amarelo em cima, vermelho embaixo.
- No canto superior esquerdo, duas linhas de HUD: `CUBES: 1  DRAWN: 1` e `FRUSTUM:ON FACE:ON`.
- Sobre elas, o painel do `DebugOverlay`, com o FPS em número grande, `FPS x.xms`, `D:x V:y` e uma linha `MEM ...K/32M (...) PK:...K`. O painel aparece porque o `main.cpp` passa `true` no quarto argumento de `ZSDK_IMPLEMENT_APP`, e é desenhado dentro do `SwapBuffers`, depois do seu quadro, por isso cobre o HUD.

O teclado só responde enquanto a janela está em primeiro plano, porque o backend lê as teclas com `GetAsyncKeyState` apenas quando ela é a janela ativa. O mapeamento do Z-Pad no Windows é este:

| Tecla | Botão lógico |
| :--- | :--- |
| setas | D-Pad |
| `Enter` | Start |
| `Z` | Action1 |
| `X` | Action2 |
| `Backspace` | Back |
| `Esc` | Home |

Com isso você navega: `Enter` abre e fecha o menu de dez itens, as setas movem o cursor, `Z` ou a seta para a direita altera o item, `Backspace` fecha o menu. Um controle de Xbox conectado entra como jogador 1 pelo XInput, com sticks e gatilhos, e os dois dispositivos se somam.

O painel de debug sai do caminho pelo item `Debug Panel` do menu, que mostra o estado atual e inverte ao ser acionado. Existe também o atalho de ombro esquerdo mais D-Pad para baixo, mas ele exige um controle de verdade: o teclado mapeado no backend Windows não tem tecla de ombro.

Duas coisas do exemplo contrariam o que se espera à primeira vista. O comentário no topo do `main.cpp` diz que Home sai do app, mas o `OnUpdate` registra Home como um segundo botão de menu; sair de fato só acontece fechando a janela ou escolhendo `Exit App`, o décimo item. E as setas movem a câmera com o menu fechado, então pan e navegação usam as mesmas teclas em contextos diferentes.

Ver o cubo girando já prova que o contexto EGL subiu, o pipeline do GLES 1.1 responde, o `FrameTimer` está alimentando `OnUpdate` e o `SwapBuffers` está apresentando o quadro.

## Etapa 3: ler o main.cpp e entender os três pontos

O arquivo tem cerca de 550 linhas, e a maior parte é tabela de vértices, menu e HUD. Três coisas explicam todo o resto.

### A classe do jogo implementa IAppDelegate

`IAppDelegate` é o contrato: quatro métodos puros e quatro opcionais. O exemplo implementa `OnInit`, `OnUpdate`, `OnRender` e `OnShutdown`, e deixa `OnSuspend`, `OnResume` e os dois de gamepad com o comportamento padrão, que não faz nada.

```cpp
#include "zsdk/zsdk.hpp"

class MeuJogo : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        auto* app = zsdk::Application::GetInstance();
        return m_gl.Initialize(app->GetNativePlatformContext(), {5, 6, 5, 0, 16, 0, 1});
    }

    void OnUpdate(uint32_t deltaTimeMs) override {
        m_angulo += static_cast<float>(deltaTimeMs) * 0.048f;
    }

    void OnRender() override {
        using zsdk::graphics::Renderer;
        Renderer::SetViewport(0, 0, m_gl.GetRenderWidth(), m_gl.GetRenderHeight());
        Renderer::Clear(zsdk::Color::Rgba(16, 20, 32, 255), true);
        m_gl.SwapBuffers();
    }

    void OnShutdown() override { m_gl.Shutdown(); }

private:
    zsdk::graphics::GLContext m_gl;
    float m_angulo = 0.0f;
};

ZSDK_IMPLEMENT_APP(MeuJogo, "MeuJogo", 60, true)
```

`OnInit` devolve um `zsdk::Result`, e devolver erro aborta a inicialização: o backend Windows desfaz a janela e o processo termina com código 1. É o primeiro ponto onde uma falha faz o app simplesmente não abrir, então trate cada `Initialize` e cada `Create` na hora.

O `GLContextConfig` acima pede framebuffer RGB 5-6-5, 16 bits de depth, sem stencil e com VSync ligado, que é a configuração que o exemplo 01 usa tanto no Windows quanto no Zeebo.

### O Application é o host, e a macro liga o resto

`Application` é o objeto que o backend cria, configura e roda: ele guarda o `InputManager`, o `FrameTimer` e o contexto de plataforma que você passa para o `GLContext`. Você não o instancia nem o destrói; chama `zsdk::Application::GetInstance()` dentro dos callbacks.

A ponte entre a sua classe e o backend é uma única função, `zsdk::CreateUserApp(AppConfig&)`, que o backend chama no início. `ZSDK_IMPLEMENT_APP(Classe, "Nome", FpsAlvo, OverlayVisivel)` gera essa função, uma instância estática do delegate e o `AppConfig` com orçamento de memória padrão:

```cpp
#define ZSDK_IMPLEMENT_APP(DelegateClass, AppName, TargetFps, EnableDebugOverlay) \
    static DelegateClass g_zsdkUserDelegate;                                      \
    namespace zsdk {                                                              \
    IAppDelegate* CreateUserApp(AppConfig& outConfig) {                           \
        outConfig.name               = (AppName);                                 \
        outConfig.targetFps          = (TargetFps);                               \
        outConfig.memoryBudgetBytes  = ZSDK_DEFAULT_MEMORY_LIMIT_BYTES;           \
        outConfig.enableDebugOverlay = (EnableDebugOverlay);                      \
        return &g_zsdkUserDelegate;                                               \
    }                                                                             \
    }
```

Por isso não existe `main` nem `WinMain` nem `AEEClsCreateInstance` em nenhum exemplo. O exemplo 01 escolhe 60 FPS no Windows e 30 no Zeebo com um `#if ZSDK_PLATFORM_ZEEBO` em volta da macro, porque a saída de TV do console pede 30 FPS.

### OnUpdate mede o tempo, OnRender desenha, SwapBuffers apresenta

O laço do backend é sempre o mesmo: medir o tempo do quadro, chamar `OnUpdate` com o delta em milissegundos, chamar `OnRender`, esperar o deadline do FPS alvo. O delta vem do `FrameTimer` já limitado ao intervalo de 1 a 250 ms, então um quadro perdido por suspensão não teleporta a simulação.

Dentro de `OnRender` a ordem também importa: dimensões vem do `GLContext`, que reflete a superfície atual, e não de constantes como 640 e 480. O `Renderer` aplica viewport, limpa os buffers, monta a matriz de projeção e desenha; quem apresenta é `SwapBuffers`, e sem essa chamada a tela fica no quadro anterior, mesmo com todo o desenho feito. A entrada chega por `app->GetInput().GetGamepad(0)`, com `IsDown` para estado contínuo, `IsPressed` para o toque do quadro e `GetLeftStick` para o analógico já com deadzone.

## Etapa 4: trocar algo pequeno e ver a mudança

O jeito mais rápido de confirmar que você está editando o código que roda é trocar a cor de fundo. Em `main.cpp`, dentro de `OnRender`:

```cpp
void OnRender() override {
    using zsdk::graphics::Renderer;
    const int32_t w = m_gl.GetRenderWidth();
    const int32_t h = m_gl.GetRenderHeight();

    Renderer::SetViewport(0, 0, w, h);
    Renderer::Clear(zsdk::Color::Rgba(90, 20, 20, 255), true);
```

Recompile e rode de novo:

```powershell
cmake --build build_windows
.\build_windows\apps\01_spinning_cube\01_spinning_cube.exe
```

O fundo agora é vinho, e o cubo, o HUD e o overlay continuam iguais, porque nada mais no arquivo depende dessa cor. Se você não vir diferença, o executável que abriu não é o que acabou de ser compilado; apague `build_windows` e repita a etapa 1.

O mesmo teste vale para texto: a primeira linha do HUD é montada por `AppendStr(line1, 0, sizeof(line1), "CUBES: ")`, e trocar essa string muda o que aparece no canto superior esquerdo sem tocar em mais nada. Mudanças de comportamento, como a velocidade base em `spinTable` dentro de `OnUpdate`, seguem o mesmo ciclo de editar, compilar e rodar.

## Etapa 5: empacotar o .mod e o .mif para o Zeebo

O mesmo código compila para o console com o alvo ARM e o SDK BREW no lugar:

```powershell
cmake -S . -B build_zeebo -G Ninja `
  -DCMAKE_SYSTEM_NAME=Generic `
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY `
  -DZSDK_BUILD_ZEEBO=ON `
  -DZSDK_BUILD_EXAMPLES=ON

cmake --build build_zeebo
```

A etapa de link monta o ELF com `tools/elf2mod/elf2mod.x`, `-nostartfiles`, `--emit-relocs` e entrada em `AEEMod_Load`; o pós-build então chama o `elf2mod.exe` e o `cifc.exe`. O resultado fica em dois diretórios distintos:

| Caminho gerado | O que é |
| :--- | :--- |
| `build_zeebo/apps/01_spinning_cube/01_spinning_cube.elf` | ELF relocável, insumo do empacotador |
| `build_zeebo/apps/01_spinning_cube/mod/01_spinning_cube/01_spinning_cube.mod` | módulo BREW pronto para copiar |
| `build_zeebo/apps/01_spinning_cube/mif/01_spinning_cube.mif` | manifesto compilado pelo `cifc` |

O CMake grava o `.mod` dentro de uma pasta com o nome do módulo, e essa pasta é o nível que o console expõe como `/mod/<jogo>`. O `.mif` vem de `tools/mif/zsdk_template.mif.in`, preenchido pelo `configure_file` com `@ONLY` a partir dos argumentos da macro. Para o exemplo 01, sem argumentos de MIF, os valores são estes:

| Campo | Valor |
| :--- | :--- |
| `appletid` | `0x01099001`, o padrão de `zsdk_add_brew_app` |
| `privs` | `0x07AF`, sem `PL_SHARED_WRITE` para não escrever na NAND compartilhada |
| `deps` | `0x0101414E`, `AEECLSID_CARD0_ACCESS` |
| nome exibido | `01_spinning_cube`, o nome do alvo |

Para o seu próprio jogo, troque a chamada no `CMakeLists.txt` da raiz por uma com identidade própria:

```cmake
include(cmake/ZsdkBrew.cmake)

zsdk_add_brew_app(meu_jogo
    SOURCES
        src/main.cpp
        src/player.cpp
    MIF_CLASS_ID  0x01099010
    MIF_APP_NAME  "Meu Jogo"
    MIF_COMPANY   "Meu Estudio"
    MIF_COPYRIGHT "2026 Meu Estudio"
    MIF_VERSION   "1.0.0"
)
```

Dois pontos que costumam passar batido. Se o `cifc.exe` não for achado nos dois diretórios que o CMake procura, o pós-build copia o `.cif` configurado com o nome `.mif`, o que produz um arquivo com o nome certo e o conteúdo errado; a linha do log do build denuncia, porque diz `Copying configured mif/... (cifc not found)` em vez de `Packaging mif/... via cifc`. E o repositório não traz ferramenta de instalação: o `TODO.md` registra que o Guia do desenvolvedor descreve apenas `/mif` e `/mod/<jogo>` como destino de upload, e que a gravação em cartão SD por app nunca foi comprovada no console.

Nada disso foi exercitado em hardware Zeebo real até aqui. O `TODO.md` é explícito: só houve execução no emulador, e os FPS que aparecem no overlay saem do `FrameTimer` dentro do emulador, não de medição no console.

## Etapa 6: o mesmo app em C99, Lua e Rust

Os quatro exemplos desenham a mesma cena e nascem da mesma biblioteca. O que muda é a linguagem de entrada e quem faz o papel de `IAppDelegate`:

| Linguagem | Macro CMake | Arquivo de entrada | Quem implementa o ciclo |
| :--- | :--- | :--- | :--- |
| C++14 | `zsdk_add_brew_app` | `main.cpp` | a sua classe derivada de `IAppDelegate` |
| C99 | `zsdk_add_c_app` | `cube_c.c` | `zsdk_c_app_init/update/render/swap/shutdown` |
| Lua 5.4 | `zsdk_add_lua_app` | `main.lua` | `zsdk_on_init/update/render` |
| Rust no_std | `zsdk_add_rust_app` | `src/lib.rs` | funções `extern "C"` com os mesmos nomes do C |

### C99: a ABI C no lugar das classes

O arquivo C inclui apenas `zsdk/c/zsdk_c.h` e implementa cinco funções livres. O host em C++ que vem junto com a macro, `zsdk_c_host.cpp`, as chama e faz o `DebugOverlay::Render` e o swap por você, então o `zsdk_c_app_swap` existe e é chamado, mas o desenho do overlay fica fora do seu código.

```c
int zsdk_c_app_init(void* platform_ctx) {
    zsdk_gl_config_t cfg = {5u, 6u, 5u, 0u, 16u, 0u, 1u};
    g_gl = zsdk_gl_create();
    if (!g_gl || zsdk_gl_init(g_gl, platform_ctx, &cfg) != ZSDK_C_OK) {
        ZSDK_C_LOG_ERROR("CUBE_C", "GLContext init failed");
        return -1;
    }
    g_batch = zsdk_batch_create();
    if (!g_batch || zsdk_batch_init(g_batch, 64u) != ZSDK_C_OK) {
        return -1;
    }
    g_frustum = zsdk_frustum_create();
    return 0;
}

void zsdk_c_app_shutdown(void) {
    if (g_frustum) { zsdk_frustum_destroy(g_frustum); g_frustum = NULL; }
    if (g_batch)   { zsdk_batch_shutdown(g_batch); zsdk_batch_destroy(g_batch); g_batch = NULL; }
    if (g_gl)      { zsdk_gl_shutdown(g_gl); zsdk_gl_destroy(g_gl); g_gl = NULL; }
}
```

Tudo que em C++ é objeto na pilha vira par `create` e `destroy` com ponteiro opaco, e as cores viram quatro `uint8_t` em vez de um `Color`. Um `return -1` em `zsdk_c_app_init` segue o mesmo caminho do `Result` de erro no C++: o host falha e o app não roda.

### Lua 5.4: script puro sobre um host embutido

No Lua não há struct nem ponteiro. O host carrega o script, chama `zsdk_on_init`, `zsdk_on_update` e `zsdk_on_render` quando existem, e expõe a SDK pela tabela global `zsdk`, aberta por `luaopen_zsdk`. Também deixa o contexto de plataforma em `zsdk_platform_context` e registra `zsdk_set_swap_interval`, que o exemplo usa para alternar o VSync.

```lua
local log  = zsdk.log
local inp  = zsdk.input
local gfx  = zsdk.graphics
local fs   = zsdk.fs
local core = zsdk.core

local batch, saveMgr

function zsdk_on_init(w, h, platform_ctx)
    batch = gfx.new_sprite_batch(64)
    gfx.configure_light(0, 6.0, 8.0, 10.0, 64, 64, 80, 255, 242, 242, 230, 255, 204, 204, 204, 255)
    saveMgr = fs.new_savegame(platform_ctx or zsdk_platform_context)
    log.info("CUBE_LUA", string.format("Spinning Cube ready (%dx%d)", w, h))
end

function zsdk_on_update(delta_ms, input)
    if not input then return end
    if input:is_pressed(0, inp.BTN_START) then
        core.set_overlay_visible(not core.is_overlay_visible())
    end
end

function zsdk_on_render(w, h)
    gfx.set_viewport(0, 0, w, h)
    gfx.clear(16, 20, 32, 255, true, false)
end
```

O `w` e o `h` chegam como argumentos, e o objeto `input` é uma view sobre o `InputManager`, com métodos chamados por dois-pontos. O script é copiado para o diretório do executável no Windows e para `mod/<alvo>/` no Zeebo, e o host tenta `main.luac` antes de `main.lua`; com `-DZSDK_LUA_BYTECODE=ON`, o `zsdk_luac` compila o bytecode. A macro só cria o alvo se a biblioteca `zsdk_lua` existir, e ela depende de `ZSDK_BUILD_LUA_BINDINGS`, ligado por padrão.

### Rust no_std: a mesma ABI, sem runtime

A crate é `staticlib` e `#![no_std]`, precisa de um `#[panic_handler]` e exporta as mesmas cinco funções C com `#[no_mangle]`. O host `zsdk_rust_host.cpp` é o mesmo tipo de ponte que o do C, então nada do ciclo de vida muda:

```rust
#![no_std]

use core::ffi::c_void;
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_init(_platform_ctx: *mut c_void) -> i32 { 0 }

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_update(_delta_ms: u32, _input: *const zsdk::sys::zsdk_input_t) {}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_render(_out_width: *mut i32, _out_height: *mut i32) {}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_swap() {}

#[no_mangle]
pub unsafe extern "C" fn zsdk_c_app_shutdown() {}
```

Como não há `std` nem alocação dinâmica, o exemplo 04 guarda tudo em `static STATE: SyncCell<CubeState>`, um `UnsafeCell` embrulhado num tipo que declara `Sync`, seguro porque o host chama tudo na mesma thread. O `Cargo.toml` usa `crate-type = ["staticlib"]`, `panic = "abort"` nos dois perfis e depende de `zsdk` por caminho, em `../../bindings/rust/zsdk`. No Zeebo, a macro compila para `armv5te-unknown-linux-gnueabi` com `RUSTFLAGS=-C relocation-model=static`. Se o `cargo` não estiver no PATH, `zsdk_add_rust_app` retorna sem aviso e o exemplo 04 não é gerado.

## Etapa 7: para onde ir depois

O cubo é pequeno de propósito. Os assuntos que ele só encosta estão nos guias:

- [01 Build e empacotamento](./01_build_e_empacotamento.md): toolchain ARM, todas as opções CMake, conteúdo do `.mif`, build fora do Windows e as flags de `config.hpp`.
- [02 Core e assíncrono](./02_core_e_async.md): `Application` a fundo, `FrameTimer`, `Clock`, ponto fixo, threads e sinais, mais o tratamento obrigatório de suspensão e de gamepad.
- [05 Gráficos 3D e 2D](./05_graficos_3d_e_2d.md): `GLContext`, cache de estado do `Renderer`, malhas, texturas, `SpriteBatch2D` e frustum, que é onde o cubo vira um jogo.
- [04 Input e controles](./04_input_controles.md) e [03 Debug e profiler](./03_debug_e_profiler.md): deadzone, rumble, teclado e mouse, e o que cada número do overlay significa.
- [Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md) e [Referência da API](./11_referencia_da_api.md): o mapa dos headers e o orçamento de memória do console.

Para escrever fora do C++, comece por [bindings/README.md](../bindings/README.md), que documenta a ABI C99 e a paridade verificada entre C, Lua e Rust, e por [08 Scripting Lua](./08_scripting_lua.md) se o plano for embutir script no jogo em vez de escrever o app inteiro em Lua.

## Se algo der errado

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| o build para na configuração ou no link, com erro de compilador inexistente | o `vcvars64.bat` não foi carregado, ou o comando foi rodado fora do `cmd /c` encadeado da etapa 1 | rode a linha inteira de novo, numa sessão nova do PowerShell |
| `build_zeebo` termina com o `.elf` mas sem `.mif` compilado | o `cifc.exe` não foi encontrado nos diretórios que o CMake procura | aponte `-DBREW_SDK_ROOT=` para a raiz correta do SDK; sem isso, o `.mif` gerado é cópia do `.cif` |
| o executável abre e fecha na hora | `OnInit` devolveu erro, quase sempre de `GLContext::Initialize` | coloque um `ZSDK_LOG_ERROR` antes do `return` e confira o `GLContextConfig` |

O passo a passo de diagnóstico por sintoma, com os códigos de erro e as mensagens do BREW, está em [12 Solução de problemas](./12_solucao_de_problemas.md). Para o que ainda não funciona no console, a lista de pendências fica em [TODO.md](../TODO.md).
