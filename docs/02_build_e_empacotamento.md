# 02 Build e empacotamento

O ZSDK compila a mesma base C++14 para dois alvos, e a diferença entre eles está no empacotamento: no Zeebo o resultado é um par `.mod` e `.mif` que o BREW carrega, no Windows é um `.exe` comum. Quem escolhe o caminho é a opção `ZSDK_BUILD_ZEEBO`, que troca os fontes do backend, as flags de compilação e o pós-build. As quatro macros de [cmake/ZsdkBrew.cmake](../cmake/ZsdkBrew.cmake) escondem a diferença entre linguagens: cada uma injeta o host certo e deixa o diretório do seu app com apenas o código do jogo. Este capítulo mostra o que cada macro gera, o que precisa existir na máquina antes do primeiro `cmake`, onde cada artefato aparece e por que o pós-build só funciona no Windows. Se você só quer ver algo na tela, comece pelo tutorial em [01 Primeiros passos](./01_primeiros_passos.md).

## Visão rápida

| Símbolo | O que produz |
| :--- | :--- |
| `zsdk_add_brew_app` | app C++14: `.elf` linkado com `elf2mod.x`, depois `.mod` e `.mif` |
| `zsdk_add_c_app` | app C99: acrescenta `src/c/host/zsdk_c_host.cpp` ao alvo acima |
| `zsdk_add_lua_app` | app Lua: usa `bindings/lua/host/zsdk_lua_host.cpp` e copia ou compila o script |
| `zsdk_add_rust_app` | app Rust: chama `cargo build --release` e linka o `staticlib` ao alvo acima |
| `libzsdk.a` | biblioteca estática com o núcleo, a ponte C e o backend do alvo |
| `<alvo>.mod` | módulo dinâmico BREW gerado por `tools/elf2mod/elf2mod.exe` |
| `<alvo>.mif` | manifesto com class ID, nome, empresa e privilégios `0x07AF` |

Todos os artefatos de um app caem sob `CMAKE_BINARY_DIR/apps/<alvo>/`, porque `RUNTIME_OUTPUT_DIRECTORY` é fixado nesse caminho para os dois alvos. O `.elf` fica na raiz dessa pasta, o `.mod` em `mod/<alvo>/` e o `.mif` em `mif/`.

## Pré-requisitos

### Toolchain ARM

O build de Zeebo usa `arm-none-eabi-gcc` e `arm-none-eabi-g++` da newlib, além de `arm-none-eabi-ar` para a biblioteca estática. O CMake não traz arquivo de toolchain pronto: você declara o compilador na linha de comando, junto com `-DCMAKE_SYSTEM_NAME=Generic` e `-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`. A última existe para que o teste de compilação do CMake aceite uma toolchain sem biblioteca C de execução.

### SDK BREW e extensões Zeebo

O build aponta quatro diretórios dentro de `BREW_SDK_ROOT`, e cada um resolve uma dependência diferente. Faltando qualquer um deles, o erro aparece como header não encontrado em `third_party/brew_crt` ou em `src/platform/zeebo`.

| Caminho sob `BREW_SDK_ROOT` | O que fornece |
| :--- | :--- |
| `platform_402/BREW 4.0.2 SP19/sdk/inc` | headers públicos do BREW 4.0.2: `AEEShell.h`, `AEEStdLib.h`, `AEEIHID.h`, `AEEVCodes.h` |
| `platform_402/BREW 4.0.2 SP19/inc` | headers referenciados por caminho relativo, como `X.h` usado em `AEERSA.h` |
| `ogles_ext/unpacked/sdk/inc` | headers da extensão OpenGL ES do Zeebo, usados pelo contexto e pelas texturas comprimidas |
| `zeebo_msi_extracted/BREW402SP09 IHID Extension Folder/sdk/inc` | extensão IHID do Z-Pad e do Boomerang |

O caminho padrão de `BREW_SDK_ROOT` no `CMakeLists.txt` aponta para a máquina de quem escreveu a SDK (`C:/Users/Esther/AppData/Local/Programs/brew-sdk`). Em qualquer outra máquina passe `-DBREW_SDK_ROOT=...` explicitamente, ou o configure falha no primeiro `#include` do CRT.

### Ferramentas de empacotamento

