<div align="center">

# ZSDK

**SDK de plataforma para Zeebo e Qualcomm BREW 4.0.2, com paridade de código em Windows**

Camada de abstração leve e determinística que mantém headers BREW e OpenGL fora do código do jogo, com API em **C++14**, **C99**, **Lua 5.4** e **Rust no_std**, gerando `.mod` + `.mif` para o console e `.exe` para o PC.

<p>
  <img src="https://img.shields.io/badge/C%2B%2B-14-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++14" />
  <img src="https://img.shields.io/badge/C-99%20ABI-A8B9CC?style=flat-square&logo=c&logoColor=black" alt="C99 ABI" />
  <img src="https://img.shields.io/badge/Lua-5.4-2C2D72?style=flat-square&logo=lua&logoColor=white" alt="Lua 5.4" />
  <img src="https://img.shields.io/badge/Rust-no__std-000000?style=flat-square&logo=rust&logoColor=white" alt="Rust no_std" />
  <img src="https://img.shields.io/badge/BREW-4.0.2-FF6F00?style=flat-square" alt="BREW 4.0.2" />
  <img src="https://img.shields.io/badge/OpenGL%20ES-1.1-5586A4?style=flat-square&logo=opengl&logoColor=white" alt="OpenGL ES 1.1" />
  <img src="https://img.shields.io/badge/Target-Zeebo%20%7C%20Windows-1E88E5?style=flat-square" alt="Zeebo | Windows" />
</p>

[Documentação](./docs/README.md) · [Primeiros passos](./docs/09_primeiros_passos.md) · [Build e empacotamento](./docs/01_build_e_empacotamento.md) · [Bindings](./bindings/README.md) · [Exemplos](./examples/)

</div>

## Visão geral

O ZSDK abstrai o hardware do Zeebo (ARM11 a 528 MHz, 32 MB de RAM e VRAM compartilhados, GPU Qualcomm Q3Dimension com OpenGL ES 1.1 de função fixa) mantendo a mesma árvore de código compilando no Windows para desenvolvimento e depuração rápida.

Quatro propriedades definem o desenho:

* **Isolamento de plataforma.** Nenhum header em `include/zsdk/` expõe `IShell`, `IDisplay`, `IHID`, `EGLDisplay` ou `GLenum`. O jogo inclui `zsdk/zsdk.hpp` e mais nada.
* **Determinismo.** Sem exceções (`-fno-exceptions`), sem RTTI (`-fno-rtti`), sem alocação dinâmica por frame em `InputManager`, `DebugOverlay` e `SaveGameManager`. Todo erro atravessa a API como `zsdk::Result`.
* **Proteção do console.** A trava `ZSDK_NAND_WRITE_GUARD` recusa escrita, remoção e criação de diretório fora de `fs:/~/` e `fs:/card0/`, e o `.mif` sai com privilégios `0x07AF`.
* **Uma API, quatro linguagens.** A ABI C99 é a base comum de C, Lua e Rust, com 302 funções verificadas por `tools/check_bindings.py`.

## Quick start

```powershell
cmake -S . -B build_windows -G Ninja -DZSDK_BUILD_ZEEBO=OFF -DZSDK_BUILD_EXAMPLES=ON
cmake --build build_windows
.\build_windows\apps\01_spinning_cube\01_spinning_cube.exe
```

Para o console, compilando com a toolchain ARM e o SDK BREW instalado:

```powershell
cmake -S . -B build_zeebo -G Ninja `
  -DCMAKE_SYSTEM_NAME=Generic `
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY `
  -DZSDK_BUILD_ZEEBO=ON
