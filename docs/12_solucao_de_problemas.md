# 12 Solução de problemas

Um erro no ZSDK aparece em uma de três camadas, e cada camada falha de um jeito diferente: o build quebra com mensagem de compilador ou de linker, o empacotamento produz um arquivo que existe mas não presta, e o console aceita o app e o mata em execução. Identificar a camada antes de mexer no código economiza a maior parte do tempo, porque quase todo sintoma tem uma causa única e conhecida. Este capítulo é uma tabela de sintoma para causa para correção, com a evidência que sustenta cada linha. O que ainda não foi validado em hardware está marcado como tal, com link para o [TODO.md](../TODO.md); nada aqui afirma teste que não aconteceu.

## Visão rápida

| Camada | Sintoma típico | Onde está a resposta |
| :--- | :--- | :--- |
| build | `undefined reference`, header não encontrado, opção desconhecida | [sintomas de build e link](#sintomas-de-build-e-link) |
| empacotamento | `.mif` que não instala, `.mod` que não abre o app | [sintomas de empacotamento](#sintomas-de-empacotamento-e-carga) |
| execução | crash, save perdido, textura preta, framerate instável | [sintomas no console](#sintomas-no-console) |
| ferramentas | verificador de paridade acusando divergência | [sintomas de ferramentas](#sintomas-de-ferramentas-e-paridade) |

## Conceitos

### As três camadas de falha

O build de console encadeia quatro programas: o `arm-none-eabi-g++` compila e linka com [tools/elf2mod/elf2mod.x](../tools/elf2mod/elf2mod.x), o `elf2mod.exe` converte o ELF em `.mod`, o `cifc` transforma o `.cif` em `.mif` e o console carrega os dois. Cada etapa tem um modo de falha próprio. Um símbolo faltando só aparece na primeira, um privilégio errado só aparece na quarta, e o arquivo pode existir nas duas do meio sem ser válido.

O ponto mais importante é que o CMake não aborta em todas elas. Quando o `cifc` não é encontrado, o pós-build copia o `.cif` para `mif/<alvo>.mif` e segue com sucesso. O build termina com código zero, o arquivo está lá, e o erro só aparece no console. Sempre leia as mensagens `Packaging ...` do log antes de culpar o código.

### O que o emulador não prova

O [TODO.md](../TODO.md) registra que as execuções foram no zeebx-emu, que emula o AEE em alto nível. Três consequências mudam o diagnóstico: os 60 FPS vêm do `FrameTimer` dentro do emulador e não medem o console, o mapeamento de botões do Z-Pad depende de headers HID que não temos, e as restrições do Zeebo Developer Guide sobre stack de 32 KB, ausência de libc e ausência de globais não são reproduzidas. Um app que roda no emulador pode falhar no hardware por qualquer um desses motivos, e o inverso também vale.

## Uso passo a passo

### Triagem em quatro perguntas

1. Em que camada parou? Se o build terminou com erro, nada do resto se aplica.
2. A mensagem `Packaging mif/... via cifc` apareceu? Se apareceu `cifc not found`, o manifesto é falso e o problema está no empacotamento.
3. O app aparece no console e fecha, ou nem aparece? Se nem aparece, o problema é de `.mif`, de `appletid` ou de caminho de instalação, não de código.
4. O último log impresso menciona qual subsistema? `FS`, `SAVE`, `LUA` e `GFX` são as etiquetas que o núcleo usa, e cada uma aponta para um capítulo diferente.

### O que coletar antes de investigar

O log do console sai por `__DBGPRINTF` e pelo `DebugOverlay`. Um app que morre cedo deixa pouca coisa, então vale gravar o que der com `zsdk_log_msg` no início de `zsdk_c_app_init` ou `OnInit`, e usar o `FileLogSink` quando o problema for depois de alguns segundos. O tamanho do `.mod` também informa: um arquivo de poucos KB indica link incompleto, e um de centenas de KB é o esperado para um app com Lua embutida.

### Registrando o código de retorno

Quase todo diagnóstico começa por transformar um `zsdk_result_t` em texto no log. Sem isso, um `-7` e um `-10` parecem o mesmo app quebrado.

```c
#include "zsdk/c/zsdk_c.h"

static int ensure_udata(void* platform_ctx) {
    zsdk_fs_t* fs = zsdk_fs_create();
    if (!fs) {
        return 1;
    }
    if (zsdk_fs_init(fs, platform_ctx) != ZSDK_C_OK) {
        zsdk_fs_destroy(fs);
        return 1;
    }
    const zsdk_result_t r = zsdk_fs_mkdir(fs, "fs:/~/udata");
    if (r != ZSDK_C_OK) {
        zsdk_log(ZSDK_LOG_LEVEL_WARN, "FS", "mkdir devolveu %d", (int)r);
    }
    zsdk_fs_shutdown(fs);
    zsdk_fs_destroy(fs);
    return 0;
}
```

O `MkDir` do backend Zeebo devolve `ZSDK_C_ERR_IO` tanto para diretório que já existe quanto para escrita bloqueada pelo guarda, porque o resultado do BREW é só sucesso ou falha. Quem separa os dois casos é o log: quando o guarda recusa, ele imprime `NAND guard blocked write: <caminho>` antes de devolver o erro. Por isso o exemplo acima trata o retorno como aviso e não como aborto.

## Referência da API

Os códigos de retorno são a primeira pista em quase toda falha de execução, porque a ABI devolve `zsdk_result_t` em toda operação que pode falhar. Os valores estão em [zsdk_c_types.h](../include/zsdk/c/zsdk_c_types.h) e são espelhados em C++ e Rust.

| Código | Valor | Quando aparece |
| :--- | ---: | :--- |
| `ZSDK_C_OK` | 0 | operação concluída |
| `ZSDK_C_ERR_FAILED` | -1 | falha genérica do backend |
| `ZSDK_C_ERR_INVALID_ARG` | -2 | ponteiro nulo, tamanho zero ou dado malformado |
| `ZSDK_C_ERR_OUT_OF_MEMORY` | -3 | alocação recusada pelo heap de 32 MB |
| `ZSDK_C_ERR_NOT_INITIALIZED` | -4 | objeto usado antes de `init` |
| `ZSDK_C_ERR_ALREADY_INIT` | -5 | `init` chamado duas vezes |
| `ZSDK_C_ERR_NOT_SUPPORTED` | -6 | recurso que o backend atual não tem |
| `ZSDK_C_ERR_NOT_FOUND` | -7 | arquivo ou chave ausente |
| `ZSDK_C_ERR_WOULD_BLOCK` | -8 | save assíncrono ainda pendente |
| `ZSDK_C_ERR_TIMEOUT` | -9 | operação de rede sem resposta |
| `ZSDK_C_ERR_IO` | -10 | falha de arquivo, CRC errado ou escrita bloqueada pelo guarda |
| `ZSDK_C_ERR_DEVICE_DISCONN` | -11 | gamepad ou dispositivo removido |
| `ZSDK_C_ERR_GRAPHICS` | -12 | contexto ou recurso de GPU inválido |
| `ZSDK_C_ERR_AUDIO` | -13 | falha do backend de áudio |
| `ZSDK_C_ERR_NETWORK` | -14 | falha de socket ou de rede |

## Limites e armadilhas

Estes são os limites que o código impõe e que não têm correção pelo usuário da SDK, apenas mitigação. Todos estão no [TODO.md](../TODO.md), com a prioridade.

| Limite | Valor | Consequência |
| :--- | :--- | :--- |
| stack por app (§5.1) | 32 KB | array grande em variável local derruba o app sem mensagem |
| stack padrão de `Thread` | 16384 bytes | thread criada sem `stackSizeBytes` explícito tem 16 KB |
| dados de save (§5.1) | 65536 bytes | `kMaxSaveDataBytes` recusa payload maior |
| save assíncrono | 4096 bytes | `kAsyncSaveMaxBytes` limita o que `BeginSaveSlot` aceita |
| memória do Lua | 2 MiB | `kDefaultLuaMemoryLimit` faz a alocação falhar, não travar |
| heap total (§5.1) | 32 MiB | o `.mod` inteiro é carregado nele, junto com texturas em VRAM |
| filesystem compartilhado | 1 GB | o guia manda checar espaço livre antes de gravar |
| slots de save | 16 | `kMaxSaveSlots`, com índice `uint8_t` |

## Erros comuns

### Sintomas de build e link

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `undefined reference to 'AEEMod_Load'` | alvo criado com `add_executable` em vez da macro, sem `AEEModGen.c` | use `zsdk_add_brew_app`, que soma o CRT e fixa `-e AEEMod_Load` em [01](./01_build_e_empacotamento.md#referência-da-api) |
| `undefined reference to '_sbrk'`, `'_write'`, `'_close'` | link sem `--specs=nosys.specs` ou sem `glibc_stubs.c` | mantenha as opções padrão da macro; os stubs estão em [glibc_stubs.c](../third_party/brew_crt/glibc_stubs.c) |
| `unrecognized command-line option '-mlong-calls'` em build x86 | flag ARM aplicada a alvo Windows com GCC | use o `CMakeLists.txt` atual, que só aplica `-mlong-calls` com `ZSDK_BUILD_ZEEBO` |
| `fatal error: AEEHIDButtons.h: No such file or directory` | headers HID não existem no SDK disponível | bloqueio conhecido; o [TODO.md](../TODO.md) registra que o `MapHidButtonToLogical` nunca foi validado contra o mapeamento real |
| `fatal error: ../../inc/X.h: No such file` dentro de `AEERSA.h` | include relativo com barras invertidas do Windows | copie o `X.h` para o diretório com o nome literal esperado; a pendência está no [TODO.md](../TODO.md) |
| `Permission denied` ao executar `elf2mod.exe` | pós-build rodando fora do Windows | empacote à mão com `wine`; veja [01](./01_build_e_empacotamento.md#limites-e-armadilhas) |
| `ZSDK_LUA_BYTECODE is ON, but zsdk_luac was not found` | `zsdk_luac` só é construído fora do alvo Zeebo | gere o utilitário em um build Windows antes, ou desligue o bytecode |
| app Lua não gera alvo e o build passa | `zsdk_add_lua_app` retornou por falta de `TARGET zsdk_lua` | declare o app depois de `add_subdirectory(bindings/lua)` |
| build com MSVC falha ou linka com símbolo ausente | caminho nunca validado, backend Windows com stubs | use o cross-compile MinGW-w64, que é o caminho exercitado; MSVC segue aberto no [TODO.md](../TODO.md) |

### Sintomas de empacotamento e carga

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `.mif` começa com `--` em vez de dado binário | `cifc` não encontrado e `.cif` copiado como `.mif` | instale o `cifc` nos dois `HINTS` da macro e confirme a mensagem `via cifc` |
| app não aparece no menu do console | `.mif` ausente em `/mif/` ou `mod/<alvo>/` no lugar errado | copie o par completo; os caminhos estão em [01](./01_build_e_empacotamento.md#build-para-zeebo) |
| app aparece e não abre, ou abre outro app | dois módulos com o mesmo `appletid` | passe `MIF_CLASS_ID` único; o padrão `0x01099001` é compartilhado por todo app que o omite |
| app abre mas não grava no cartão SD | falta a dependência `0x0101414E`, `AEECLSID_CARD0_ACCESS` | restaure o bloco `deps` do [template do `.mif`](../tools/mif/zsdk_template.mif.in) |
| app Lua do console não lê o script | script não foi copiado para `mod/<alvo>/` | o host procura `fs:/~/main.luac` e depois `fs:/~/main.lua`; veja [08](./08_scripting_lua.md) |

### Sintomas no console

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| crash imediato sem log ao entrar na cena | stack de 32 KB estourada por buffer local | mova o buffer para o heap ou para o objeto; o `LUAI_MAXCCALLS=48` existe pelo mesmo motivo em [01](./01_build_e_empacotamento.md#referência-da-api) |
| erro Lua `C stack overflow` em script profundo | limite de chamadas C aninhadas do runtime | reduza a recursão do script, não aumente o limite |
| `NAND guard blocked write: <caminho>` e retorno `-10` | escrita fora de `fs:/~/` e `fs:/card0/` | grave em `fs:/~/udata/` ou desligue o guarda sabendo o que faz; veja [06](./06_filesystem_e_savegame.md) |
| save antigo some depois de queda de energia | janela entre `Remove` e `Rename` no commit | nada a fazer no app: o `LoadSlot` recupera o `.tmp` completo, e o TODO registra que esse caminho nunca foi testado sob queda real |
| `CRC mismatch in <arquivo>` ao carregar save | payload gravado incompleto ou versão diferente | trate `-10` como save inválido e ofereça slot novo; o `SaveHeader` usa layout nativo, sem endianness nem padding fixados, então save de outra build pode não ser legível |
| textura carregada de ATITC sai preta ou não carrega | assinatura, dimensão, flags ou tamanho fora do contrato | confira `0xCCC40002`, dimensões até 2048, sem a flag tiled; o código detalha as rejeições em [05](./05_graficos_3d_e_2d.md) |
| textura ATITC retorna `-6` | arquivo tiled | converta para linear; `CreateFromAtitc` recusa tiled de propósito |
| 60 FPS não se sustentam no console | meta medida no emulador, não no hardware | meça no console e reduza draw calls e cubos; o TODO trata a medição como pendente |
| botões trocados no controle | mapeamento HID sem validação real | trate `GamepadButton` como provisório; veja [04](./04_input_controles.md) e o bloqueio no [TODO.md](../TODO.md) |
| save no cartão SD não persiste | `fs:/card0/udata` é suposição sem respaldo no guia | use `fs:/~/udata/`, que é o caminho documentado |

### Sintomas de ferramentas e paridade

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `not called in bindings/lua/src/*.c` | função nova na ABI sem handler Lua | escreva o handler e registre na tabela `luaL_Reg`; passos em [bindings](../bindings/README.md#como-portar-uma-função-nova) |
| `has no definition in src/c/*.cpp` | declaração sem implementação | implemente a função `extern "C"` na ponte do subsistema |
| `Enum mismatch` entre C++, C e Rust | valor ou ordem de enumerador divergente | corrija o lado que divergiu; a lista de enums comparados está em [bindings](../bindings/README.md#enumerações-compartilhadas) |
| `not declared in bindings/rust/zsdk-sys/src/*.rs` | função nova sem `extern "C"` no crate bruto | declare em `zsdk-sys` e embrulhe em `zsdk` |
| `New C++ subsystem 'include/zsdk/<nome>' has no mapping` | diretório novo em `include/zsdk` sem binding planejado | acrescente a tupla em `SUBSYSTEM_MAP`, ou o nome em `HOST_ONLY_SUBSYSTEMS`, no [verificador](../tools/check_bindings.py) |
| `references undefined handler 'l_xxx'` | entrada na tabela sem a função `static int` correspondente | corrija o nome ou escreva o handler |

## Veja também

- [01 Build e empacotamento](./01_build_e_empacotamento.md): toolchain, macros, `.elf`, `.mod` e `.mif`.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md): o orçamento de memória e os limites do hardware em contexto.
- [06 Filesystem e save](./06_filesystem_e_savegame.md): o guarda de NAND, o `Vfs` e a recuperação de save.
- [Bindings C, Lua e Rust](../bindings/README.md): o contrato da ABI e o verificador de paridade.
- [TODO.md](../TODO.md): a lista de pendências que sustenta cada limitação citada aqui.
