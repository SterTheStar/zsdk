<div align="center">

# ZSDK

**SDK Base Moderno em C++14 para Zeebo & Qualcomm BREW 4.0.2**

Camada de abstração de plataforma (*PAL*) leve, determinística e livre de headers BREW/GL no código do jogo, com build nativo para **Zeebo (`.mod` + `.mif`)** e **Windows (`.exe`)**.

<p>
  <img src="https://img.shields.io/badge/C%2B%2B-14%20(no--exceptions%20%7C%20no--rtti)-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++14" />
  <img src="https://img.shields.io/badge/Plataforma-Zeebo%20%7C%20Windows-1E88E5?style=for-the-badge" alt="Plataforma" />
  <img src="https://img.shields.io/badge/BREW-4.0.2-FF6F00?style=for-the-badge" alt="BREW 4.0.2" />
  <img src="https://img.shields.io/badge/Gr%C3%A1ficos-OpenGL%20ES%201.1-5586A4?style=for-the-badge&logo=opengl&logoColor=white" alt="OpenGL ES 1.1" />
  <img src="https://img.shields.io/badge/Hardware-ARM11%20%40%20528%20MHz-2E7D32?style=for-the-badge" alt="ARM11" />
</p>

[Documentação](./docs/README.md) • [Guia de Build](./docs/01_build_e_empacotamento.md) • [Gráficos 3D/2D](./docs/05_graficos_3d_e_2d.md) • [Exemplo Oficial](./examples/01_spinning_cube/main.cpp)

</div>

---

## Visão Geral

O **ZSDK** foi projetado para desenvolvimento de jogos e aplicações de alta performance no hardware do **Zeebo** (ARM11 @ 528 MHz, GPU Qualcomm Q3Dimension), permitindo compilar e depurar o mesmo código-fonte C++ no **Windows** com zero alterações.

* **Isolamento Total:** o jogo inclui apenas `#include "zsdk/zsdk.hpp"`, sem dependência direta de `IShell`, `IDisplay`, `IHID`, `EGLDisplay` ou headers OpenGL.
* **Baixo Consumo & Previsibilidade:** zero alocações dinâmicas por frame nos loops críticos, buffers pré-alocados e rastreamento de memória por subsistema via `MemoryTracker`.
* **Proteção de Hardware:** trava `ZSDK_NAND_WRITE_GUARD` bloqueando escritas fora de `fs:/~/` e `fs:/card0/`, além de privilégios `.mif` seguros (`0x07AF`).
* **Paridade Zeebo / Windows:** mesmo ciclo de vida (`IAppDelegate`), input, renderização, áudio, filesystem, savegame (`udata`) e rede.

---

## Módulos Implementados

