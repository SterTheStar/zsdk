# ZSDK

O ZSDK é uma camada de abstração de plataforma para o console Zeebo e para o Qualcomm BREW 4.0.2, escrita em C++14 sem exceções e sem RTTI, com a mesma base de código compilando para o console (`.mod` e `.mif`) e para o Windows (`.exe`). O jogo nunca inclui `IShell`, `IDisplay`, `IHID` ou header de OpenGL: inclui `zsdk/zsdk.hpp` e chama a SDK. Quem escreve em C99, Lua 5.4 ou Rust no_std usa a mesma API pela ABI C99.

Esta documentação é a fonte da verdade do projeto. A wiki de `docs/` é gerada a partir dela por `tools/docs2wiki.py`, então qualquer correção deve ser feita aqui, no repositório.

## O que a SDK entrega

| Área | Entrega concreta |
| :--- | :--- |
| Ciclo de vida | `Application` com `IAppDelegate`, laço de frame com alvo de 30 ou 60 FPS e callbacks de suspensão e retomada |
| Entrada | Z-Pad com até quatro controles, sticks com deadzone, rumble, motion, teclado e mouse no Windows, teclado virtual na tela |
| Gráficos | Contexto EGL e GLES 1.1, renderer com cache de estado, texturas com decodificação de PNG, JPEG e BMP, malhas, sprites 2D em lote, culling por frustum |
| Áudio | Engine com trilha de fundo e canais de efeito, mixer PCM 16 bits com reamostragem |
| Arquivos | `FileSystem` com guarda de NAND, `Vfs` com prefixos portáveis, save com CRC32 e gravação assíncrona, database e unzip |
| Rede | Socket não bloqueante, cliente HTTP, estado de rede e Wi-Fi, Bluetooth SPP |
| Segurança | Hash MD5 e SHA-1, HMAC, cifra simétrica, RSA, geração de aleatório e verificação de licença |
| Debug | `MemoryTracker` com RAM por subsistema e VRAM, logger com sinks, overlay de métricas dentro do jogo |
| Scripting | `LuaVM` com Lua 5.4 em sandbox, memória limitada a 2 MiB e `require` sobre o `Vfs` |
| Toolchain | Macros CMake que geram `.mod`, `.mif` ou `.exe` a partir das quatro linguagens, e um verificador de paridade de bindings |

## Para quem esta documentação é

Para quem vai programar um jogo ou ferramenta com a SDK e já sabe C++ ou C. Não ensinamos C++, nem OpenGL, nem o modelo de programação do BREW: explicamos como a SDK cobre essas bases e onde estão os limites do hardware. Se você nunca compilou um `.mod` na vida, comece pelo tutorial em [01 Primeiros passos](./01_primeiros_passos.md), que leva do clone ao app rodando sem exigir conhecimento prévio do console.

## Quick start

Compilar o exemplo de cubo girando para Windows, que é o caminho mais rápido para ver algo na tela:

```powershell
cmake -S . -B build_windows -G Ninja -DZSDK_BUILD_ZEEBO=OFF -DZSDK_BUILD_EXAMPLES=ON
cmake --build build_windows
.\build_windows\apps\01_spinning_cube\01_spinning_cube.exe
```

Para o console, troque o alvo por ARM e tenha o SDK BREW instalado:

```powershell
cmake -S . -B build_zeebo -G Ninja `
  -DCMAKE_SYSTEM_NAME=Generic `
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY `
  -DZSDK_BUILD_ZEEBO=ON
