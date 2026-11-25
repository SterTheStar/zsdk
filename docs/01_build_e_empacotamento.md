# 01 — Build e Empacotamento (Zeebo `.mod`/`.mif` e Windows `.exe`)

O **ZSDK** automatiza todo o processo de compilação e empacotamento através do arquivo [ZsdkBrew.cmake](../cmake/ZsdkBrew.cmake), permitindo gerar binários para **Zeebo** e **Windows** a partir do mesmo código-fonte.

---

## 1. Build para Zeebo (`arm-none-eabi-g++` + `elf2mod` + `cifc`)

### Pré-requisitos
* Toolchain ARM (`arm-none-eabi-gcc` / `arm-none-eabi-g++`)
* `CMake` + `Ninja`
* SDK BREW/Zeebo em `C:/Users/Esther/AppData/Local/Programs/brew-sdk` (ou configure `-DBREW_SDK_ROOT=...`)

### Comandos
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

### Saídas geradas em `build_zeebo/`
* `libzsdk.a` — Biblioteca estática do ZSDK.
* `01_spinning_cube.elf` — Executável ELF relocável linkado com `tools/elf2mod/elf2mod.x`.
* `mod/01_spinning_cube/01_spinning_cube.mod` — Módulo executável BREW gerado pelo `tools/elf2mod/elf2mod.exe` (estrutura padrão BREW pronta para copiar para o console/SD).
* `mif/01_spinning_cube.mif` — Manifesto BREW compilado pelo `cifc.exe` a partir de `tools/mif/zsdk_template.mif.in`.

> **Nota de Runtime C++:** O arquivo [AEEModGen.c](../third_party/brew_crt/AEEModGen.c) foi adaptado para colocar `AEEMod_Load` no offset `0x00000000` (`.text.AEEMod_Load`) e executar os construtores globais C++ (`__preinit_array` e `__init_array`) automaticamente no carregamento do módulo.

---

## 2. Build para Windows (MSVC 2022 x64)

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake -S . -B build_windows -G Ninja -DZSDK_BUILD_ZEEBO=OFF -DZSDK_BUILD_EXAMPLES=ON && cmake --build build_windows'
```

Saídas em `build_windows/`: `zsdk.lib` e `01_spinning_cube.exe` (linkado com `opengl32`, `user32`, `gdi32`, `winmm`).

---

## 3. Criando um Novo Jogo (`C++`, `C99`, `Lua` ou `Rust`)

O [ZsdkBrew.cmake](../cmake/ZsdkBrew.cmake) oferece 4 macros conforme a linguagem do projeto:

* **`zsdk_add_brew_app`**: app em C++14 usando `IAppDelegate` + `ZSDK_IMPLEMENT_APP`.
* **`zsdk_add_c_app`**: app em **C99 puro** (implementa `zsdk_c_app_*` sem precisar de `main.cpp`, via `src/c/host/zsdk_c_host.cpp`).
* **`zsdk_add_lua_app`**: app em **Lua puro** (`main.lua`, via `bindings/lua/host/zsdk_lua_host.cpp`). Aceita a flag opcional `BYTECODE` (ou `-DZSDK_LUA_BYTECODE=ON`) para compilar para `main.luac` via `tools/zsdk_luac.c`; o host tenta carregar `main.luac` primeiro e faz fallback para `main.lua`.
* **`zsdk_add_rust_app`**: app em **Rust `#![no_std]`** (`Cargo.toml` `staticlib` + `src/lib.rs`, via `bindings/rust/host/zsdk_rust_host.cpp`; no Zeebo usa `armv5te-unknown-linux-gnueabi` com `-C relocation-model=static`).

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

---

## 4. Flags de Compilação Importantes (`config.hpp`)

| Flag | Padrão | Descrição |
| :--- | :--- | :--- |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | `1` (Debug) / `0` (Release) | Compila ou remove totalmente o `DebugOverlay` e logs `INFO`/`WARN` |
| `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` | `32 MiB` | Orçamento exibido na barra do profiler |
| `ZSDK_NAND_WRITE_GUARD` | `1` | Bloqueia escritas fora de `fs:/~/` e `fs:/card0/` no Zeebo |
| `ZSDK_MAX_GAMEPADS` | `4` | Quantidade máxima de controles simultâneos |


## Opções CMake e exemplos

| Opção | Padrão | Efeito |
| :--- | :--- | :--- |
| `ZSDK_BUILD_ZEEBO` | ON | ON = alvo Zeebo/BREW (ARM); OFF = backend Windows |
| `ZSDK_BUILD_EXAMPLES` | ON | Compila `01_spinning_cube` (C++), `02_c_spinning_cube` (C99), `03_lua_spinning_cube` (Lua) e `04_rust_spinning_cube` (Rust) |
| `ZSDK_ENABLE_LUA_VM` | ON | Embute o runtime Lua 5.4 (`zsdk::script::LuaVM`) |
| `ZSDK_BUILD_LUA_BINDINGS` | ON | Compila a biblioteca modular `zsdk_lua` (`bindings/lua/`) |
| `ZSDK_LUA_BYTECODE` | OFF | Compila scripts `zsdk_add_lua_app` para bytecode stripped (`main.luac`) via `zsdk_luac` (`LUA_32BITS=1`) |
| `ZSDK_ENABLE_TESTS` | ON | Habilita `zsdk_bindings_parity` (`tools/check_bindings.py`) e `zsdk_c_abi_compile_test` |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | ON | Overlay de debug e logs INFO/WARN |

> Consulte [`bindings/README.md`](../bindings/README.md) para detalhes da ABI C99, bindings Lua/Rust e verificação de paridade com `python tools/check_bindings.py`.

### Build fora do Windows (Linux)

* O pós-build chama `tools/elf2mod/elf2mod.exe` diretamente e falha com "Permission denied" no Linux; a biblioteca e o `.elf` linkam normalmente. Empacote à mão: `wine tools/elf2mod/elf2mod.exe -output X.mod X.elf` e `wine cifc.exe -mif X.mif X.cif`.
* `AEERSA.h` inclui `..\..\inc\X.h` com barras invertidas: copie `AEEIBase.h` e `AEECallback.h` para `sdk/inc` com o nome literal.
* Os headers `AEEHIDButtons.h`/`AEEHIDThumbsticks.h` não vêm no SDK que temos (veja `TODO.md`).
* Build Windows com MinGW (só validação): `-DZSDK_BUILD_ZEEBO=OFF -DCMAKE_SYSTEM_NAME=Windows` com `x86_64-w64-mingw32-gcc/g++`.
