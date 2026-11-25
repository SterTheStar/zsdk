<div align="center">

# ZSDK

**SDK Base Multi-Linguagem para Zeebo & Qualcomm BREW 4.0.2**

Camada de abstração de plataforma (*PAL*) leve, determinística e livre de headers BREW/GL no código do jogo — com suporte nativo a **C++14**, **C99**, **Lua 5.4** e **Rust (`#![no_std]`)**, gerando builds para **Zeebo (`.mod` + `.mif`)** e **Windows (`.exe`)**.

<p>
  <img src="https://img.shields.io/badge/C%2B%2B-14-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++14" />
  <img src="https://img.shields.io/badge/C-99%20ABI-A8B9CC?style=flat-square&logo=c&logoColor=black" alt="C99 ABI" />
  <img src="https://img.shields.io/badge/Lua-5.4-2C2D72?style=flat-square&logo=lua&logoColor=white" alt="Lua 5.4" />
  <img src="https://img.shields.io/badge/Rust-no__std-000000?style=flat-square&logo=rust&logoColor=white" alt="Rust no_std" />
  <img src="https://img.shields.io/badge/BREW-4.0.2-FF6F00?style=flat-square" alt="BREW 4.0.2" />
  <img src="https://img.shields.io/badge/OpenGL%20ES-1.1-5586A4?style=flat-square&logo=opengl&logoColor=white" alt="OpenGL ES 1.1" />
  <img src="https://img.shields.io/badge/Target-Zeebo%20%7C%20Windows-1E88E5?style=flat-square" alt="Zeebo | Windows" />
</p>

[Documentação](./docs/README.md) · [Build & Empacotamento](./docs/01_build_e_empacotamento.md) · [Bindings C / Lua / Rust](./bindings/README.md) · [Exemplos](./examples/)

</div>

---

## Visão Geral

O **ZSDK** abstrai o hardware do **Zeebo** (ARM11 @ 528 MHz, GPU Qualcomm Q3Dimension, 32 MB RAM/VRAM) mantendo paridade de código-fonte com **Windows** para desenvolvimento e depuração rápida.

* **Isolamento total:** o código do jogo nunca inclui `IShell`, `IDisplay`, `IHID`, `EGLDisplay` ou headers OpenGL diretamente.
* **Desempenho determinístico:** sem exceções (`-fno-exceptions`), sem RTTI (`-fno-rtti`), zero alocações por frame nos loops críticos e rastreamento de RAM/VRAM via `MemoryTracker`.
* **Proteção da NAND:** trava `ZSDK_NAND_WRITE_GUARD` bloqueando escritas fora de `fs:/~/` e `fs:/card0/` (SD Card), com privilégios `.mif` seguros (`0x07AF` + `CARD0_ACCESS`).
* **ABI C99 única:** `231` funções C estáveis servindo como base comum para **C**, **Lua** e **Rust**, verificadas automaticamente por `tools/check_bindings.py`.

---

## Linguagens & Exemplos

Cada exemplo demonstra a mesma cena 3D (cubo colorido giratório com câmera, sombra e `DebugOverlay`) usando apenas a linguagem-alvo, sem boilerplate C++ no diretório do app:

| Exemplo | Linguagem | Macro CMake | Ponto de Entrada |
| :--- | :--- | :--- | :--- |
| **01 — Spinning Cube** | C++14 (`1–1024` cubos, menu, `udata`) | `zsdk_add_brew_app` | [`examples/01_spinning_cube/main.cpp`](./examples/01_spinning_cube/main.cpp) |
| **02 — C Spinning Cube** | C99 puro | `zsdk_add_c_app` | [`examples/02_c_spinning_cube/cube_c.c`](./examples/02_c_spinning_cube/cube_c.c) |
| **03 — Lua Spinning Cube** | Lua 5.4 puro | `zsdk_add_lua_app` | [`examples/03_lua_spinning_cube/main.lua`](./examples/03_lua_spinning_cube/main.lua) |
| **04 — Rust Spinning Cube** | Rust `#![no_std]` | `zsdk_add_rust_app` | [`examples/04_rust_spinning_cube/src/lib.rs`](./examples/04_rust_spinning_cube/src/lib.rs) |