cmake --build build_zeebo
```

Os artefatos caem em `build_zeebo/apps/<alvo>/`, com o `.elf` na raiz da pasta, o `.mod` em `mod/<alvo>/` e o `.mif` em `mif/`. Os pré-requisitos completos de toolchain, o conteúdo do `.mif` e a lista de opções CMake estão em [02 Build e empacotamento](./02_build_e_empacotamento.md).

Verificação de saúde do repositório, útil antes de abrir qualquer mudança:

```powershell
python tools/check_bindings.py
ctest --test-dir build_windows --output-on-failure
```

## Mapa da documentação

A documentação segue a organização do Diátaxis: um tutorial para aprender, guias por módulo para trabalhar, uma referência para consultar e uma explicação para entender.

| Módulo | Guia | Conteúdo principal |
| :--- | :--- | :--- |
| **Primeiros passos** | [01_primeiros_passos.md](./01_primeiros_passos.md) | tutorial do zero ao app rodando, nas quatro linguagens |
| **Build e empacotamento** | [02_build_e_empacotamento.md](./02_build_e_empacotamento.md) | toolchain, CMake, `elf2mod`, `cifc`, `.mod`, `.mif`, build Windows |
| **Core e assíncrono** | [03_core_e_async.md](./03_core_e_async.md) | `Application`, `FrameTimer`, `Clock`, ponto fixo, `Thread`, `Signal` |
| **Debug e profiler** | [04_debug_e_profiler.md](./04_debug_e_profiler.md) | `MemoryTracker`, `Logger`, `DebugOverlay`, `FileLogSink` |
| **Input e controles** | [05_input_controles.md](./05_input_controles.md) | Z-Pad, hotplug, deadzone, rumble, teclado e mouse, teclado virtual |
| **Gráficos 3D e 2D** | [06_graficos_3d_e_2d.md](./06_graficos_3d_e_2d.md) | `GLContext`, `Renderer`, `Frame`, texturas, malhas, sprites, culling |
| **Filesystem e save** | [07_filesystem_e_savegame.md](./07_filesystem_e_savegame.md) | `FileSystem`, guarda de NAND, `Vfs`, save com CRC32, database, unzip |
| **Áudio, rede e segurança** | [08_audio_rede_seguranca.md](./08_audio_rede_seguranca.md) | `AudioEngine`, `PcmMixer`, `Socket`, `HttpClient`, cripto e licença |
| **Scripting Lua** | [09_scripting_lua.md](./09_scripting_lua.md) | `LuaVM`, sandbox, API exposta, `require`, bytecode |
| **Arquitetura e portabilidade** | [10_arquitetura_e_portabilidade.md](./10_arquitetura_e_portabilidade.md) | camadas, backends, orçamento de memória, limites do hardware |
| **Referência da API** | [11_referencia_da_api.md](./11_referencia_da_api.md) | mapa de headers, códigos de erro, macros e opções CMake |
| **Solução de problemas** | [12_solucao_de_problemas.md](./12_solucao_de_problemas.md) | sintoma, causa e correção por área |
| **Bindings C, Lua e Rust** | [../bindings/README.md](../bindings/README.md) | ABI C99, bindings Lua e Rust, paridade verificada |

## Exemplos

Cada exemplo desenha a mesma cena 3D, um cubo colorido girando com câmera e overlay, usando apenas a linguagem alvo.

| Exemplo | Linguagem | Macro CMake | Entrada |
| :--- | :--- | :--- | :--- |
| 01 Spinning Cube | C++14 | `zsdk_add_brew_app` | [main.cpp](../examples/01_spinning_cube/main.cpp) |
| 02 C Spinning Cube | C99 | `zsdk_add_c_app` | [cube_c.c](../examples/02_c_spinning_cube/cube_c.c) |
| 03 Lua Spinning Cube | Lua 5.4 | `zsdk_add_lua_app` | [main.lua](../examples/03_lua_spinning_cube/main.lua) |
| 04 Rust Spinning Cube | Rust no_std | `zsdk_add_rust_app` | [lib.rs](../examples/04_rust_spinning_cube/src/lib.rs) |

## Estado do projeto

A SDK foi exercitada no emulador e nas builds Windows com MinGW-w64. Ela não foi validada em hardware Zeebo real, o mapa de botões do controle ainda depende de headers HID que não estão disponíveis, e o build com MSVC não foi testado. Os pontos abertos, com prioridade e impacto, estão no [TODO.md](../TODO.md) e estão resumidos em [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md#limites-conhecidos). Leia essa lista antes de assumir que um recurso funciona no console.

## Estrutura do repositório

```text
ZSDK/
├── bindings/               bindings Lua e Rust sobre a ABI C99
├── cmake/                  macros zsdk_add_*_app em ZsdkBrew.cmake
├── docs/                   esta documentação
├── examples/               quatro exemplos, um por linguagem
├── include/zsdk/           headers públicos C++14 e ABI C99 em include/zsdk/c
├── src/                    núcleo portável, ponte C e backends zeebo e windows
├── tests/                  teste de compilação da ABI C99
├── third_party/            runtime C do BREW e Lua 5.4
└── tools/                  elf2mod, template de .mif, verificador de bindings, docs2wiki
```
