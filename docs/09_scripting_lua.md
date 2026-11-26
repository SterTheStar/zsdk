# 09 Scripting Lua

O ZSDK embarca Lua 5.4.7 para que a lógica do jogo possa ser escrita, ajustada e recompilada sem tocar em C++ e sem recompilar o `.mod` inteiro. Existem duas portas de entrada, e elas resolvem problemas diferentes: o `zsdk::script::LuaVM` embute um `lua_State` em processo com sandbox e teto de memória, e o binding `zsdk_lua` expõe a ABI C99 inteira para um `lua_State` que o próprio jogo criou. Use o `LuaVM` quando quiser script para partes do jogo; use `zsdk_add_lua_app` quando o jogo for todo em Lua. Se o alvo é performance no ARM11, leia as limitações antes de decidir: o Lua é interpretado e o orçamento de quadro é de 16 ms.

## Visão rápida

| Peça | O que é | Onde vive |
| :--- | :--- | :--- |
| `zsdk::script::LuaVM` | Host embutido: cria o estado, abre libs em sandbox, limita memória, expõe `gfx`/`input`/`save`/`app` | `include/zsdk/script/lua_vm.hpp`, `src/script/lua_vm.cpp` |
| `zsdk_lua` | Binding modular da ABI C99: `zsdk.log`, `zsdk.core`, `zsdk.input`, `zsdk.graphics`, `zsdk.fs`, `zsdk.audio`, `zsdk.net`, `zsdk.security` | `bindings/lua/src/` |
| `zsdk_lua_host.cpp` | Host de app inteiro em Lua, usado automaticamente por `zsdk_add_lua_app` | `bindings/lua/host/` |
| `zsdk_luac` | Compilador de bytecode `main.lua` para `main.luac`, com `LUA_32BITS=1` | `tools/zsdk_luac.c` |
| `require` do `LuaVM` | Carrega `script:/<modulo>.lua` pelo `Vfs`, com cache | `src/script/lua_vm.cpp` |
| Lua vendorizado | Lua 5.4.7 com `LUA_32BITS=1`, `LUA_USE_C89=1` e `LUAI_MAXCCALLS=48` | `third_party/lua` |

## Pré-requisitos

`ZSDK_ENABLE_LUA_VM` precisa estar ligado, e é o padrão: ele compila `src/script/lua_vm.cpp` e cria o alvo `zsdk_lua_runtime` a partir de `third_party/lua`. O binding e o host dependem de `ZSDK_BUILD_LUA_BINDINGS`, também ligado por padrão; desligado, `zsdk_lua` não existe, `zsdk_add_lua_app` volta sem criar alvo e o exemplo 03 desaparece do build. Para `require` e `LoadFile` você precisa de um `zsdk::fs::Vfs` inicializado com o contexto de plataforma, coberto em [07 Filesystem e savegame](./07_filesystem_e_savegame.md).

## Conceitos

### Duas camadas, dois papéis

O `LuaVM` é um host fechado. Ele chama `lua_newstate` com um alocador próprio que conta bytes e respeita um teto, abre um conjunto fixo de bibliotecas, remove o que dá acesso a disco e registra as tabelas `gfx`, `input`, `save` e `app`. O jogo entrega um batch 2D e um `SaveGameManager`, carrega um script e chama `update` e `draw` por quadro.

O `zsdk_lua` é o oposto: um binding aberto, escrito em C99 sobre a ABI, que publica oito módulos para qualquer `lua_State`. Ele não cria estado, não limita memória e não remove bibliotecas. Quem chama é o host, e o host do ZSDK (`zsdk_lua_host.cpp`) cria o estado com `luaL_openlibs`, registra `luaopen_zsdk` e dirige três funções globais: `zsdk_on_init`, `zsdk_on_update` e `zsdk_on_render`.

A diferença importa na hora de escolher. Um jogo com engine em C++ e mods em Lua usa `LuaVM`, porque a superfície exposta é pequena e o teto de memória protege o heap de 32 MB. Um protótipo inteiro em Lua usa `zsdk_add_lua_app`, porque o binding dá acesso a malhas, texturas, áudio e rede, coisas que o `LuaVM` não expõe.

### O sandbox do LuaVM