---

## Módulos & Documentação

| Módulo | Recursos Principais | Guia |
| :--- | :--- | :--- |
| **Build & Empacotamento** | CMake (`zsdk_add_{brew,c,lua,rust}_app`), `elf2mod`, `cifc`, `.mod` + `.mif` e MSVC | [01_build_e_empacotamento.md](./docs/01_build_e_empacotamento.md) |
| **Core & Assíncrono** | `Application`, `ZSDK_IMPLEMENT_APP`, `FpsCap`, `Clock`, `Fixed16`, `Thread`, `SignalBus` | [02_core_e_async.md](./docs/02_core_e_async.md) |
| **Debug & Profiler** | `MemoryTracker` (RAM por subsistema + VRAM), `Logger` e `DebugOverlay` | [03_debug_e_profiler.md](./docs/03_debug_e_profiler.md) |
| **Input & Controles** | Z-Pad (`IHID`), hotplug, deadzone, Boomerang, `SetRumble`, XInput e teclado/mouse | [04_input_controles.md](./docs/04_input_controles.md) |
| **Gráficos 3D & 2D** | `GLContext`, `Renderer` (State Cache), `SpriteBatch2D`, `Frustum`, `Texture`, `MeshBuffer` | [05_graficos_3d_e_2d.md](./docs/05_graficos_3d_e_2d.md) |
| **Filesystem & SaveGame** | `FileSystem`, `ZSDK_NAND_WRITE_GUARD`, `SaveGameManager` (`udata`/SD + CRC32), `Vfs`, `UnzipStream` | [06_filesystem_e_savegame.md](./docs/06_filesystem_e_savegame.md) |
| **Áudio, Rede & Segurança** | `AudioEngine`, `PcmMixer`, `MediaStream`, `Socket`, `HttpClient`, `NetworkManager`, Crypto & Licença | [07_audio_rede_seguranca.md](./docs/07_audio_rede_seguranca.md) |
| **Scripting Lua** | `LuaVM` (Lua 5.4 em sandbox 32-bit), `require` via `Vfs` e `zsdk_lua` | [08_scripting_lua.md](./docs/08_scripting_lua.md) |
| **C ABI & Bindings** | Headers C99 (`include/zsdk/c/`), bindings Lua (`228/231`) e Rust `zsdk-sys`/`zsdk` (`230/231`) | [bindings/README.md](./bindings/README.md) |

---

## Quick Start

### Zeebo (`.mod` + `.mif`)
```powershell
cmake --build build_zeebo
```

### Windows (`.exe`)
```powershell
cmake --build build_windows
```

### Verificação de Paridade (C / Lua / Rust)
```powershell
python tools/check_bindings.py
ctest --test-dir build_windows --output-on-failure
```

> Veja os pré-requisitos completos de toolchain ARM, BREW SDK, Rust e MSVC em [`docs/01_build_e_empacotamento.md`](./docs/01_build_e_empacotamento.md).

---

## Estrutura do Projeto

```text
ZSDK/
├── bindings/               # Bindings Lua (zsdk_lua) e Rust #![no_std] (zsdk-sys, zsdk)
├── cmake/                  # Toolchain BREW ARM & macros zsdk_add_{brew,c,lua,rust}_app
├── docs/                   # Documentação modularizada em PT-BR
├── examples/               # 01 (C++), 02 (C99), 03 (Lua), 04 (Rust)
├── include/zsdk/           # Headers públicos C++14 e ABI C99 (include/zsdk/c/*.h)
├── src/                    # Núcleo C++, ponte C (src/c/) e backends (zeebo / windows)
├── tests/                  # Teste de compilação C99 pura (c_abi_smoke_test.c)
├── third_party/            # brew_crt (AEEModGen, EGL_1x, GLES_1x) e Lua 5.4.7
└── tools/                  # elf2mod, template .mif (0x07AF) e check_bindings.py
```
