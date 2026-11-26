# 11 Referência da API

Esta página é o mapa da superfície pública da SDK: quais headers existem, o que cada classe e função faz, quais são os códigos de erro e quais macros mudam o comportamento do binário. Ela é para consulta, não para leitura linear. Se você está começando, o caminho é o [tutorial de primeiros passos](./09_primeiros_passos.md); se quer explicação de arquitetura, vá para [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md).

## Convenções da API

Três convenções valem para todo o código em `include/zsdk/` e explicam metade das dúvidas de quem chega.

**Erro é valor, não exceção.** Toda função que pode falhar devolve `zsdk::Result`, um `enum class` de `int32_t` em que `Ok` é `0` e todo erro é negativo. Use `Succeeded(r)` e `Failed(r)` em vez de comparar com `Ok`, porque a comparação numérica continua válida se novos códigos forem adicionados.

**Objeto que abre, objeto que fecha.** `GLContext`, `File`, `Database`, `Thread`, `Signal`, `Texture`, `MeshBuffer`, `Bitmap2D` e `UnzipStream` seguem RAII: o construtor não adquire recurso, `Initialize` ou `Open` adquire, e o destrutor libera. Se você não chamar `Shutdown` explicitamente, o destrutor chama. O padrão existe para que o caminho de erro não vaze recurso em código sem exceção.

**A plataforma é um `void*` opaco.** Funções que precisam do contexto do sistema recebem `void* platformContext`, que no Zeebo é um `IShell*` vindo de `Application::GetNativePlatformContext()`. O tipo é opaco de propósito: nenhum header público menciona BREW.

### Códigos de resultado

| Código | Valor | Quando aparece |
| :--- | :--- | :--- |
| `Result::Ok` | `0` | sucesso |
| `Result::Error` | `-1` | falha genérica do backend |
| `Result::InvalidArg` | `-2` | ponteiro nulo, tamanho zero, caminho vazio |
| `Result::OutOfMemory` | `-3` | alocação recusada, inclusive pelo orçamento estrito |
| `Result::NotInitialized` | `-4` | método chamado antes de `Initialize` |
| `Result::AlreadyInitialized` | `-5` | `Initialize` chamado duas vezes |
| `Result::NotSupported` | `-6` | recurso ausente no backend, comum no Windows |
| `Result::NotFound` | `-7` | arquivo, chave ou recurso inexistente |
| `Result::WouldBlock` | `-8` | operação não bloqueante sem dados |
| `Result::Timeout` | `-9` | espera esgotada |
| `Result::IoError` | `-10` | falha de leitura ou escrita |
| `Result::DeviceDisconnected` | `-11` | controle ou dispositivo removido |
| `Result::GraphicsError` | `-12` | erro de EGL ou GLES |
| `Result::AudioError` | `-13` | erro de mixer ou driver de áudio |
| `Result::NetworkError` | `-14` | falha de socket, DNS ou HTTP |

Fonte: [types.hpp](../include/zsdk/core/types.hpp).

## Mapa de headers

A tabela abaixo é o índice por domínio. A coluna "Capítulo" aponta para o guia que ensina a usar o módulo.