Três decisões definem o sandbox, todas em `LuaVM::Initialize`. A primeira é o conjunto de bibliotecas: base, `table`, `string`, `math`, `utf8` e `coroutine`. Não entram `io`, `os`, `package` nem `debug`. A segunda é a remoção de `dofile`, `loadfile` e `require`, seguida da instalação de um `require` próprio que só lê do `Vfs`. A terceira é o teto de memória: o alocador recusa qualquer `realloc` que passe do limite e devolve `nullptr`, o que o Lua trata como erro de memória em vez de deixar o app morrer por falta de heap.

O `print` também é substituído. O `print` padrão escreve em `stdout`, que não existe no Zeebo, e o caminho do stub de `abort()` do CRT é um laço infinito. A versão do ZSDK formata a linha num buffer de `ZSDK_DEBUG_LOG_LINE_LEN` bytes e manda para o `Logger` com a tag `LUA`.

Um erro em qualquer callback marca `m_failed`. A partir daí `Update`, `Draw` e `NotifyGamepad` retornam imediatamente, o erro vai para o log como `[E/LUA]`, e o jogo continua rodando sem script. O `Update` ainda bombeia o save assíncrono antes de checar a flag, então uma gravação em andamento termina mesmo com o script quebrado.

### O host de app e os bindings modulares

`zsdk_add_lua_app` monta um app completo com uma linha de CMake: pega `bindings/lua/host/zsdk_lua_host.cpp`, liga com `zsdk_lua` e copia o script para o diretório de saída. O host faz, em `OnInit`, o trabalho que normalmente ficaria no `main.cpp` do jogo:

1. Inicializa o `GLContext` com o contexto de plataforma.
2. Inicializa um `FileSystem` e tenta ler `main.luac`; se falhar, lê `main.lua`. No Zeebo os caminhos são `fs:/~/main.luac` e `fs:/~/main.lua`; no Windows são relativos ao diretório do executável.
3. Cria o estado com `luaL_newstate`, chama `luaL_openlibs`, liga o coletor geracional (`LUA_GCGEN` com 20 e 100) e registra o módulo `zsdk`.
4. Publica dois globais: `zsdk_set_swap_interval(n)` e `zsdk_platform_context`, o ponteiro de plataforma como light userdata.
5. Executa o chunk, cria uma view de `InputManager` com `zsdk_lua_push_input_view` e guarda a referência, então chama `zsdk_on_init(w, h, ctx)`.

Por quadro, `OnUpdate` chama `zsdk_on_update(delta_ms, input)` com o objeto de entrada como segundo argumento, e `OnRender` chama `zsdk_on_render(w, h)`, executa dois passos de GC, desenha o `DebugOverlay` e troca os buffers. O script não precisa e não deve chamar `SwapBuffers`.

Como o host usa `luaL_openlibs`, `io`, `os` e `package` existem no app em Lua puro, ao contrário do `LuaVM`. Ele também não impõe teto de memória: o script pode consumir o heap até o sistema recusar. Se você precisa de sandbox para script de terceiros, use o `LuaVM`.

### Assinatura e configuração

O `LuaVM` é um objeto comum, com ciclo de vida explícito e sem construtor de cópia:

```cpp
namespace zsdk {
namespace script {

static const uint32_t kDefaultLuaMemoryLimit = 2u * 1024u * 1024u;
static const uint32_t kLuaSaveMaxBytes       = 4096u;

class LuaVM : public NonCopyable {
public:
    Result Initialize(uint32_t memoryLimitBytes = kDefaultLuaMemoryLimit);
    void   Shutdown();
    void   Bind(graphics::SpriteBatch2D* batch, fs::SaveGameManager* save);
    void   SetVfs(fs::Vfs* vfs);
    Result LoadFile(const char* path);
    Result LoadString(const char* chunkName, const char* source, size_t length);
    void   Update(float deltaSeconds);
    void   NotifyGamepad(uint8_t index, bool connected);
    void   Draw();
    bool   HasError() const;
    bool   QuitRequested() const;
    uint32_t GetMemoryUsed() const;
};

} // namespace script
} // namespace zsdk
```

`Bind` aceita `nullptr` nos dois parâmetros: sem batch, `gfx.rect` e `gfx.text` viram no-op e `gfx.clear` ainda limpa a tela; sem `SaveGameManager`, `save.*` devolve `false` e `nil`. `SetVfs` é obrigatório para `require` e `LoadFile`, e opcional se você carrega tudo por `LoadString`. `Shutdown` chama `lua_close`, que libera o estado e todo o rastro do alocador; o destrutor chama `Shutdown`, então chamar os dois é seguro.

