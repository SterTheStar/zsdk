# ZSDK — SDK Base Moderno em C++ para Zeebo & Qualcomm BREW

O **ZSDK** é uma camada de abstração de plataforma (*Platform Abstraction Layer* — PAL) escrita em **C++14** sem exceções (`-fno-exceptions`), sem RTTI (`-fno-rtti`) e focada em desempenho determinístico para o hardware do **Zeebo** (ARM11 @ 528 MHz, 32 MB RAM/VRAM, GPU Qualcomm Q3Dimension / Adreno 130, BREW 4.0.2) — mantendo compatibilidade de código-fonte com **Windows (PC)** para desenvolvimento e testes rápidos.

---

## Princípios de Arquitetura

* **Ponte 100% Isolada:** Nenhum header público em `include/zsdk/` expõe `IShell`, `IDisplay`, `AECHAR`, `IHID`, `EGLDisplay` ou `GLenum`. O jogo inclui apenas `#include "zsdk/zsdk.hpp"`.
* **Zero-Overhead & Previsibilidade:** Uso de tipos de largura fixa (`uint8_t`, `int32_t`), `ZSDK_FORCE_INLINE`, RAII leve e zero alocações dinâmicas por frame em loops críticos (`InputManager`, `DebugOverlay`, `SaveGameManager`).
* **Proteção de Hardware:** Trava `ZSDK_NAND_WRITE_GUARD` bloqueando escritas fora de `fs:/~/` e `fs:/card0/`, além de privilégios `.mif` seguros (`0x07AF`, sem `PL_SHARED_WRITE`).

---

## Índice da Documentação

| Módulo | Guia | Conteúdo Principal |
| :--- | :--- | :--- |
| **Build & `.mod`/`.mif`** | [01_build_e_empacotamento.md](./01_build_e_empacotamento.md) | CMake, `zsdk_add_{brew,c,lua,rust}_app`, `elf2mod`, `cifc`, `.mif` e build Windows |
| **Core & Assíncrono** | [02_core_e_async.md](./02_core_e_async.md) | `Application`, `ZSDK_IMPLEMENT_APP`, `FpsCap`, `Clock`, `Fixed16`, `Thread`, `Signal`, `SystemInfo` |
| **Debug & Profiler** | [03_debug_e_profiler.md](./03_debug_e_profiler.md) | `MemoryTracker` (RAM/VRAM), `Logger` e `DebugOverlay` (`L + Baixo`) |
| **Input & Controles** | [04_input_controles.md](./04_input_controles.md) | Z-Pad (`IHID`), Boomerang (motion), Rumble, XInput (Xbox) e `KeyboardMouseManager` |
| **Gráficos 3D & 2D** | [05_graficos_3d_e_2d.md](./05_graficos_3d_e_2d.md) | `GLContext`, `Renderer` (com State Cache), `SpriteBatch2D`, `Frustum`, `Texture`, `MeshBuffer`, `Display2D`, `ImageDecoder`, `Sprite2D` |
| **Filesystem & Save** | [06_filesystem_e_savegame.md](./06_filesystem_e_savegame.md) | `FileSystem`, `ZSDK_NAND_WRITE_GUARD`, `SaveGameManager` (`udata` + CRC32, SD Card `fs:/card0/`), `Database`, `UnzipStream` |
| **Áudio, Rede & Crypto** | [07_audio_rede_seguranca.md](./07_audio_rede_seguranca.md) | `AudioEngine`, `PcmMixer`, `Socket`, `HttpClient`, `NetworkManager`, `BluetoothManager`, Crypto & `LicenseManager` |
| **Scripting Lua** | [08_scripting_lua.md](./08_scripting_lua.md) | `LuaVM` (Lua 5.4 sandbox), API `gfx`/`input`/`save`/`app` e `zsdk_lua` (`03_lua_spinning_cube`) |
| **C ABI & Bindings** | [../bindings/README.md](../bindings/README.md) | ABI C99 (`include/zsdk/c/`), Lua (`bindings/lua/`) e Rust `#![no_std]` (`bindings/rust/`) |

---

## Estrutura de Pastas

```text
ZSDK/
├── bindings/                     # Bindings Lua (zsdk_lua) e Rust (zsdk-sys + zsdk)
├── cmake/ZsdkBrew.cmake          # Macros zsdk_add_{brew,c,lua,rust}_app (.elf -> .mod + .mif / .exe)
├── tools/
│   ├── check_bindings.py         # Verificador de paridade C ABI / Lua / Rust
│   ├── elf2mod/                  # elf2mod.exe + elf2mod.x (linker script GNU ARM)
│   └── mif/zsdk_template.mif.in  # Template CIF -> MIF (0x07AF + CARD0_ACCESS)
├── third_party/brew_crt/         # Runtime C oficial BREW (AEEModGen.c, EGL_1x.c, GLES_1x.c...)
├── include/zsdk/                 # Headers públicos C++14 e ABI C99 (include/zsdk/c/*.h)
├── src/                          # Implementações core, ponte C (src/c/) + plataformas (zeebo / windows)
├── examples/
│   ├── 01_spinning_cube/         # Exemplo oficial C++ (1–1024 cubos + Menu START + DebugOverlay)
│   ├── 02_c_spinning_cube/       # Exemplo em C99 puro (cube_c.c)
│   ├── 03_lua_spinning_cube/     # Exemplo em Lua puro (main.lua)
│   └── 04_rust_spinning_cube/    # Exemplo em Rust #![no_std] (src/lib.rs)
└── docs/                         # Esta documentação
```