`tools/elf2mod/elf2mod.exe` já está versionado no repositório e não precisa compilar. O `cifc.exe` vem do SDK, e a macro o procura em dois lugares: `toolset/Toolset 7.10 Rev 12.0.1549664/bin` e `platform_402/BREW 4.0.2 SP19/bin`. Se o `find_program` falhar, o build não para: ele copia o `.cif` textual para um arquivo com extensão `.mif`, o que não serve para o console. Confira a mensagem `Packaging mif/... via cifc` no log para saber qual dos dois caminhos rodou.

### Opcionais

Python 3 é necessário apenas para o alvo `zsdk_check_bindings` e para o teste `zsdk_bindings_parity`; sem ele, `ZSDK_ENABLE_TESTS` não registra a verificação. Rust exige `cargo` no `PATH` e, para o alvo de console, a toolchain `armv5te-unknown-linux-gnueabi` instalada por `rustup target add`. MSVC 2022 não é necessário para nada que esteja validado, como explica a seção de limites.

## Conceitos

### O que cada macro acrescenta

As três macros de linguagem não são atalhos cosméticos: cada uma resolve um problema de entrada diferente.

`zsdk_add_brew_app` é a base e faz o trabalho pesado. Ela junta seus `SOURCES` aos nove arquivos de `third_party/brew_crt` (`AEEModGen.c`, `AEEAppGen.c`, `AEEHIDButtons.c`, `AEEHIDThumbsticks.c`, `AEEMediaUtil.c`, `EGL_1x.c`, `GLES_1x.c`, `glibc_stubs.c`, `mod_malloc.c`), define `DYNAMIC_APP`, aponta os quatro diretórios do SDK e liga o resultado a `zsdk`, `-lm`, `-lc` e `-lgcc`. O produto é um `.elf` com sufixo trocado, e não um `.mod`: o BREW não consome ELF.

`zsdk_add_c_app` e `zsdk_add_rust_app` compartilham o mesmo host, que só encaminha para `zsdk_c_app_init`, `update`, `render`, `swap` e `shutdown` de [zsdk_c_app.h](../include/zsdk/c/zsdk_c_app.h). A diferença é que o host Rust também define `rust_eh_personality` como função vazia, porque o código C++ é compilado sem unwinding e o `staticlib` do Rust referencia esse símbolo.

`zsdk_add_lua_app` é a única que não recebe `SOURCES`. Ela usa `bindings/lua/host/zsdk_lua_host.cpp`, que cria o estado Lua, registra o módulo `zsdk`, carrega `main.luac` ou `main.lua` e chama `zsdk_on_init`, `zsdk_on_update` e `zsdk_on_render`. O script é copiado para o diretório do app no build e, no Zeebo, para dentro de `mod/<alvo>/`, que é onde o host procura `fs:/~/main.lua` em tempo de execução.

As três macros de linguagem retornam sem criar alvo quando falta o pré-requisito: Lua exige `TARGET zsdk_lua` e Rust exige `cargo` encontrado. Não há mensagem de erro nesse caminho, então um app que desaparece do build tem uma dessas duas causas.

### O `.mif` e os privilégios

O `.mif` não é um arquivo de configuração do CMake: é um script de recursos `.cif` preenchido por `configure_file` a partir de [tools/mif/zsdk_template.mif.in](../tools/mif/zsdk_template.mif.in) e transformado em binário pelo `cifc`. Ele declara quatro recursos de texto (nome do app no id 1000, empresa no 6, copyright no 7, versão no 8) e um bloco `Applet` com o `appletid`, o host, as permissões e as dependências.

Os privilégios são `0x07AF`, que o próprio template decompõe em `PL_FILE`, `PL_NETWORK`, `PL_TAPI`, `PL_WEB`, `PL_ADDRBOOK`, `PL_DOWNLOAD`, `PL_POS_LOCATION`, `PL_RINGER` e `PL_SECTORINFO`. O comentário do template registra a ausência deliberada de `PL_SHARED_WRITE`: sem ela, o app não escreve na NAND compartilhada, e é isso que combina com o guarda do `FileSystem`. A dependência `0x0101414E` é `AEECLSID_CARD0_ACCESS`, necessária para o prefixo `fs:/card0/`.

### A cadeia do `.elf` ao `.mod`

O link não usa o script padrão do linker, usa [tools/elf2mod/elf2mod.x](../tools/elf2mod/elf2mod.x), e três decisões desse script explicam o resultado. A primeira é que a seção `.text.AEEMod_Load` vai para o início do arquivo em offset zero, porque o BREW procura o ponto de entrada ali. A segunda é `-Wl,-e,AEEMod_Load`, que fixa o símbolo de entrada. A terceira é que o script descarta a seção `.stack` e as de unwind `.ARM.exidx`; o stack do app pertence ao sistema, não ao módulo.

