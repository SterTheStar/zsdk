# TODO / Dívida técnica

Itens abertos após Lua, Vfs, save assíncrono, Frame e sinks de log. Prioridade: A = bloqueia validação real, B = importante, C = melhoria.

## A — Bloqueia validação em hardware / SDK real

- [ ] **Headers HID reais.** `AEEHIDButtons.h` e `AEEHIDThumbsticks.h` não existem em nenhum SDK que temos. Os testes usaram shims provisórios no scratch (IDs `ZEEBO_BUTTON_*` = 101..111 inventados; `HID_THUMBSTICK_MIN/MAX` = int16). O mapeamento real de botões e a normalização dos sticks nunca foram validados. Pedir os headers à dona do SDK Zeebo (Esther) e conferir `MapHidButtonToLogical` em `src/platform/zeebo/zeebo_input.cpp` (inclui os `AEEHIDBUTTON_7/8/10/11/12` usados como TriggerLeft/Right, Start e thumbs).
- [ ] **Nada foi testado em hardware Zeebo.** Só no zeebx-emu (HLE). Os "60 FPS" vêm do `FrameTimer` dentro do emulador, não medem o console.
- [ ] **SD card em `fs:/card0/`.** *(wiki: o firmware referencia `fs:/card0/longcheerzeebo/autocopysdcardinfotoenand.dat`, então o mount existe; que apps possam **gravar** nele continua sem prova, e o wiki indica que só o diretório privado do app é gravável.)* O `SaveGameManager`/`Vfs` assumem esse caminho, mas não está confirmado que o BREW do Zeebo monte o cartão ali. Verificar nos docs do SDK ou em dumps; hoje `preferSdCard` nunca foi exercitado.
- [ ] **Build Windows com MSVC não testado.** Cross-compilado com MinGW-w64 (`-DZSDK_BUILD_ZEEBO=OFF`, 0 erros, `01` e `02` linkam) e o `02_lua_demo.exe` roda sob wine (Vfs, `script:`, log em arquivo OK; gráficos não conferidos). Falta MSVC de verdade. `windows_backend.cpp` tem `Thread` e outros stubs.

## A — Achados da auditoria contra o Zeebo Developer Guide 0.97 (§5.1, §4.3, §11.3, §11.7)

Fonte: `~/projects/zeebo-emulator/research/docs/sdk-extract/Zeebo SDK + BREW SDK 4.0.2 + BREW MP SDK/ZeeboDeveloperGuide0.97.pdf`. O emulador HLE **não** reproduz estas restrições, então nada disso foi exercitado.

- [ ] **Stack de apenas 32 KB** (§5.1); o guia manda evitar arrays/structs grandes locais e usar `IThread` para stack maior. Auditar o uso de stack do ZSDK e do Lua (o `LUAI_MAXCCALLS` foi reduzido a 48 no CMake e o estouro vira erro Lua "C stack overflow", mas o consumo real por nível não foi medido). Locais grandes no código novo: `char path[128]` em `Vfs`/`SaveGameManager`, `char line[96]` no `print`.
- [ ] **"A biblioteca C padrão não está disponível"** e "não há variáveis estáticas/globais" em módulos dinâmicos (§4.3; elf2mod suporta "algum uso" via fix-up, mas o guia desencoraja). O Lua usa libc do newlib (strtod, snprintf, setjmp/longjmp, qsort, memcpy…) linkada estaticamente no `.mod`, e o ZSDK usa globais/statics (`g_sinks`, `g_minLevel`, `g_zsdkUserDelegate`, `static char buf` em `save.read`). Funciona no emulador; **não validado no console**. Preferir as funções BREW (`MALLOC`, `MEMCPY`, `SNPRINTF`) onde possível e mover estado global para o heap/contexto do app.
- [ ] **Save: o guia só descreve `/mod/<jogo>/udata/` (NAND).** O SD aparece apenas como meio de upload de jogos (§11.7: `/mif`, `/mod/<jogo>`), nunca como destino de save. O caminho `fs:/card0/udata` do `SaveGameManager`/`Vfs` é **suposição nossa, sem respaldo no guia**. Tratar o save em SD como não suportado até prova em hardware.
- [ ] **Limite de 64 KB para dados de save** (§5.1 "at most 64KB") e filesystem de 1 GB compartilhado: o guia manda checar espaço livre antes de gravar. O `FileLogSink` (cap 64 KiB) mais os saves podem passar do limite; escolher cap menor/desligar log em release; checar espaço livre.
- [ ] **Heap de 32 MB, com o binário inteiro carregado nele** (§5.1): o `.mod` do `02` (~398 KB) e o limite de 2 MiB do Lua consomem esse orçamento. Memória do Lua **não passa pelo `MemoryTracker`** (aparece fora de `MEM:` no overlay); integrar sob um subsistema novo.
- [ ] **Botão Home = `AVK_CLR`** e clique do thumbstick direito = `AVK_SELECT` (§6.3.7/§6.4); Home longo (>3 s) volta ao menu (§11). Conferir o tratamento de `EVT_KEY` vs `AEEHIDButton_GetNextButtonEvent` em `zeebo_input.cpp` e o fluxo suspend/resume (§11.8, power button).
- [ ] **Área segura de 4–8 px** em todas as bordas (§11.2/§6): o demo (HUD em `x=10`, retângulos em `y=0`) não respeita. Ajustar HUD do `02` e do `01` (CRT).
- [ ] **O guia diz que `AEEHIDButtons.c`/`AEEHIDThumbsticks.c` ficam na pasta `src` do SDK; os `.h` deveriam vir junto.** Não temos os `.h` (ver A acima); procurar no `ZeeboSDKPackage-1.2.4`/`0.93` completos antes de pedir à Esther.