| Domínio | Headers em `include/zsdk/` | Classes e tipos principais | Capítulo |
| :--- | :--- | :--- | :--- |
| Core | `core/app.hpp`, `clock.hpp`, `types.hpp`, `math_fixed.hpp`, `signal.hpp`, `thread.hpp`, `system_info.hpp`, `config.hpp` | `Application`, `IAppDelegate`, `AppConfig`, `FrameTimer`, `Clock`, `Thread`, `Signal`, `SignalBus`, `AsyncCallback`, `SystemInfo`, `Fixed16` | [02](./02_core_e_async.md) |
| Debug | `debug/memory_tracker.hpp`, `logger.hpp`, `debug_overlay.hpp`, `file_log.hpp`, `embedded_font.hpp` | `MemoryTracker`, `MemorySnapshot`, `Logger`, `DebugOverlay`, `FileLogSink` | [03](./03_debug_e_profiler.md) |
| Input | `input/input.hpp`, `input_types.hpp`, `keyboard_mouse.hpp`, `virtual_keyboard.hpp` | `InputManager`, `GamepadState`, `AnalogConfig`, `MotionState`, `KeyboardMouseManager`, `VirtualKeyboard` | [04](./04_input_controles.md) |
| Gráficos | `graphics/gl_context.hpp`, `renderer.hpp`, `renderer_types.hpp`, `vertex_formats.hpp`, `texture.hpp`, `mesh_buffer.hpp`, `frame.hpp`, `frustum.hpp`, `sprite_batch.hpp`, `sprite2d.hpp`, `display2d.hpp`, `image_decoder.hpp` | `GLContext`, `Renderer`, `Frame`, `Texture`, `MeshBuffer`, `SpriteBatch2D`, `SpriteEngine2D`, `Sprite2D`, `Frustum`, `ImageDecoder` | [05](./05_graficos_3d_e_2d.md) |
| Filesystem | `fs/filesystem.hpp`, `vfs.hpp`, `savegame.hpp`, `database.hpp`, `archive_unzip.hpp`, `ram_cache.hpp`, `local_storage.hpp` | `FileSystem`, `File`, `Vfs`, `SaveGameManager`, `Database`, `UnzipStream`, `RamCache`, `LocalStorage` | [06](./06_filesystem_e_savegame.md) |
| Áudio | `audio/audio_engine.hpp`, `audio.hpp`, `pcm_mixer.hpp` | `AudioEngine`, `MediaStream`, `SoundEffect`, `PcmMixer`, `Voice` | [07](./07_audio_rede_seguranca.md) |
| Rede | `net/socket.hpp`, `http_client.hpp`, `network_mgr.hpp`, `bluetooth.hpp` | `Socket`, `HttpClient`, `NetworkManager`, `BluetoothManager` | [07](./07_audio_rede_seguranca.md) |
| Segurança | `security/crypto.hpp`, `license.hpp` | `Random`, `Hash`, `Hmac`, `Cipher`, `Rsa`, `LicenseManager`, `LicenseStatus` | [07](./07_audio_rede_seguranca.md) |
| Scripting | `script/lua_vm.hpp` | `LuaVM` | [08](./08_scripting_lua.md) |
| ABI C99 | `c/zsdk_c.h` e os oito headers por módulo | funções `zsdk_*` e handles opacos | [Bindings](../bindings/README.md) |

### Detalhe por módulo

**Core.** `Application` é o host do jogo: guarda `AppConfig`, o `IAppDelegate`, o `InputManager`, o `FrameTimer` e o estado de plataforma. `ZSDK_IMPLEMENT_APP(DelegateClass, AppName, TargetFps, EnableDebugOverlay)` define a fábrica `CreateUserApp` que o backend chama na entrada do módulo. `Clock` dá tempo monotônico e de parede, `FrameTimer` calcula delta e FPS sem alocar. `Signal` e `SignalBus` embrulham `ISignal` e `ISignalBus` do BREW, e `AsyncCallback` adia execução para o laço principal sem criar thread.

**Debug.** `MemoryTracker` é estático e contabiliza RAM por subsistema mais VRAM, com `SetStrictBudget` para simular o teto do console no PC. `Logger` é estático, sem alocação, escreve em `DBGPRINTF` ou `stdout`, no ring buffer do overlay e em até quatro sinks extras. `DebugOverlay` desenha o painel no framebuffer atual. `FileLogSink` grava linha a linha pelo `Vfs` e para sozinho depois de `maxBytes`.

**Input.** `InputManager` mantém até `ZSDK_MAX_GAMEPADS` (padrão 4) `GamepadState`, atualizados uma vez por frame. `AnalogConfig` define deadzone e saturação dos sticks. `VirtualKeyboard` desenha um teclado na metade inferior da tela virtual de 640x480 e devolve o texto digitado.