## Uso passo a passo

### Embutir o LuaVM num app C++

O host mínimo: contexto gráfico, batch, save, Vfs e a VM.

```cpp
#include "zsdk/zsdk.hpp"
#include "zsdk/script/lua_vm.hpp"

class LuaGame : public zsdk::IAppDelegate {
public:
    zsdk::Result OnInit() override {
        auto* app = zsdk::Application::GetInstance();
        zsdk::graphics::GLContextConfig cfg = {5, 6, 5, 0, 16, 0, 1};
        if (zsdk::Failed(m_gl.Initialize(app->GetNativePlatformContext(), cfg))) {
            return zsdk::Result::GraphicsError;
        }
        m_batch.Initialize(128u);
        m_save.Initialize(app->GetNativePlatformContext());
        m_vfs.Initialize(app->GetNativePlatformContext());

        if (zsdk::Failed(m_vm.Initialize(zsdk::script::kDefaultLuaMemoryLimit))) {
            return zsdk::Result::OutOfMemory;
        }
        m_vm.Bind(&m_batch, &m_save);
        m_vm.SetVfs(&m_vfs);
        return m_vm.LoadFile("script:/main.lua");
    }
```

### Rodar o script por quadro

O restante da mesma classe fecha o ciclo: `Update` recebe segundos, não milissegundos, e `Draw` precisa estar entre `BeginVirtual` e `End` do batch:

```cpp
    void OnUpdate(uint32_t deltaTimeMs) override {
        m_vm.Update(static_cast<float>(deltaTimeMs) * 0.001f);
        if (m_vm.QuitRequested()) {
            zsdk::Application::GetInstance()->RequestQuit();
        }
    }

    void OnRender() override {
        const int32_t w = m_gl.GetRenderWidth();
        const int32_t h = m_gl.GetRenderHeight();
        m_batch.BeginVirtual(640.0f, 480.0f, w, h);
        m_vm.Draw();
        m_batch.End();
        m_gl.SwapBuffers();
    }

    void OnGamepadConnected(uint8_t index) override { m_vm.NotifyGamepad(index, true); }
    void OnGamepadDisconnected(uint8_t index) override { m_vm.NotifyGamepad(index, false); }

private:
    zsdk::graphics::GLContext       m_gl;
    zsdk::graphics::SpriteBatch2D   m_batch;
    zsdk::fs::SaveGameManager       m_save;
    zsdk::fs::Vfs                   m_vfs;
    zsdk::script::LuaVM             m_vm;
};
```

`app.quit()` dentro do script só liga a flag do `LuaVM`: quem encerra o app é o `OnUpdate` acima, ao ver `QuitRequested()` e chamar `RequestQuit`. O script em si é carregado no `OnInit`, então erro de sintaxe aparece no log antes do primeiro quadro.

### Um script completo para o LuaVM

O script abaixo usa só a API do `LuaVM`. A primeira parte guarda o estado e carrega o recorde salvo:

```lua
-- script:/main.lua
local W, H = gfx.size()

local paddle = { x = W * 0.5, y = H - 40.0, w = 72.0, h = 12.0, speed = 320.0 }
local ball   = { x = W * 0.5, y = H * 0.5, vx = 150.0, vy = -170.0, size = 10.0 }
local hits   = 0
local best   = 0

function init()
    local blob = save.read(0)
    if blob then
        best = tonumber(blob) or 0
        app.log("recorde carregado: " .. tostring(best))
    end
end
```

A segunda parte é a simulação. O delta chega em segundos, então a velocidade entra na conta sem conversão:

```lua
function update(dt)
    local sx = input.stick(0)
    paddle.x = paddle.x + sx * paddle.speed * dt
    if paddle.x < paddle.w * 0.5 then paddle.x = paddle.w * 0.5 end
    if paddle.x > W - paddle.w * 0.5 then paddle.x = W - paddle.w * 0.5 end

    ball.x = ball.x + ball.vx * dt
    ball.y = ball.y + ball.vy * dt
    if ball.x < ball.size or ball.x > W - ball.size then
        ball.vx = -ball.vx
    end
    if ball.y < ball.size then
        ball.vy = -ball.vy
    end

    local meio = ball.size + paddle.h * 0.5
    if ball.vy > 0 and ball.y > paddle.y - meio and ball.y < paddle.y + meio then
        if ball.x > paddle.x - paddle.w * 0.5 and ball.x < paddle.x + paddle.w * 0.5 then
            ball.vy = -math.abs(ball.vy) * 1.05
            hits = hits + 1
        end
    end

    if ball.y > H + ball.size then
        if hits > best then
            best = hits
            save.write(0, tostring(best))
        end
        hits = 0
        ball.x, ball.y = W * 0.5, H * 0.5
        ball.vx, ball.vy = 150.0, -170.0
    end
end
```