## B — Build e ferramentas

- [ ] **Flags ARM em builds não-MSVC.** *(corrigido nesta rodada)* `-mlong-calls` agora só é aplicado com `ZSDK_BUILD_ZEEBO`; antes quebrava qualquer build Windows com GCC/Clang.

- [ ] **Pós-build do CMake no Linux.** `zsdk_add_brew_app` chama `tools/elf2mod/elf2mod.exe` direto e falha com "Permission denied" fora do Windows. Detectar plataforma e usar `wine`, ou documentar. Hoje o empacotamento (`elf2mod` + `cifc`) é manual via wine.
- [ ] **Include dirs do SDK BREW.** `AEERSA.h` inclui `..\..\inc\X.h` com barras invertidas; no Linux precisa de cópias com o nome literal. Resolver no CMake (ou doc) em vez de gambiarra no scratch. O SDK montado no scratch é uma mescla BREW 4.0.2 + BREW MP SDK 7.12.5; documentar a receita exata.
- [ ] **`elf2mod.x`:** a seção `.ARM.exidx` foi removida e descartada. Revisar se algum código C/C++ futuro precisa de unwind. O passo de link não rastreia o script (precisa `rm *.elf` ao editar).
- [ ] **`third_party/lua/luaconf.h`** tem uma edição local (guard de `LUA_32BITS`). Reaplicar ao atualizar o Lua; nota em `third_party/lua/README.zsdk.md`.
- [ ] **Tamanho do `.mod`:** o `02_lua_demo` tem ~398 KB (vs ~169 KB do cubo). Medir quanto é Lua (`-Os`, remover libs não usadas como `utf8`/`coroutine` se não forem necessárias).

## B — Lua / scripting

- [ ] **Desempenho do Lua no ARM11** não medido (só 14M instruções / 300 frames em emulação). Fazer benchmark em hardware; avaliar `LUA_32BITS` + float vs. VFP.
- [ ] **Limite de memória do Lua (2 MiB) e sandbox** não testados em estresse (alocação até o limite, scripts maliciosos, laço infinito). Não há limite de instruções/tempo por frame (`lua_sethook` com contador).
- [ ] **Bindings mínimos.** Faltam: áudio, texturas/sprites (`gfx` só tem `rect`/`text`), `input` de Boomerang/motion, rumble, rede, tempo (`app.time()`), RNG decente (inteiros de 32 bits dão *wrap*; hoje o demo usa um LCG pequeno).
- [ ] **`gfx.text` faz `Flush()` do batch** a cada chamada (muitas chamadas = muitos draw calls). Considerar batcher de texto.
- [ ] **`save.read` usa buffer `static`** de 4 KiB (não reentrante). Trocar por buffer do `LuaVM`.
- [ ] **`require` lê o arquivo inteiro** via `Vfs::ReadAllBytes` a cada módulo novo (ok), mas não há como recarregar (hot-reload) nem listar módulos.
- [ ] **Script do demo embutido no build** gera `star_catcher_lua.h` com string raw; limite de ~16 KB por literal no MSVC. Trocar por array de bytes gerado se crescer.

## B — Vfs / save

- [ ] **`Vfs` e `SaveGameManager` têm `FileSystem` separados** e lógica de `udata` duplicada. Fazer o `SaveGameManager` usar o `Vfs` (`udata:`).
- [ ] **Save assíncrono não usa thread:** só diluiu a escrita por frame. Se `IFile_Write` bloquear mesmo em pedaços pequenos (NAND lenta), ainda trava o frame. Medir no hardware; avaliar `IThread`. `BeginSaveSlot` limita a 4 KiB e mantém o buffer no próprio objeto.
- [x] *(feito)* `FileSystem::Rename`/`GetFreeBytes`; save grava em `.tmp`, remove o antigo e renomeia (BREW não sobrescreve no `Rename`, então há uma janela entre Remove e Rename; o `LoadSlot` recupera o `.tmp`). Testado no zeebx-emu (sync e async, sobrescrita). A recuperação do `.tmp` após queda **não** foi testada.
- [ ] **`FileLogSink`:** *(flush corrigido nesta rodada: `File::Flush()` + flush por linha; no PC o log saía vazio se o processo morresse)*. Em BREW `Flush()` é no-op (suposição de que `IFILE_Write` não bufferiza). Sem rotação de log.
- [ ] **`Vfs` sem enumeração de diretório** por prefixo e sem montagem somente-leitura.
- [ ] **Aviso:** `SaveHeader` usa `struct` com layout nativo (endianness/padding não fixados).