O `elf2mod.exe` lê esse ELF relocável e escreve o `.mod` com as relocações aplicadas no formato que o BREW espera. Por isso o link precisa de `-Wl,--emit-relocs`: sem a tabela de relocações o empacotador não tem o que reaplicar.

### O que o build Windows não faz

`ZSDK_BUILD_ZEEBO=OFF` troca o backend inteiro: em vez de dezenove arquivos em `src/platform/zeebo/`, o CMake compila apenas `src/platform/windows/windows_backend.cpp`. Não há `.elf`, `.mod` nem `.mif` nesse caminho, e o pós-build não roda. As consequências práticas são duas: o app vê um backend com partes incompletas (o [TODO.md](../TODO.md) lista `Thread` entre os stubs) e o carregamento de assets muda de raiz, porque não existe `fs:/~/`. Um app Lua lê `main.lua` do diretório de trabalho, um app que grava save escreve em `udata/` relativo. Nada disso invalida o uso do Windows como ambiente de desenvolvimento: o código do jogo é o mesmo e o ciclo no PC dispensa o empacotamento do `.mod`.

## Uso passo a passo

### Build para Windows

O `ZSDK_BUILD_ZEEBO` é `ON` por padrão, então um build de PC precisa desligá-lo explicitamente.

```powershell
cmake -S . -B build_windows -G Ninja -DZSDK_BUILD_ZEEBO=OFF -DZSDK_BUILD_EXAMPLES=ON
cmake --build build_windows
ctest --test-dir build_windows --output-on-failure
```

O executável não fica na raiz do build: `RUNTIME_OUTPUT_DIRECTORY` é `apps/<alvo>`, então o cubo sai em `build_windows/apps/01_spinning_cube/01_spinning_cube.exe`. O target `zsdk` linka `opengl32`, `user32`, `gdi32` e `winmm`, e o script de um app Lua vai para `apps/03_lua_spinning_cube/main.lua`, sem o prefixo `fs:/~/` que o host usa no console.

O `ctest` roda `zsdk_bindings_parity` e o alvo `zsdk_check_bindings` roda a cada build. Os dois executam [tools/check_bindings.py](../tools/check_bindings.py) sobre a árvore de código, o que significa que uma inconsistência de ABI quebra o build antes de qualquer coisa rodar.

### Build para Zeebo

```powershell
cmake -S . -B build_zeebo -G Ninja `
  -DCMAKE_SYSTEM_NAME=Generic `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ `
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY `
  -DBREW_SDK_ROOT=C:/caminho/para/brew-sdk `
  -DZSDK_BUILD_ZEEBO=ON
cmake --build build_zeebo
```

O build produz, para cada app declarado:

| Artefato | Caminho |
| :--- | :--- |
| biblioteca | `build_zeebo/libzsdk.a` |
| ELF relocável | `build_zeebo/apps/<alvo>/<alvo>.elf` |
| módulo | `build_zeebo/apps/<alvo>/mod/<alvo>/<alvo>.mod` |
| manifesto | `build_zeebo/apps/<alvo>/mif/<alvo>.mif` |
| script Lua | `build_zeebo/apps/<alvo>/mod/<alvo>/main.lua` ou `main.luac` |

Para instalar, copie o `.mif` para `/mif/` e o diretório `mod/<alvo>/` para `/mod/<alvo>/` no armazenamento do console. Os caminhos de upload que o projeto assume vêm do Zeebo Developer Guide e estão registrados no [TODO.md](../TODO.md).

### Build de validação com MinGW

O caminho Windows efetivamente exercitado é o cross-compile com MinGW-w64, que não exige Visual Studio e pode rodar a partir do mesmo repositório.

```powershell
cmake -S . -B build_mingw -G Ninja `
  -DCMAKE_SYSTEM_NAME=Windows `
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc `
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ `
  -DZSDK_BUILD_ZEEBO=OFF