cmake --build build_zeebo
```

Os pré-requisitos completos, o conteúdo do `.mif` e a lista de opções CMake estão em [docs/01_build_e_empacotamento.md](./docs/01_build_e_empacotamento.md). Para começar do zero, siga o tutorial em [docs/09_primeiros_passos.md](./docs/09_primeiros_passos.md).

Verificação de saúde antes de abrir qualquer mudança:

```powershell
python tools/check_bindings.py
ctest --test-dir build_windows --output-on-failure
```

## Documentação

A documentação segue o Diátaxis: tutorial para aprender, guias por módulo para trabalhar, referência para consultar e explicação para entender.

| Módulo | Guia | Conteúdo principal |
| :--- | :--- | :--- |
| **Primeiros passos** | [09_primeiros_passos.md](./docs/09_primeiros_passos.md) | tutorial do clone ao app rodando nas quatro linguagens |
| **Build e empacotamento** | [01_build_e_empacotamento.md](./docs/01_build_e_empacotamento.md) | toolchain, macros CMake, `elf2mod`, `cifc`, `.mif`, MSVC |
| **Core e assíncrono** | [02_core_e_async.md](./docs/02_core_e_async.md) | `Application`, `FrameTimer`, `Clock`, ponto fixo, `Thread`, `SignalBus` |
| **Debug e profiler** | [03_debug_e_profiler.md](./docs/03_debug_e_profiler.md) | `MemoryTracker` de RAM e VRAM, `Logger`, `DebugOverlay` |
| **Input e controles** | [04_input_controles.md](./docs/04_input_controles.md) | Z-Pad, hotplug, deadzone, rumble, motion, teclado e mouse |
| **Gráficos 3D e 2D** | [05_graficos_3d_e_2d.md](./docs/05_graficos_3d_e_2d.md) | `GLContext`, `Renderer`, `Frame`, texturas, malhas, sprites |
| **Filesystem e save** | [06_filesystem_e_savegame.md](./docs/06_filesystem_e_savegame.md) | `FileSystem`, guarda de NAND, `Vfs`, save com CRC32, unzip |
| **Áudio, rede e segurança** | [07_audio_rede_seguranca.md](./docs/07_audio_rede_seguranca.md) | `AudioEngine`, `PcmMixer`, `Socket`, `HttpClient`, cripto e licença |
| **Scripting Lua** | [08_scripting_lua.md](./docs/08_scripting_lua.md) | `LuaVM` em sandbox, API exposta, `require`, bytecode |
| **Arquitetura** | [10_arquitetura_e_portabilidade.md](./docs/10_arquitetura_e_portabilidade.md) | camadas, backends, orçamento de memória, limites |
| **Referência da API** | [11_referencia_da_api.md](./docs/11_referencia_da_api.md) | mapa de headers, códigos de erro, macros, opções CMake |
| **Solução de problemas** | [12_solucao_de_problemas.md](./docs/12_solucao_de_problemas.md) | sintoma, causa e correção por área |
| **Bindings** | [bindings/README.md](./bindings/README.md) | ABI C99, bindings Lua e Rust, paridade verificada |

## Exemplos

Os quatro exemplos desenham a mesma cena, um cubo colorido girando com câmera e overlay de debug, cada um escrito apenas na linguagem alvo.

| Exemplo | Linguagem | Macro CMake | Ponto de entrada |
| :--- | :--- | :--- | :--- |
| **01: Spinning Cube** | C++14 (de 1 a 1024 cubos, menu, `udata`) | `zsdk_add_brew_app` | [`examples/01_spinning_cube/main.cpp`](./examples/01_spinning_cube/main.cpp) |
| **02: C Spinning Cube** | C99 puro | `zsdk_add_c_app` | [`examples/02_c_spinning_cube/cube_c.c`](./examples/02_c_spinning_cube/cube_c.c) |
| **03: Lua Spinning Cube** | Lua 5.4 puro | `zsdk_add_lua_app` | [`examples/03_lua_spinning_cube/main.lua`](./examples/03_lua_spinning_cube/main.lua) |
| **04: Rust Spinning Cube** | Rust `#![no_std]` | `zsdk_add_rust_app` | [`examples/04_rust_spinning_cube/src/lib.rs`](./examples/04_rust_spinning_cube/src/lib.rs) |

## Estrutura do projeto

```text
ZSDK/
├── bindings/               bindings Lua (zsdk_lua) e Rust (zsdk-sys, zsdk)
├── cmake/                  macros zsdk_add_{brew,c,lua,rust}_app em ZsdkBrew.cmake
├── docs/                   documentação modularizada em PT-BR
├── examples/               01 (C++), 02 (C99), 03 (Lua), 04 (Rust)
├── include/zsdk/           headers públicos C++14 e ABI C99 (include/zsdk/c/*.h)
├── src/                    núcleo C++, ponte C (src/c/) e backends (zeebo, windows)
├── tests/                  teste de compilação C99 pura
├── third_party/            brew_crt (AEEModGen, EGL_1x, GLES_1x) e Lua 5.4
└── tools/                  elf2mod, template .mif, check_bindings.py, docs2wiki.py
```

## Estado do projeto

A SDK foi exercitada no emulador e em builds Windows com MinGW-w64. Ela ainda não foi validada em hardware Zeebo real, depende de headers HID que não estão disponíveis para conferir o mapa de botões, e a build com MSVC não foi testada. A lista completa de pendências, com prioridade e impacto, está em [TODO.md](./TODO.md), e o resumo está em [docs/10_arquitetura_e_portabilidade.md](./docs/10_arquitetura_e_portabilidade.md#limites-conhecidos).
