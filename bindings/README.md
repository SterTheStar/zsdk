# ZSDK Language Bindings & C ABI

O ZSDK expõe uma **ABI C99 estável** como camada base única, permitindo integração direta em **C**, **Lua** e futura expansão limpa para **Rust**, **Zig**, **Python**, etc.

## Arquitetura em Camadas

```text
  [ C ]          [ Lua ]           [ Rust ]
    │               │                 │
    │         bindings/lua/     bindings/rust/
    │       (C99 + lua.h)     (zsdk-sys / zsdk)
    │               │                 │
    └───────────────┴─────────────────┘
                    │
                    ▼
          include/zsdk/c/*.h   <── ABI C99 estável (sem C++ / templates)
                    │
                    ▼
           src/c/*.cpp         <── Ponte fina para o núcleo C++ (src/*)
```

## Estrutura de Pastas

* **`include/zsdk/c/`** (headers públicos em C99 puro, sempre incluídos no target `zsdk`):
  * `zsdk_c.h` — Header guarda-chuva para projetos em C e FFI.
  * `zsdk_c_types.h` — Tipos (`zsdk_result_t`, `zsdk_log_level_t`) e macros `ZSDK_C_API` / `ZSDK_C_EXTERN_BEGIN`.
  * `zsdk_c_log.h` — `zsdk_log`, `zsdk_log_v`, `zsdk_log_msg` e macros `ZSDK_C_LOG_INFO` / `WARN` / `ERROR`.
  * `zsdk_c_core.h` — Relógio (`zsdk_clock_*`), memória (`zsdk_memory_*`), `SystemInfo` e `DebugOverlay`.
  * `zsdk_c_input.h` — `InputManager`, botões, analógicos e rumble.
  * `zsdk_c_audio.h` — `AudioEngine` e `PcmMixer` (pitch/sample-rate ao vivo).
  * `zsdk_c_graphics.h` — `GLContext`, `Texture`, `MeshBuffer` e `Renderer` (2D/3D).
  * `zsdk_c_fs.h` — `FileSystem`, `File`, `LocalStorage` e `SaveGameManager`.
  * `zsdk_c_net.h` — `NetworkManager` e `Socket` (TCP/UDP não-bloqueante).
  * `zsdk_c_security.h` — `Random`, `Hash` (MD5/SHA-1) e `LicenseManager`.
  * `zsdk_c_app.h` — Callbacks de entrada (`zsdk_c_app_init`, `update`, `render`, `swap`, `shutdown`) usados por `zsdk_add_c_app` e `zsdk_add_rust_app`.
* **`src/c/`** (implementação C++ da ponte `extern "C"`):
  * `zsdk_c_log.cpp`, `zsdk_c_core.cpp`, `zsdk_c_input.cpp`, `zsdk_c_audio.cpp`, `zsdk_c_graphics.cpp`, `zsdk_c_fs.cpp`, `zsdk_c_net.cpp`, `zsdk_c_security.cpp` e `host/zsdk_c_host.cpp`.
* **`bindings/lua/`** (módulo Lua em C99 puro, compatível com Lua 5.1–5.4 e LuaJIT):
  * `include/zsdk_lua.h` — `luaopen_zsdk` + `luaopen_zsdk_{log,core,input,audio,graphics,fs,net,security}`.
  * `src/zsdk_lua.c` e `src/zsdk_lua_*.c` — Um arquivo por subsistema (`log`, `core`, `input`, `audio`, `graphics`, `fs`, `net`, `security`).
  * `host/zsdk_lua_host.cpp` — Host genérico usado automaticamente por `zsdk_add_lua_app` (veja `examples/03_lua_spinning_cube/main.lua`): carrega `main.luac` (se gerado com `BYTECODE` / `-DZSDK_LUA_BYTECODE=ON`) ou `main.lua`, expõe `zsdk_platform_context` e chama `zsdk_on_init(w, h, platform_ctx)`, `zsdk_on_update(delta_ms, input)` e `zsdk_on_render(w, h)`.
  * `CMakeLists.txt` — Ativado com `-DZSDK_BUILD_LUA_BINDINGS=ON`.
* **`bindings/rust/`** (workspace `#![no_std]` com `zsdk-sys` + `zsdk`):
  * `host/zsdk_rust_host.cpp` — Host genérico usado automaticamente por `zsdk_add_rust_app` (veja `examples/04_rust_spinning_cube/`).

## Uso Rápido

### Em C (`zsdk_add_c_app`)
Implemente os 5 callbacks em `.c` puro (sem `main.cpp`, como em `examples/02_c_spinning_cube/cube_c.c`):
```c
#include "zsdk/c/zsdk_c.h"

int  zsdk_c_app_init(void* platform_ctx) { return 0; }
void zsdk_c_app_update(uint32_t delta_ms, const zsdk_input_t* input) { (void)delta_ms; (void)input; }
void zsdk_c_app_render(int32_t* out_w, int32_t* out_h) { (void)out_w; (void)out_h; }
void zsdk_c_app_swap(void) {}
void zsdk_c_app_shutdown(void) {}
```

### Em Lua
Registre no `lua_State*`:
```c
#include "zsdk_lua.h"
luaL_requiref(L, "zsdk", luaopen_zsdk, 1);
lua_pop(L, 1);
```
E use no script Lua:
```lua
zsdk.log.info("GAME", "Iniciado em " .. zsdk.core.uptime_ms() .. " ms")
zsdk.log.warn("AI", "Pathfinding lento")
zsdk.log.error("FS", "Arquivo nao encontrado")
```

## Rust (`#![no_std]`)
Os bindings em **Rust (`#![no_std]`)** estão completos em `bindings/rust/`:
* **`bindings/rust/zsdk-sys/`** — FFI `extern "C"` sobre `include/zsdk/c/*.h` (230/231 funções; apenas `zsdk_log_v` usa `va_list`).
* **`bindings/rust/zsdk/`** — Wrappers seguros `#![no_std]` com `Result<T, Error>` e `Drop` automático para todos os 8 subsistemas (`log`, `core_sys`, `input` + `KeyboardMouse`/`VirtualKeyboard`, `audio` + `PcmMixer`/`MediaStream`/`Sfx`, `graphics` + `GLContext`/`Texture`/`MeshBuffer`/`SpriteBatch`/`Frustum`/`renderer`, `fs` + `File`/`LocalStorage`/`SaveGameManager`/`RamCache`, `net` + `NetworkManager`/`Socket`/`HttpClient`, `security`).
* **`examples/04_rust_spinning_cube/`** — Exemplo completo `#![no_std]` (`staticlib`) demonstrando `GLContext`, `InputManager`, `renderer` 2D/3D e `log`.
* **`bindings/rust/zeebo-armv5te.json`** — Target spec Rust para **Zeebo (ARMv5TE soft-float EABI)**.

Para verificar/compilar o workspace Rust, o exemplo e a paridade de todos os bindings (`231` funções C ABI, `228/231` em Lua, `230/231` em Rust):
```bash
python tools/check_bindings.py
cargo check --manifest-path bindings/rust/Cargo.toml
cargo build --manifest-path examples/04_rust_spinning_cube/Cargo.toml --release
```

Para compilar para Zeebo (`armv5te-unknown-linux-gnueabi`):
```bash
rustup target add armv5te-unknown-linux-gnueabi
cmake --build build_zeebo
```

Para linkar contra a biblioteca estática `zsdk` compilada pelo CMake:
```bash
ZSDK_LINK_ZSDK=1 cargo build --manifest-path bindings/rust/Cargo.toml --release
```