| Módulo | Recursos Principais | Documentação |
| :--- | :--- | :--- |
| **Build & Empacotamento** | CMake (`zsdk_add_brew_app`, `zsdk_add_c_app`, `zsdk_add_lua_app`, `zsdk_add_rust_app`), `elf2mod`, `cifc`, `.mod` + `.mif` e MSVC Windows | [01_build_e_empacotamento.md](./docs/01_build_e_empacotamento.md) |
| **Core & Assíncrono** | `Application`, `ZSDK_IMPLEMENT_APP`, `FpsCap` (30/60), `Clock`, `FrameTimer`, `Fixed16`, `Thread`, `SignalBus` | [02_core_e_async.md](./docs/02_core_e_async.md) |
| **Debug & Profiler** | `MemoryTracker` (RAM por subsistema + VRAM), `Logger` e `DebugOverlay` (FPS real, Draws, Verts) | [03_debug_e_profiler.md](./docs/03_debug_e_profiler.md) |
| **Input & Controles** | Z-Pad (`IHID`), hotplug, deadzone, Boomerang (acelerômetro/giroscópio), `SetRumble`, XInput e teclado/mouse | [04_input_controles.md](./docs/04_input_controles.md) |
| **Gráficos 3D & 2D** | `GLContext` (EGL/WGL), `Renderer` com State Cache, `SpriteBatch2D`, `Frustum` Culling, `Texture`, `MeshBuffer`, `ImageDecoder` | [05_graficos_3d_e_2d.md](./docs/05_graficos_3d_e_2d.md) |
| **Filesystem & SaveGame** | `FileSystem` com `ZSDK_NAND_WRITE_GUARD`, `SaveGameManager` (`udata` + CRC32, SD Card `fs:/card0/`), `UnzipStream`, `LocalStorage`, `Database`, `Vfs` | [06_filesystem_e_savegame.md](./docs/06_filesystem_e_savegame.md) |
| **Áudio, Rede & Segurança** | `MediaStream`, `SoundEffect`, `AudioEngine`, `PcmMixer`, `Socket`, `HttpClient`, `NetworkManager`, `BluetoothManager`, Crypto & `LicenseManager` | [07_audio_rede_seguranca.md](./docs/07_audio_rede_seguranca.md) |
| **Scripting Lua** | `LuaVM` (Lua 5.4 em sandbox, 32 bits), API `gfx`/`input`/`save`/`app` e bindings modulares `zsdk_lua` (`03_lua_spinning_cube`) | [08_scripting_lua.md](./docs/08_scripting_lua.md) |
| **C ABI & Bindings (C/Lua/Rust)** | ABI C99 (`231` funções), bindings Lua (`228/231`), crates Rust `#![no_std]` `zsdk-sys` / `zsdk` (`230/231`) e `check_bindings.py` | [bindings/README.md](./bindings/README.md) |

---

## Exemplo Rápido (`01_spinning_cube`)

O exemplo oficial em [`examples/01_spinning_cube/main.cpp`](./examples/01_spinning_cube/main.cpp) demonstra renderização 3D de **1 até 1024 cubos** com *Frustum Culling*, *Backface Culling*, iluminação, *SpriteBatch2D*, menu interativo, persistência em `udata` e `DebugOverlay`:

* **`START`:** abre/fecha o menu de configurações (`Cube Count` 1–1024, `Frustum Cull`, `Face Cull`, `Lighting`, `FPS Cap`, `VSync`, `Spin Speed`, `Save Config`).
* **`START` + `D-Pad Baixo`:** exibe/oculta o `DebugOverlay` (FPS médio real, *frametime*, *Draw Calls*, vértices, RAM/VRAM e logs).
* **Analógico Esquerdo / D-Pad:** move a câmera para testar o *Frustum Culling* ao vivo.
* **Analógico Direito (ou `ZL`/`ZR`):** controla o zoom da câmera.

---

## Compilação Rápida

### Zeebo (`.mod` + `.mif`)
```powershell
cmake --build build_zeebo
```

### Windows (`.exe`)
```powershell
cmake --build build_windows
```

> Consulte o passo a passo completo de configuração de toolchain, CMake e empacotamento BREW em [`docs/01_build_e_empacotamento.md`](./docs/01_build_e_empacotamento.md).

---

## Estrutura do Projeto

```text
ZSDK/
├── bindings/               # Bindings oficiais sobre a C ABI: lua/ (zsdk_lua) e rust/ (zsdk-sys + zsdk)
├── cmake/                  # Toolchain BREW ARM & macros zsdk_add_{brew,c,lua,rust}_app
├── docs/                   # Documentação modularizada completa em PT-BR
├── examples/               # 01 cubo C++, 02 cubo C99 puro, 03 cubo Lua puro, 04 cubo Rust no_std
├── include/zsdk/           # Headers públicos C++14 e C99 (include/zsdk/c/*.h)
├── src/                    # Implementações core, ponte C (src/c/) e backends (zeebo / windows)
├── third_party/brew_crt/   # Runtime C oficial BREW (AEEModGen, EGL_1x, GLES_1x)
├── third_party/lua/        # Lua 5.4.7 (MIT, ver LICENSE e README.zsdk.md)
└── tools/                  # elf2mod, template .mif (0x07AF) e check_bindings.py
```