A terceira parte desenha e reage ao hotplug. `draw` roda dentro do batch virtual de 640x480, então as coordenadas são sempre nessa grade:

```lua
function draw()
    gfx.clear(10, 12, 20)
    gfx.rect(paddle.x - paddle.w * 0.5, paddle.y, paddle.w, paddle.h, 90, 180, 255)
    gfx.rect(ball.x - ball.size * 0.5, ball.y - ball.size * 0.5,
             ball.size, ball.size, 255, 220, 90)
    gfx.text(12, 12, "TOQUES " .. hits, 235, 245, 255)
    gfx.text(12, 30, "RECORDE " .. best, 160, 205, 240)
end

function on_pad(index, connected)
    app.log(string.format("pad %d %s", index, connected and "conectado" or "removido"))
end
```

As quatro funções são opcionais e independentes: sem `draw` o módulo `gfx` não é usado, sem `on_pad` o hotplug é ignorado. `input.stick(0)` devolve dois valores, e o índice escolhe o stick esquerdo (0) ou o direito (1) do pad 0, nunca outro pad.

### Carregar de arquivo e dividir em módulos

`LoadFile` resolve o caminho pelo `Vfs`, e `require` monta o caminho por convenção: pontos viram subpastas e o sufixo `.lua` é acrescentado. O módulo é executado uma vez e o resultado fica em cache num registro que o script não alcança.

```lua
-- script:/main.lua
local hud = require("util.hud")

function update(dt)
    hud.tick(dt)
end

function draw()
    hud.draw(gfx)
end
```

```lua
-- script:/util/hud.lua
local M = { frames = 0 }

function M.tick(dt)
    M.frames = M.frames + 1
end

function M.draw(gfx)
    gfx.text(12, 12, "FRAMES " .. M.frames, 235, 245, 255)
end

return M
```

O nome do módulo aceita apenas `A-Z`, `a-z`, `0-9`, `_`, `-` e `.`, e qualquer `..` é recusado, o que impede sair de `script:/` por caminho relativo. Se o chunk não devolver valor, o `require` guarda `true` no cache. O nome é passado ao módulo como único argumento, então um arquivo pode receber o próprio nome sem precisar de global.

### Escrever o app inteiro em Lua

Com `zsdk_add_lua_app` o jogo é um `.lua` e o C++ fica no host da SDK:

```cmake
zsdk_add_lua_app(meu_jogo
    SCRIPT meu_jogo/main.lua
    MIF_APP_NAME "MeuJogoLua"
)
```

Sem `MIF_CLASS_ID`, o `.mif` usa `0x01099001`; o `MIF_APP_NAME` vira o nome do `.mif`, o título da janela no Windows e a definição `ZSDK_LUA_APP_NAME` que o host passa ao `Application`. O contrato do script são três globais. O `zsdk_on_update` recebe o delta em milissegundos e o objeto de entrada:

```lua
local gfx  = zsdk.graphics
local inp  = zsdk.input
local core = zsdk.core

local batch = nil
local x, y = 320.0, 240.0

function zsdk_on_init(w, h, platform_ctx)
    batch = gfx.new_sprite_batch(64)
    core.set_fps_cap(30)
end

function zsdk_on_update(delta_ms, input)
    local sx, sy = input:left_stick(0)
    x = x + sx * 200.0 * delta_ms * 0.001
    y = y + sy * 200.0 * delta_ms * 0.001
    if input:is_pressed(0, inp.BTN_START) then
        core.request_quit()
    end
end

function zsdk_on_render(w, h)
    gfx.set_viewport(0, 0, w, h)
    gfx.clear(12, 14, 24, 255, true, false)
    batch:begin_virtual(640.0, 480.0, w, h)
    batch:draw_rect(x - 24.0, y - 24.0, 48.0, 48.0, 90, 180, 255, 255)
    batch:flush()
    gfx.draw_text_2d(12.0, 12.0, 1.0, "STICK PARA MOVER", 235, 245, 255, 255)
    batch:finish()
end
```