cmake --build build_mingw
```

O [TODO.md](../TODO.md) registra esse build com zero erros e os exemplos `01` e `02` linkando; a demo Lua chegou a rodar sob `wine`, com `Vfs`, `script:` e log em arquivo funcionando e os gráficos não conferidos. É a base da afirmação de que o alvo Windows é viável, e não uma prova de que o MSVC compila o mesmo código.

### Conferindo os artefatos

Tomando o exemplo `01` como referência, um build de console bem-sucedido deixa quatro arquivos sob `build_zeebo/apps/01_spinning_cube/`: o `01_spinning_cube.elf` na raiz da pasta, o `01_spinning_cube.cif` ao lado dele, o `01_spinning_cube.mod` em `mod/01_spinning_cube/` e o `01_spinning_cube.mif` em `mif/`. O `.cif` fica junto do `.elf` porque `configure_file` o grava no mesmo diretório antes de o `cifc` rodar. Duas verificações rápidas substituem abrir o console: se a mensagem `Packaging mif/... via cifc` aparece no log, o manifesto saiu do compilador de recurso; se o `.mif` começa com `--`, ele é o `.cif` copiado, porque o template abre com um comentário de linha Lua e um MIF binário nunca começa assim.

### Adicionando um app novo

O `ZSDK_BUILD_EXAMPLES` é só um guarda em volta de quatro chamadas de macro no `CMakeLists.txt` raiz. Um app novo entra do mesmo jeito, depois da linha `include(cmake/ZsdkBrew.cmake)`, porque as funções só existem a partir dali.

```cmake
include(cmake/ZsdkBrew.cmake)

zsdk_add_brew_app(meu_jogo
    SOURCES src/main.cpp src/player.cpp
    MIF_CLASS_ID  0x01099010
    MIF_APP_NAME  "Meu Jogo"
    MIF_COMPANY   "Estudio"
    MIF_VERSION   "0.1.0"
)

zsdk_add_c_app(meu_jogo_c
    SOURCES src/game.c
    MIF_CLASS_ID 0x01099011
    MIF_APP_NAME "Meu Jogo C"
)

zsdk_add_lua_app(meu_jogo_lua
    SCRIPT assets/main.lua
    BYTECODE
    MIF_CLASS_ID 0x01099012
)

