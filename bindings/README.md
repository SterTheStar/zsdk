# ZSDK Language Bindings & C ABI

O ZSDK expõe uma **ABI C99 estável** como camada base única, permitindo integração direta em **C**, **Lua** e futura expansão limpa para **Rust**, **Zig**, **Python**, etc.

## Arquitetura em Camadas

```text
  [ C ]          [ Lua ]           [ Rust (futuro) ]
    │               │                     │
    │         bindings/lua/         bindings/rust/
    │       (C99 + lua.h/lauxlib)   (zsdk-sys / bindgen)
    │               │                     │
    └───────────────┴─────────────────────┘
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
* **`src/c/`** (implementação C++ da ponte `extern "C"`):
  * `zsdk_c_log.cpp`, `zsdk_c_core.cpp`, `zsdk_c_input.cpp`, `zsdk_c_audio.cpp`, `zsdk_c_graphics.cpp`, `zsdk_c_fs.cpp`, `zsdk_c_net.cpp`, `zsdk_c_security.cpp`.
* **`bindings/lua/`** (módulo Lua em C99 puro, compatível com Lua 5.1–5.4 e LuaJIT):
  * `include/zsdk_lua.h` — `luaopen_zsdk` + `luaopen_zsdk_{log,core,input,audio,graphics,fs,net,security}`.
  * `src/zsdk_lua.c` e `src/zsdk_lua_*.c` — Um arquivo por subsistema (`log`, `core`, `input`, `audio`, `graphics`, `fs`, `net`, `security`).
  * `CMakeLists.txt` — Ativado com `-DZSDK_BUILD_LUA_BINDINGS=ON -DZSDK_LUA_INCLUDE_DIR=...`.

## Uso Rápido

### Em C
```c
#include "zsdk/c/zsdk_c.h"

void meu_modulo_c(void) {
    ZSDK_C_LOG_INFO("GAME", "Uptime: %u ms", (unsigned)zsdk_clock_uptime_ms());
}
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

Para verificar/compilar o workspace Rust e o exemplo:
```bash
cargo check --manifest-path bindings/rust/Cargo.toml
cargo build --manifest-path examples/04_rust_spinning_cube/Cargo.toml --release
```

Para linkar contra a biblioteca estática `zsdk` compilada pelo CMake:
```bash
ZSDK_LINK_ZSDK=1 cargo build --manifest-path bindings/rust/Cargo.toml --release
```