Os métodos de objeto usam dois-pontos (`input:left_stick(0)`) porque o `self` é o userdata; as funções de módulo usam ponto (`gfx.clear(...)`). `zsdk.core` substitui o que o `LuaVM` expõe como `app`: `request_quit`, `set_fps_cap`, `uptime_ms`, `mem_info`, `set_overlay_visible` e `record_draw_call`.

### Compilar bytecode com zsdk_luac

Com `-DZSDK_LUA_BYTECODE=ON` (ou a palavra `BYTECODE` na chamada de `zsdk_add_lua_app`), o CMake roda `zsdk_luac -s <script> main.luac` e remove o `main.lua` do diretório de saída. Sem a flag, copia o `.lua` e remove um `.luac` antigo, para nunca ficar com os dois. À mão:

```powershell
.\build_windows\tools\zsdk_luac.exe -s examples\03_lua_spinning_cube\main.lua main.luac
```

A flag `-s` remove nomes de depuração, `-g` mantém. O `zsdk_luac` só é gerado em builds que não são Zeebo, porque ele é um programa de host; num build de console com `ZSDK_LUA_BYTECODE=ON` o CMake procura o executável em `build_windows/tools` e aborta com `FATAL_ERROR` se não achar. O bytecode tem que sair de um `zsdk_luac` compilado com o mesmo `LUA_32BITS=1`, senão o host recusa o chunk. Note que o `LuaVM` compila apenas texto (`luaL_loadbufferx` com modo `"t"`): bytecode serve ao host de app, não ao `LuaVM`.

## Referência da API

### `LuaVM`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Initialize` | `Result Initialize(uint32_t memoryLimitBytes = kDefaultLuaMemoryLimit)` | `OutOfMemory` se `lua_newstate` falhar |
| `Shutdown` | `void Shutdown()` | `lua_close`; idempotente |
| `Bind` | `void Bind(graphics::SpriteBatch2D* batch, fs::SaveGameManager* save)` | Qualquer um pode ser `nullptr` |
| `SetVfs` | `void SetVfs(fs::Vfs* vfs)` | Habilita `require` e `LoadFile` |
| `LoadFile` | `Result LoadFile(const char* path)` | Sem `Vfs`, devolve `InvalidArg` |
| `LoadString` | `Result LoadString(const char* chunkName, const char* source, size_t length)` | Executa e chama `init()` se existir |
| `Update` | `void Update(float deltaSeconds)` | Bombeia o save assíncrono e chama `update(dt)` |
| `NotifyGamepad` | `void NotifyGamepad(uint8_t index, bool connected)` | Chama `on_pad(index, connected)` |
| `Draw` | `void Draw()` | Chama `draw()`; use entre `BeginVirtual` e `End` |
| `HasError` | `bool HasError() const` | `true` desliga todos os callbacks seguintes |
| `QuitRequested` | `bool QuitRequested() const` | Ligado por `app.quit()` |
| `GetMemoryUsed` | `uint32_t GetMemoryUsed() const` | Bytes vivos segundo o alocador da VM |

### `gfx`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `gfx.clear` | `gfx.clear(r, g, b)` | Faz `Flush` do batch e chama `Renderer::Clear` |
| `gfx.rect` | `gfx.rect(x, y, w, h, r, g, b [, a])` | Alfa padrão 255; no-op sem batch |
| `gfx.text` | `gfx.text(x, y, str, r, g, b [, scale])` | Escala padrão 1.0; faz `Flush` a cada chamada |
| `gfx.size` | `gfx.size() -> 640, 480` | Constantes, não o tamanho real da janela |

### `input`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `input.down` | `input.down(btn) -> bool` | Botão pressionado neste instante |
| `input.pressed` | `input.pressed(btn) -> bool` | Borda de descida |
| `input.stick` | `input.stick(0\|1) -> x, y` | 0 é stick esquerdo, 1 é direito, sempre no pad 0 |
| `input.connected` | `input.connected([pad]) -> bool` | Índice validado contra `ZSDK_MAX_GAMEPADS` |

Os nomes de botão são `up`, `down`, `left`, `right`, `a`, `b`, `x`, `y`, `l`, `r`, `start`, `back` e `home`; qualquer outro gera erro `unknown button`.

