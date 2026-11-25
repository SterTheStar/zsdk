# 08 — Scripting Lua (`zsdk::script::LuaVM`)

VM Lua 5.4.7 embutida (`third_party/lua`, licença MIT), compilada com `LUA_32BITS` (inteiros e floats de 32 bits, mais leve no ARM11). Habilitada por `ZSDK_ENABLE_LUA_VM` (ON por padrão).

## Sandbox
* Bibliotecas abertas: base, `table`, `string`, `math`, `utf8`, `coroutine`. **Sem** `io`, `os`, `package`, `debug`; `dofile`, `loadfile` e `require` removidos.
* Memória limitada (padrão 2 MiB, `LuaVM::Initialize(limit)`); estourar o limite gera erro Lua, não crash.
* Erro em `update`/`draw` é logado (`[E/LUA]`) e desativa novas chamadas (`HasError()`).
* Com inteiros de 32 bits, `*` e `+` dão *wrap*: use LCGs pequenos (ex.: `seed*75+74 % 65537`) em vez de `1103515245`.

## Contrato do script (tudo opcional)
```lua
function init() end        -- uma vez, após carregar
function update(dt) end    -- dt em segundos
function draw() end        -- dentro de SpriteBatch2D::BeginVirtual(640, 480)
```

## API
| Módulo | Funções |
| :--- | :--- |
| `gfx` | `clear(r,g,b)`, `rect(x,y,w,h,r,g,b[,a])`, `text(x,y,str,r,g,b[,escala])`, `size() -> 640, 480` |
| `input` | `connected([pad])`, `down(btn)`, `pressed(btn)`, `stick(0\|1) -> x, y` — `btn`: `up down left right a b x y l r start back home` |
| `save` | `write(slot, str) -> bool`, `read(slot) -> str\|nil`, `write_async(slot, str) -> bool`, `status() -> "idle"\|"pending"\|"done"\|"failed"` (slots 0..15, máx. 4096 bytes, CRC32) |
| `app` | `quit()`, `log(x)`, `sdcard() -> bool` |

Cores 0..255. `save.*` usa `SaveGameManager`: para gravar no SD (`fs:/card0/udata`) inicialize-o com `preferSdCard = true`.

## Uso em C++
```cpp
zsdk::script::LuaVM vm;
vm.Initialize();
vm.Bind(&batch, &saveMgr);
vm.LoadString("game.lua", src, len);   // OnInit
vm.Update(dt);                        // OnUpdate
batch.BeginVirtual(640, 480, w, h); vm.Draw(); batch.End();   // OnRender
```
Exemplo completo: [`examples/04_lua_vm_demo/main.cpp`](../examples/04_lua_vm_demo/main.cpp) (jogo "Star Catcher" inteiro em Lua, melhor pontuação salva em slot).

## Scripts em arquivo, `require` e `print`

* `vm.SetVfs(&vfs); vm.LoadFile("script:/main.lua");` carrega o script pelo `Vfs` (Zeebo: `fs:/~/scripts/main.lua`).
* `require("sub.util")` carrega `script:/sub/util.lua` uma vez (cache); pontos viram subpastas. Só aceita `[A-Za-z0-9_.-]` e rejeita `..`. Exige `SetVfs()`.
* `print(...)` vai para o `Logger` (tag `LUA`). O `print` padrão do Lua escreve em stdout, que não existe no Zeebo e travava o app (o stub `abort()` do CRT é um laço infinito).
* O `04_lua_vm_demo` embute `scripts/star_catcher.lua` no build (CMake gera `star_catcher_lua.h`) e o usa se `script:/star_catcher.lua` não existir.

Hook opcional `function on_pad(index, connected)`: chamado quando um gamepad entra ou sai (encaminhe `IAppDelegate::OnGamepadConnected/Disconnected` para `LuaVM::NotifyGamepad`). O `star_catcher.lua` pausa e mostra "CONNECT A CONTROLLER" sem o pad 0.

## Relação com os bindings `bindings/lua`

* `bindings/lua` (`zsdk_lua`) expõe a C ABI para um `lua_State` do próprio jogo (`luaopen_zsdk`); `LuaVM` (`zsdk_lua_runtime` + `src/script`) é o *host*: traz o Lua 5.4 vendorizado, sandbox, limite de memória, `require` por `script:/` e o carregamento de arquivos. São complementares.
* Com `-DZSDK_BUILD_LUA_BINDINGS=ON` e `ZSDK_ENABLE_LUA_VM=ON`, `bindings/lua` usa os headers do Lua vendorizado (`third_party/lua`) se `ZSDK_LUA_INCLUDE_DIR` estiver vazio.
* A API `gfx`/`input`/`save`/`app` do `LuaVM` é enxuta e própria; unificá-la com o módulo `zsdk.*` (registrar `luaopen_zsdk` no `LuaVM`) está no `TODO.md`.