## B — Lacunas vindas do zeebo-wiki (`~/projects/zeebo-wiki`)

Lidas: `AGENTS.md`, páginas 3 (runtime AEE), 6 (input), 8 (vídeo), 9 (áudio), 10 (storage), amostras de 2/13/14. **Não lidas em detalhe:** 4, 5, 7, 11, 12, 15 (online), 16 (exploits), 17 (USB host), 18 (SIM toolkit), `F*.md`, `TEC-*.md`, `LACUNAS.md`. O wiki é de emulação: usar como fonte de contrato de plataforma, não de API do ZSDK.
- [ ] **Áudio:** limite de vozes (1 MIDI + até 4 QCP/AMR/ADPCM do mesmo tipo) levou `ZSDK_MAX_SFX_CHANNELS` a 4 no Zeebo, mas não foi confirmado para PCM; `AudioEngine` ainda não mistura tipos conforme a regra nem trata notificações de fim de mídia.
- [x] *(feito)* Detecção de gamepad (`OnGamepadConnected/Disconnected`, `input.connected`, `on_pad`); `AVK_CLR` → `Home` no caminho de teclas. Testado no emulador (com e sem pad, `--ports=none`); **desconexão em tempo de execução não foi testada** (o emulador não a simula na CLI).
- [ ] **Input:** o wiki fala em no máximo 2 gamepads; `ZSDK_MAX_GAMEPADS` é 4. Conferir com o hardware.
- [x] *(feito)* `Texture::CreateFromAtitc` (arquivo ATITC do header do SDK: assinatura 0xCCC40002, RGB/RGBA, rejeita tiled). Testado no emulador com um badge 16x16 gerado em memória (decodificou como gradiente de blocos). **Não** cobre o `.qxt` do Z-Wheel (header de 40 bytes, formato 0x0c), mipmaps nem tiled.
- [ ] **`glDrawTexOES` avaliado e não adotado:** o `SpriteBatch2D` já junta N quads em 1 draw call; `glDrawTexOES` seria 1 chamada por sprite, pior para muitos sprites. Só valeria para poucos blits grandes (fundos). Medir em hardware antes de implementar.
- [ ] **Extensões:** `HasExtension` existe, mas nenhum caminho do ZSDK escolhe fallback com base nela ainda.
- [ ] **Lifecycle:** `zeebo_core.cpp` trata START/STOP/SUSPEND/RESUME (os valores vêm do header do SDK, então o 0 do `EVT_APP_START` não é problema). Faltam: expor as flags `AEE_START_*` (`wParam`) e `EVT_APP_MESSAGE`; o `Zeebo_Initialize`/`ZeeboLib.h` do guia §10 (teclado virtual, `Zeebo_HandleEvent`) não é usado pelo ZSDK.
- [ ] **Caminhos:** o wiki nota que nomes de arquivo são sensíveis a maiúsculas; o `Vfs` compara prefixos com `strcmp` (sensível), mas os nomes de arquivo ficam a cargo do BREW. Padronizar minúsculas nas docs/exemplos.

## C — Estudo de outras SDKs (incompleto)

- [ ] Os 5 subagentes do estudo falharam (HTTP 400, modelo indisponível na integração). A análise de libdragon, libogc, KallistiOS, ps2sdk e pspsdk veio de greps diretos nos headers. **Não cobertos:** áudio (mixer/AICA/audsrv/pspaudio), rede, threads em detalhe, sistemas de build/pacotes (n64.mk, kos-ports, devkitPro), pipelines de assets. Repositórios clonados em `~/.hermes/cache/scratch/sdks/` (notas em `NOTES_API_COMPARISON.md`). Só inspiração; **não copiar código** (licenças).
- [ ] Ideias ainda não implementadas: init por flags de build (como `KOS_INIT_FLAGS`) para excluir subsistemas (Lua, rede, crypto); carregamento de assets com compressão transparente (`asset_load`/`asset_fopen`); destinos de log por build; helper de frame com múltiplos passes nomeados (3D → 2D → HUD).

## C — Testes e qualidade

- [ ] **Sem suíte de testes automatizados.** O que existe é manual: teste de host do `Vfs::Resolve` (scratch, não versionado), execuções no zeebx-emu com `--keys`. Criar `tests/` (host, stubs de `FileSystem`/`Logger`) e um smoke test no emulador (frames, log, persistência de save).
- [ ] **Avisos de compilação** do código novo não foram revisados com `-Wall -Wextra` em modo estrito; o Lua é compilado com `-w`.
- [ ] **`examples/02_lua_demo`:** jogo mínimo ("Star Catcher"); ainda não é uma demo apresentável (sem áudio, sem sprites, sem tela de config).
- [x] *(feito)* Docs 01 e README citam o exemplo 02, `ZSDK_ENABLE_LUA` e o build Linux. `lua_vm.hpp` fica fora do `zsdk.hpp` de propósito (opcional).