### `save`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `save.write` | `save.write(slot, str) -> bool` | Slots 0 a 15, até 4096 bytes |
| `save.read` | `save.read(slot) -> str \| nil` | `nil` quando não há save ou o CRC não confere |
| `save.write_async` | `save.write_async(slot, str) -> bool` | Rejeita string vazia; drena em `Update` |
| `save.status` | `save.status() -> "idle" \| "pending" \| "done" \| "failed"` | Estado do último `write_async` |

Os saves do script usam a versão `0x4C554131` (`'LUA1'`) no cabeçalho do `SaveGameManager`, o que impede um script de ler o save de um app C++ e vice-versa sem combinar a versão.

### `app`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `app.quit` | `app.quit()` | Apenas liga `QuitRequested` |
| `app.log` | `app.log(x)` | Vai para o `Logger` com a tag `LUA` |
| `app.sdcard` | `app.sdcard() -> bool` | `SaveGameManager::IsUsingSdCard()` |

### Globais do `LuaVM`

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `print` | `print(...)` | Junta com tabulação e envia ao `Logger` |
| `require` | `require("nome")` | Exige `SetVfs`; cache em registro privado |
| `init` | `function init()` | Chamada uma vez, depois do chunk |
| `update` | `function update(dt)` | `dt` em segundos |
| `draw` | `function draw()` | Dentro do batch virtual |
| `on_pad` | `function on_pad(index, connected)` | Hotplug encaminhado do delegate |

### Módulos do `zsdk_lua`

| Módulo | Conteúdo | Chamada típica |
| :--- | :--- | :--- |
| `zsdk.log` | `info`, `warn`, `error`, `write` | `zsdk.log.info("TAG", "msg")` |
| `zsdk.core` | Tempo, memória, bateria, idioma, overlay, FPS, quit | `zsdk.core.set_fps_cap(30)` |
| `zsdk.input` | `new`, `new_kbm`, `new_vkbd`, métodos por dois-pontos e constantes `BTN_*` | `input:is_down(0, inp.BTN_ACTION1)` |
| `zsdk.graphics` | `new_gl`, `new_texture`, `new_mesh`, `new_sprite_batch`, `new_frustum` e as funções de estado e desenho | `batch:draw_rect(...)`, `gfx.draw_text_2d(...)` |
| `zsdk.fs` | `new_fs`, `new_storage`, `new_savegame`, `new_ram_cache` | `saveMgr:save_slot(0, blob, 2)` |
| `zsdk.audio` | `new`, `new_pcm_mixer`, `new_media`, `new_sfx` e métodos de música, efeito e voz | `local music = zsdk.audio.new()` |
| `zsdk.net` | `new_manager`, `new_socket`, `new_http`; socket com `connect_ipv4`, `send`, `recv` | `local sock = zsdk.net.new_socket(...)` |
| `zsdk.security` | `hash_md5`, `hash_sha1`, `hmac_md5`, `hmac_sha1`, `random_u32`, `random_bytes`, `check_license`, `rsa_mod_exp` | `zsdk.security.hash_md5(data)` |

O que não existe na API do `LuaVM`: textura, sprite, áudio, tempo decorrido, gerador aleatório e câmera 3D. Nos bindings `zsdk.*` há textura, malha, áudio e rede, mas não há função de tempo por quadro. As duas lacunas estão em [TODO.md](../TODO.md), junto com a intenção de registrar `luaopen_zsdk` dentro do `LuaVM` para juntar as duas APIs num só estado.

## Limites e armadilhas

O teto de memória do `LuaVM` é 2 MiB por padrão e inclui o estado, o chunk compilado e todas as tabelas. Estourar não derruba o processo: a alocação falha, o Lua levanta erro, o `PCall` loga e a VM entra em `HasError`. A `LuaVM` não aparece no `MemoryTracker`, então esse consumo fica fora do `MEM:` do overlay e do orçamento que o `Application` configurou.

Não há limite de instruções nem de tempo por quadro. Um `while true do end` dentro de `update` trava o app: o sandbox corta disco e memória, não CPU. O [TODO.md](../TODO.md) registra a ausência de um `lua_sethook` com contador, então script não confiável precisa rodar fora do quadro principal.

Com `LUA_32BITS`, `lua_Integer` e `lua_Number` são de 32 bits. Multiplicar dois inteiros grandes dá wrap silencioso: `1103515245 * seed` estoura, e o LCG do exemplo usa constantes pequenas por isso. Para posição e velocidade use o que já é `float` (o `lua_Number` em 32 bits é `float`), e não misture contadores inteiros grandes.