zsdk_add_rust_app(meu_jogo_rust
    MANIFEST rust/Cargo.toml
    LIB_NAME meu_jogo_rust
    MIF_CLASS_ID 0x01099013
)
```

Com o alvo declarado, o que você precisa escrever muda por linguagem:

| Macro | Arquivos que você escreve | Ponto de entrada |
| :--- | :--- | :--- |
| `zsdk_add_brew_app` | `main.cpp` com `ZSDK_IMPLEMENT_APP` | `IAppDelegate` |
| `zsdk_add_c_app` | `game.c` com as cinco funções `zsdk_c_app_*` | `zsdk_c_app_init` |
| `zsdk_add_lua_app` | `main.lua` e o argumento `SCRIPT` | `zsdk_on_init`, `zsdk_on_update`, `zsdk_on_render` |
| `zsdk_add_rust_app` | `Cargo.toml` com `crate-type = ["staticlib"]` e `src/lib.rs` | `zsdk_c_app_init` |

Três detalhes decidem se a declaração funciona. O primeiro é o `MIF_CLASS_ID`: todo app que não o informa recebe `0x01099001`, e dois módulos com o mesmo `appletid` disputam o mesmo registro no console. O segundo é a ordem: `zsdk_add_lua_app` precisa que `add_subdirectory(bindings/lua)` já tenha rodado, senão retorna sem criar nada. O terceiro é o nome: se você omite `MIF_APP_NAME`, o `.mif` mostra o nome do alvo, mas o host registra `ZSDK_C_App`, `ZSDK_Lua_App` ou `ZSDK_Rust_App`, porque a macro só define `ZSDK_C_APP_NAME`, `ZSDK_LUA_APP_NAME` e `ZSDK_RUST_APP_NAME` quando o argumento existe.

No app Rust, o caminho de `MANIFEST` é resolvido contra o diretório do `CMakeLists` que faz a chamada, e `LIB_NAME` é o nome do pacote no `Cargo.toml`, porque a macro procura `lib<LIB_NAME>.a` (ou `<LIB_NAME>.lib` no MSVC) dentro de `target/<triple>/release/`. No app Lua, a flag `BYTECODE` sem valor liga a compilação do script só para aquele alvo, sem mexer em `ZSDK_LUA_BYTECODE`.

## Referência da API

A "API" deste capítulo são as opções do CMake e os argumentos das macros. Tudo abaixo foi lido de [CMakeLists.txt](../CMakeLists.txt), de [cmake/ZsdkBrew.cmake](../cmake/ZsdkBrew.cmake) e de [bindings/lua/CMakeLists.txt](../bindings/lua/CMakeLists.txt).

### Opções CMake

| Opção | Padrão | Efeito |
| :--- | :--- | :--- |
| `ZSDK_BUILD_ZEEBO` | `ON` | `ON` compila backend Zeebo e empacota `.mod` e `.mif`; `OFF` usa o backend Windows e gera `.exe` |
| `ZSDK_BUILD_EXAMPLES` | `ON` | declara os quatro apps de exemplo |
| `ZSDK_ENABLE_LUA_VM` | `ON` | adiciona `src/script/lua_vm.cpp` e compila `third_party/lua` como `zsdk_lua_runtime` |
| `ZSDK_BUILD_LUA_BINDINGS` | `ON` | adiciona `bindings/lua`, criando o alvo `zsdk_lua` |
| `ZSDK_LUA_BYTECODE` | `OFF` | compila o `SCRIPT` dos apps Lua em `main.luac` stripped em vez de copiar o `.lua` |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | `ON` | define `ZSDK_ENABLE_DEBUG_OVERLAY` como `1` ou `0` em todo consumidor de `zsdk` |
| `ZSDK_ENABLE_LOGGING` | `ON` | define `ZSDK_ENABLE_LOGGING`; em `0`, `INFO` e `WARN` viram no-op |
| `ZSDK_ENABLE_TESTS` | `ON` | registra `zsdk_bindings_parity` e compila `tests/c_abi_smoke_test.c` como objeto C99 |
| `BREW_SDK_ROOT` | caminho da máquina de origem | raiz do SDK BREW; precisa ser sobrescrito em qualquer outra máquina |
| `ZSDK_LUA_INCLUDE_DIR` | vazio | força os headers de Lua de fora, em vez de usar `third_party/lua` |
| `ZSDK_RUST_TARGET` | `armv5te-unknown-linux-gnueabi` no Zeebo | triple do `cargo build`; vazio usa o alvo do host |

### Argumentos das macros

| Argumento | Macro | Padrão | Observação |
| :--- | :--- | :--- | :--- |
| `SOURCES` | brew, c | vazio | lista multi-valor; no app C o host é somado a ela |
| `MIF_CLASS_ID` | todas | `0x01099001` | `appletid` do `.mif` |
| `MIF_APP_NAME` | todas | nome do alvo | também vira `ZSDK_*_APP_NAME` no host |
| `MIF_COMPANY` | todas | `ZSDK` | recurso 6 do `.mif` |
| `MIF_COPYRIGHT` | todas | `ZSDK` | recurso 7 do `.mif` |
| `MIF_VERSION` | todas | `1.0.0` | recurso 8 do `.mif` |
| `SCRIPT` | lua | vazio | caminho do `.lua`; relativo é resolvido contra o diretório do `CMakeLists` que faz a chamada |
| `BYTECODE` | lua | `OFF` | flag sem valor; equivale a `-DZSDK_LUA_BYTECODE=ON` |
| `MANIFEST` | rust | obrigatório | caminho do `Cargo.toml` |
| `LIB_NAME` | rust | obrigatório | nome da `staticlib` gerada pelo `cargo` |

### Flags fixas

| Contexto | Flags |
| :--- | :--- |
| alvo `zsdk` | C++14 estrito, `-fno-exceptions`, `-fno-rtti`, `-fno-use-cxa-atexit`, `-fno-threadsafe-statics`, `-Wall -Wextra`, `-ffunction-sections -fdata-sections` |
| só no Zeebo | `-mlong-calls` no `zsdk` e no app; `-nostartfiles`, `--specs=nosys.specs`, `-Wl,--gc-sections`, `-Wl,--strip-debug` |
| MSVC | `/W4 /GR- /EHs-c-` |
| runtime Lua | `LUA_32BITS=1`, `LUA_USE_C89=1`, `LUAI_MAXCCALLS=48`, `-O2`, `-mlong-calls` no Zeebo |

O `LUAI_MAXCCALLS=48` não é preferência de estilo: o comentário no `CMakeLists.txt` registra que o Zeebo dá 32 KB de stack por app, e o padrão de 200 chamadas C aninhadas do Lua estoura esse espaço.

## Limites e armadilhas

O pós-build é Windows nativo. `zsdk_add_brew_app` chama `tools/elf2mod/elf2mod.exe` por caminho absoluto e o `cifc` do SDK também é um binário PE, então no Linux o build da biblioteca e do `.elf` termina, mas o empacotamento falha com `Permission denied`. O [TODO.md](../TODO.md) registra a pendência: hoje o `.mod` e o `.mif` são gerados à mão, com `wine` e os mesmos argumentos que o CMake passaria.

O fallback do `cifc` é silencioso e engana. Quando `find_program` não acha o compilador de recurso, a macro executa `copy_if_different` do `.cif` para `mif/<alvo>.mif` e escreve no log `cifc not found`. O arquivo resultante é texto de script, não o binário que o BREW espera; um `.mif` copiado assim não é um manifesto válido e o console recusa o app.

Bytecode Lua em build de console exige um build de PC antes. O alvo `zsdk_luac` só é criado quando `ZSDK_BUILD_ZEEBO` está `OFF`, e `zsdk_add_lua_app` com bytecode ligado procura o executável em `build_windows/tools`, `tools` e no diretório do build. Sem nenhum deles o configure para com `ZSDK_LUA_BYTECODE is ON, but zsdk_luac was not found`.

O script do linker não é dependência do link. O [TODO.md](../TODO.md) anota que editar `elf2mod.x` não força um novo link: é preciso apagar o `.elf` antes de reconstruir, senão o `.mod` continua saindo com o layout antigo.

O `Makefile.zeebo` é um caminho antigo e incompleto. Ele compila apenas `libzsdk.a` com `-std=c++14 -O2`, e a lista de fontes não inclui `src/c/*.cpp`, `src/graphics/gl_extensions.cpp`, `src/audio/pcm_mixer.cpp`, `src/fs/vfs.cpp` nem `src/debug/file_log.cpp`. Serve para gerar a biblioteca em um ambiente sem CMake, não para produzir um app.

O tamanho do `.mod` conta contra o orçamento de memória. O [TODO.md](../TODO.md) registra cerca de 169 KB para o cubo e cerca de 398 KB para uma demo Lua; são medições antigas, mas a ordem de grandeza continua valendo, porque o binário inteiro é carregado no heap de 32 MB.

Nada disso foi validado no console. O [TODO.md](../TODO.md) diz que as execuções foram no zeebx-emu, que emula o AEE em alto nível, e que o build Windows validado é o cross-compile com MinGW-w64. O caminho MSVC nunca foi exercitado, e `windows_backend.cpp` ainda tem stubs, como o de `Thread`.

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `Permission denied` ao executar `elf2mod.exe` | pós-build rodando fora do Windows | empacote com `wine tools/elf2mod/elf2mod.exe -output X.mod X.elf` |
| `fatal error: AEEHIDButtons.h: No such file or directory` | header ausente no SDK disponível | veja [12 Solução de problemas](./12_solucao_de_problemas.md); o [TODO.md](../TODO.md) trata o caso como bloqueio de validação |
| `undefined reference to 'AEEMod_Load'` | alvo criado com `add_executable` em vez da macro | use `zsdk_add_brew_app` para não perder `AEEModGen.c` nem o script de linker |
| `.mif` instalado e app não aparece | `cifc` não encontrado e `.cif` copiado como `.mif` | instale o `cifc` do SDK em um dos dois `HINTS` e confirme a mensagem `via cifc` no log |
| `ZSDK_LUA_BYTECODE is ON, but zsdk_luac was not found` | alvo `zsdk_luac` só existe fora do build Zeebo | gere `zsdk_luac` em `build_windows` ou desligue `ZSDK_LUA_BYTECODE` |
| app Lua ou Rust não gera alvo e o build passa | macro retornou sem `TARGET zsdk_lua` ou sem `cargo` | declare o app depois de `add_subdirectory(bindings/lua)` e ponha `cargo` no `PATH` |
| `undefined reference to '_sbrk'` ou `'_write'` | link sem `--specs=nosys.specs` ou sem `glibc_stubs.c` | mantenha as opções padrão da macro, que já incluem os dois |
| `cannot find -lzsdk` em build de app separado | o alvo `zsdk` não está no mesmo build tree | declare o app no `CMakeLists.txt` raiz, depois do `include(cmake/ZsdkBrew.cmake)` |

## Veja também

- [01 Primeiros passos](./01_primeiros_passos.md): o mesmo caminho de build, começando do zero.
- [12 Solução de problemas](./12_solucao_de_problemas.md): sintomas de build e de carga no console, com a causa no código.
- [Bindings C, Lua e Rust](../bindings/README.md): o que o verificador de paridade compara e como portar uma função.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md): por que existem dois backends e quais limites o hardware impõe.
- [cmake/ZsdkBrew.cmake](../cmake/ZsdkBrew.cmake): a fonte de tudo o que este capítulo afirma.