**Gráficos.** `GLContext` cria e destrói EGL e GLES. `Renderer` é o wrapper de comandos com cache de estado e constantes de matriz. `Frame` é RAII de um frame inteiro: limpa, permite um passe 2D e troca buffers na destruição, inclusive em retorno antecipado. `Texture` e `MeshBuffer` são handles com contabilização de VRAM. `SpriteBatch2D` agrupa sprites em um único draw; `SpriteEngine2D`, `Font2D` e `HardwareBlitter2D` usam as extensões de 2D do BREW; `Frustum` faz culling por esfera e AABB.

**Filesystem.** `FileSystem` fala com `IFileMgr` e concentra a guarda de NAND. `Vfs` adiciona prefixos portáveis (`udata:`, `script:`, `home:`, `sd:`) e recusa `..` em caminho montado. `SaveGameManager` padroniza slots com header e CRC32 e tem caminho assíncrono por frame. `Database` embrulha `IDatabase` e `IDBMgr`.

**Áudio.** `AudioEngine` mistura uma trilha de fundo via `MediaStream` e até `kMaxSfxChannels` efeitos via `SoundEffect`. `PcmMixer` mistura e reamostra PCM 16 bits para saída estéreo intercalada, com vozes gerenciadas por `Voice`.

**Rede.** `Socket` é não bloqueante sobre `ISockPort`. `HttpClient` cobre requisição sobre `IWeb` e `ISource`. `NetworkManager` informa disponibilidade e Wi-Fi. `BluetoothManager` cobre o rádio e o perfil SPP.

**Segurança.** `Random` usa `IRandomSource` com fallback `GETRAND`. `Hash` cobre MD5 e SHA-1, `Hmac` cobre HMAC-MD5 e HMAC-SHA1, `Cipher` embrulha `ICipher1` e `Rsa` embrulha `IRSA`. `LicenseManager::CheckLicense` devolve `LicenseStatus` com validade e dias restantes.

**Scripting.** `LuaVM` embute Lua 5.4 com `LUA_32BITS`, limite de memória de 2 MiB (`kDefaultLuaMemoryLimit`) e sandbox que abre só `base`, `table`, `string`, `math`, `utf8` e `coroutine`. O contrato do script é opcional: `init`, `update(dt)` e `draw`.

## ABI C99

A ABI expõe a mesma funcionalidade em C puro, com handles opacos, e é a base dos bindings. O verificador reporta o estado atual:

```text
[ZSDK BINDINGS CHECK OK] 8 subsystems, 13 enums 1:1 parity, 302 C ABI functions,
297/302 Lua, 301/302 Rust-sys, 300/302 Rust-safe, 256/273 C++ methods bridged.
```

| Header | Cobre |
| :--- | :--- |
| `c/zsdk_c_core.h` | aplicação, configuração, tempo, matemática fixa, sistema |
| `c/zsdk_c_graphics.h` | contexto GL, renderer, textura, malha, sprite batch, frustum, display 2D |
| `c/zsdk_c_input.h` | gamepads, sticks, rumble, teclado e mouse, teclado virtual |
| `c/zsdk_c_fs.h` | sistema de arquivos, Vfs, save, database, RAM cache, unzip |
| `c/zsdk_c_audio.h` | engine, stream, efeitos, mixer PCM |
| `c/zsdk_c_net.h` | socket, HTTP, rede, bluetooth |
| `c/zsdk_c_security.h` | hash, HMAC, random, RSA, licença |
| `c/zsdk_c_log.h` | log e sinks |
| `c/zsdk_c_app.h` | ciclo de vida de app em C puro |
| `c/zsdk_c_types.h` | tipos, handles e enums compartilhados |

Rode `python tools/check_bindings.py` depois de qualquer mudança na ABI: ele compara C contra Lua e Rust e falha quando uma função fica sem par. Detalhes de nomenclatura e de como portar função nova estão em [Bindings](../bindings/README.md).