`save.read` usa um buffer `static` de 4096 bytes dentro do binding. Duas leituras aninhadas no mesmo passo, ou uma leitura seguida de uso do buffer depois de outra chamada, se sobrepõem. Copie a string para uma variável Lua antes de chamar outro `save.*`.

`gfx.text` faz `Flush` do batch a cada chamada. Um HUD com vinte linhas vira vinte draw calls. Agrupe o texto estático numa string só, ou desenhe o texto fora do `draw` do script, no C++.

`LUAI_MAXCCALLS` está em 48, e não nos 200 padrão, porque o app tem 32 KB de stack. Chamadas aninhadas entre C e Lua além disso viram erro `C stack overflow`. Recursão em Lua puro não conta para esse limite, mas um `require` que se chama em profundidade conta.

O `.mod` do demo com Lua fica em cerca de 398 KB, contra 169 KB do cubo em C++, quase tudo por causa da libc do newlib e do interpretador. Se o orçamento de 32 MB apertar, desligue `utf8` e `coroutine` se o script não usar, como sugere o [TODO.md](../TODO.md).

O host `zsdk_add_lua_app` não tem sandbox: `io`, `os` e `package` estão disponíveis, o coletor é geracional com passo de 2 por quadro e não há teto de memória. Ele também não expõe `require` do `Vfs`: o `require` ali é o do `package`, resolvido por caminho do processo.

Nada disso foi medido em hardware Zeebo. O [TODO.md](../TODO.md) registra 14M de instruções em 300 quadros no emulador e nenhum benchmark real, e diz que o limite de memória e o sandbox nunca foram testados sob estresse.

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `require: no filesystem bound` | `SetVfs` não foi chamado antes do `LoadFile` ou do `require` | Chame `m_vm.SetVfs(&vfs)` depois de `Vfs::Initialize` |
| `module 'x' not found (script:/x.lua)` | Arquivo fora do prefixo `script:` ou nome com caractere inválido | Confirme o caminho no `Vfs` e use só `A-Z a-z 0-9 _ - .` no nome |
| Script para de rodar depois de um erro e o log mostra `[E/LUA]` | `m_failed` foi marcado e todos os callbacks ficam desligados | Corrija o script e recrie a VM: não há volta de `HasError` |
| `gfx.rect` não desenha nada | VM sem batch ligado, ou `draw` chamado fora de `BeginVirtual` | Chame `Bind(&batch, &save)` e envolva `Draw` com `Begin`/`End` |
| `app.quit()` não encerra o app | O `LuaVM` só liga a flag `QuitRequested` | Verifique `QuitRequested()` no `OnUpdate` e chame `RequestQuit` |
| Números de posição pulam com script rodando perto de 60 FPS | Inteiros de 32 bits fazendo wrap em conta de física | Use `float` para posição e velocidade, inteiro só para contadores pequenos |
| O host de app falha ao carregar o script no console | `main.lua` não foi copiado para `mod/<alvo>/` | Use `zsdk_add_lua_app` com `SCRIPT` e não mova o arquivo depois do build |
| `ZSDK_LUA_BYTECODE is ON, but zsdk_luac was not found` | Build Zeebo não gera o compilador de host | Compile `build_windows` primeiro, ou desligue a flag |

## Veja também

* [02 Build e empacotamento](./02_build_e_empacotamento.md): `zsdk_add_lua_app`, `ZSDK_ENABLE_LUA_VM` e `ZSDK_LUA_BYTECODE`.
* [06 Gráficos 3D e 2D](./06_graficos_3d_e_2d.md): o `SpriteBatch2D` e o `Renderer` que o script comanda por `gfx`.
* [07 Filesystem e savegame](./07_filesystem_e_savegame.md): o `Vfs` de `script:` e o `SaveGameManager` usado por `save.*`.
* [bindings/README.md](../bindings/README.md): a ABI C99 que o binding Lua consome e a checagem de paridade.
* Código: [lua_vm.hpp](../include/zsdk/script/lua_vm.hpp), [lua_vm.cpp](../src/script/lua_vm.cpp), [zsdk_lua.h](../bindings/lua/include/zsdk_lua.h), [zsdk_lua_host.cpp](../bindings/lua/host/zsdk_lua_host.cpp) e [main.lua](../examples/03_lua_spinning_cube/main.lua).