## Macros de configuração

Definidas em [config.hpp](../include/zsdk/core/config.hpp) e sobrescrevíveis por `-D` no CMake.

| Macro | Padrão | Efeito |
| :--- | :--- | :--- |
| `ZSDK_PLATFORM_ZEEBO` | detectado | `1` em ARM/BREW, `0` no backend Windows |
| `ZSDK_PLATFORM_WINDOWS` | detectado | `1` em `_WIN32` sem `AEE_SIMULATOR` |
| `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES` | `32 MiB` | orçamento de referência do profiler |
| `ZSDK_MAX_GAMEPADS` | `4` | controles simultâneos, P1 a P4 |
| `ZSDK_NAND_WRITE_GUARD` | `1` | bloqueia escrita fora de `fs:/~/` e `fs:/card0/` |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | `1`, ou `0` com `NDEBUG` | remove overlay e logs `INFO`/`WARN` do binário |
| `ZSDK_ENABLE_MEMORY_TRACKING` | igual ao overlay | liga a contabilização do `MemoryTracker` |
| `ZSDK_ENABLE_LOGGING` | `1` | liga `ZSDK_LOG_INFO` e `ZSDK_LOG_WARN` |
| `ZSDK_DEBUG_LOG_CAPACITY` | `32` | linhas no ring buffer do overlay |
| `ZSDK_DEBUG_LOG_LINE_LEN` | `96` | caracteres por linha de log |
| `ZSDK_FORCE_DEBUG_OVERLAY` | indefinida | força o overlay mesmo em build otimizada |

## Opções CMake

Declaradas no [CMakeLists.txt](../CMakeLists.txt) raiz.

| Opção | Padrão | Efeito |
| :--- | :--- | :--- |
| `ZSDK_BUILD_ZEEBO` | `ON` | `ON` gera ARM/BREW, `OFF` gera backend Windows |
| `ZSDK_BUILD_EXAMPLES` | `ON` | compila os quatro exemplos |
| `ZSDK_ENABLE_LUA_VM` | `ON` | embute o runtime Lua 5.4 |
| `ZSDK_BUILD_LUA_BINDINGS` | `ON` | compila a biblioteca modular `zsdk_lua` |
| `ZSDK_LUA_BYTECODE` | `OFF` | gera `main.luac` em vez de copiar `main.lua` |
| `ZSDK_ENABLE_TESTS` | `ON` | ativa paridade de bindings e teste de compilação C |
| `ZSDK_ENABLE_DEBUG_OVERLAY` | `ON` | overlay e logs no build |
| `ZSDK_ENABLE_LOGGING` | `ON` | logs `INFO` e `WARN` |

## Constantes que você vai encontrar

| Constante | Onde | Valor | Significado |
| :--- | :--- | :--- | :--- |
| `kFixedOne` | `math_fixed.hpp` | `0x00010000` | um inteiro em 16.16 |
| `kMaxMounts` | `vfs.hpp` | `8` | prefixos montados ao mesmo tempo |
| `kMaxPathLen` | `vfs.hpp` | `128` | caminho interno do `Vfs` |
| `kDefaultLuaMemoryLimit` | `lua_vm.hpp` | `2 MiB` | teto do alocador do Lua |
| `kLuaSaveMaxBytes` | `lua_vm.hpp` | `4096` | tamanho máximo de um save vindo de script |
| `kVirtualKeyboardMaxChars` | `virtual_keyboard.hpp` | `63` | caracteres no teclado virtual |
| `kMaxSfxChannels` | `audio_engine.hpp` | conforme o header | canais de efeito simultâneos |

## Veja também

- [Bindings](../bindings/README.md) para a ABI C99, Lua e Rust em detalhe.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md) para o porquê das camadas.
- [12 Solução de problemas](./12_solucao_de_problemas.md) para sintoma e correção por mensagem de erro.
- [zsdk.hpp](../include/zsdk/zsdk.hpp) para a lista completa de headers em uma tela.
