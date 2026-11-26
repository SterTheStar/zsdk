# 03 Debug, logs e profiler de memória

O console entrega 32 MB de RAM e VRAM do mesmo poço e não existe sistema operacional para culpar quando algo estoura; no PC de desenvolvimento, ao contrário, nada impõe teto e o jogo cresce até a máquina reclamar. O `MemoryTracker` fecha essa lacuna: toda alocação feita pela SDK passa por ele, que soma bytes por subsistema, guarda o pico e, no Windows, recusa a alocação que passaria do orçamento configurado. O `Logger` registra o que aconteceu, com nível, tag e destinos plugáveis, sem alocar no heap por linha. O `DebugOverlay` mostra os dois na tela sobre o quadro atual, junto com FPS, tempo de frame e draw calls. Use os três durante o desenvolvimento; em build de release desligue o overlay e o log informativo pelas flags de compilação, porque nenhum deles custa zero.

## Visão rápida

| Peça | Header | O que faz |
| :--- | :--- | :--- |
| `MemoryTracker` | `debug/memory_tracker.hpp` | Contabiliza RAM por subsistema e VRAM total; impõe teto no Windows |
| `Logger` | `debug/logger.hpp` | Formata e distribui linhas para `__DBGPRINTF`/`stdout`, overlay e sinks |
| `FileLogSink` | `debug/file_log.hpp` | Sink que grava cada linha em arquivo pelo `Vfs`, com teto de bytes |
| `DebugOverlay` | `debug/debug_overlay.hpp` | Painel OpenGL ES com FPS, frame time, draw calls, RAM e VRAM |
| `GetEmbeddedFontGlyph` | `debug/embedded_font.hpp` | Fonte 8x8 de 96 glifos, 768 bytes em ROM, sem asset externo |

## Pré-requisitos

Nada precisa ser inicializado à mão quando o app usa a macro `ZSDK_IMPLEMENT_APP`: o backend chama `DebugOverlay::Initialize` no `EVT_APP_START` (Zeebo) ou na criação da janela (Windows) e aplica `AppConfig::enableDebugOverlay` como visibilidade inicial. O orçamento vem de `AppConfig::memoryBudgetBytes`, que `Application::Configure` repassa para `MemoryTracker::SetBudgetBytes`. O valor padrão do `AppConfig` é `ZSDK_DEFAULT_MEMORY_LIMIT_BYTES`, 32 MiB.

O overlay depende do `Renderer` estar funcional, porque o texto usa a fonte embutida convertida em um atlas de 128 por 64 pixels que o próprio `Renderer` sobe para VRAM na primeira vez que desenha texto. Ele também precisa de um contexto OpenGL válido: `DebugOverlay::Render` é chamado dentro de `SwapBuffers`, depois do seu `OnRender`, e por isso cobre o HUD.

Headers necessários, em qualquer combinação:

```cpp
#include "zsdk/debug/memory_tracker.hpp"
#include "zsdk/debug/logger.hpp"
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/file_log.hpp"
```

## Conceitos

### Um orçamento, dois contadores

O estado do profiler é um `MemorySnapshot` global preenchido por `MemoryTracker`. Ele guarda quatro contadores, o orçamento e um vetor de estatísticas por subsistema: `totalRamBytes` e `peakRamBytes` do heap rastreado, `totalVramBytes` e `peakVramBytes` da memória de GPU, `budgetBytes` configurado e `subsystems[MemorySubsystem::Count]`, com `currentBytes`, `peakBytes` e `allocationCount` para cada um.

Os sete subsistemas, com o nome curto que o overlay usa: `Core` (`CORE`), `Graphics` (`GFX`), `Audio` (`AUD`), `Filesystem` (`FS`), `Input` (`INP`), `Network` (`NET`) e `User` (`USR`). `MemorySubsystem::User` é o balde reservado ao código do jogo: a C ABI usa os outros seis para os objetos que cria, e qualquer alocação sua que não pertença a um módulo da SDK deve ir para `User`. `GetSubsystemName` devolve `UNK` para valor fora da faixa.

`allocationCount` conta blocos vivos, não alocações históricas: sobe em `Alloc`, cai em `Free`. `peakBytes` é o maior valor que `currentBytes` atingiu desde o início do processo, e nunca diminui. É esse número, e não o atual, que responde à pergunta "meu pico cabe no console".

Cada bloco rastreado carrega um cabeçalho de 8 bytes antes do ponteiro devolvido, com o tamanho e o subsistema original. Esse cabeçalho não entra na conta: `currentBytes` soma só o tamanho pedido. O custo real por alocação é, portanto, o tamanho mais 8 bytes no heap do BREW ou do `malloc`.

### RAM por subsistema, VRAM sem dono

RAM e VRAM são contabilizadas por caminhos diferentes. `Alloc`, `Realloc` e `Free` tratam o heap, com cabeçalho e contagem por subsistema. VRAM não tem alocador rastreado: as classes que criam objeto de GPU avisam o `MemoryTracker` por fora, com `RecordVramAlloc` e `RecordVramFree`, e o resultado vai para um contador único, sem divisão por subsistema.

Os caminhos gráficos que fazem isso hoje são `Texture::Create` e variantes (calcula `largura * altura * bytes por pixel`, consulta `CanAllocateVram` e registra depois do upload), `MeshBuffer::Create` (registra o tamanho do VBO e, quando o VBO falha, cai para um buffer de RAM contabilizado em `Graphics`), `SpriteBatch2D` (vértices e índices em RAM, em `Graphics`) e o atlas de fonte do `Renderer`, com 8192 bytes registrados na primeira vez que sobe a textura.

`CanAllocateVram` não reserva nada: ele só responde se a soma atual mais o valor pedido cabe no orçamento. Quem chama é responsável por agir no resultado, e é o que `Texture`, `MeshBuffer` e `SpriteBatch2D` fazem, devolvendo `Result::OutOfMemory` antes de tocar na GPU.

### SetStrictBudget e o teto do console no Windows

`CanAllocateVram` e a checagem interna de `Alloc` e `Realloc` compartilham a mesma regra, em `WouldExceedStrictBudget`: soma `totalRamBytes + totalVramBytes`, acrescenta o que está sendo pedido e compara com `budgetBytes`. Estourar faz `Alloc` devolver `nullptr`; `Realloc` só recusa quando o novo tamanho é maior que o antigo, e nesse caso considera apenas o crescimento como pedido.

Essa regra existe apenas no Windows. O estado `g_strictBudget` nasce `true`, então uma build de PC já começa simulando o console; `SetStrictBudget(false)` desarma a checagem para medir sem o teto. No Zeebo o flag não existe: `SetStrictBudget` é no-op e `IsStrictBudget` devolve sempre `false`, o que significa que o hardware nunca recusa uma alocação por orçamento simulado. Lá quem recusa é o `MALLOC` do BREW, quando a memória acaba de verdade.

A checagem também está dentro do bloco `ZSDK_ENABLE_MEMORY_TRACKING`. Com a contabilidade compilada fora, `Alloc` vira `malloc` cru, sem cabeçalho, e o teto deixa de ser aplicado junto. Como `ZSDK_ENABLE_MEMORY_TRACKING` herda o valor de `ZSDK_ENABLE_DEBUG_OVERLAY`, desligar o overlay em release também desliga o teto e os contadores.

### Níveis, tags e destinos do Logger

`Logger` é uma classe estática, sem instância e sem alocação. `LogLevel` tem três valores ordenados: `Info` (0), `Warn` (1) e `Error` (2). `SetMinLevel` guarda o piso, que nasce `Info`, e `LogV` compara o nível recebido com ele antes de formatar qualquer coisa, então uma linha descartada custa uma comparação.

A linha formatada sai como `[P/TAG] mensagem`, onde `P` é `I`, `W` ou `E`. O buffer é de 96 bytes em stack, definido por `ZSDK_DEBUG_LOG_LINE_LEN`, e o resultado é truncado com terminador garantido se não couber. Isso significa que o tamanho útil da mensagem é o que sobra depois de `[P/TAG] `.

Cada linha aceita vai para três lugares, nesta ordem: `__DBGPRINTF("%s", ...)` no Zeebo ou `printf("%s\n", ...)` no Windows, o ring buffer do `DebugOverlay` via `PushLog`, e até quatro sinks extras registrados com `AddSink`. `kMaxLogSinks` é 4. O sink recebe o mesmo texto que foi para o console, já com o prefixo e sem quebra de linha. `AddSink` devolve `false` quando a tabela está cheia ou quando o par `função + userData` já está registrado, e `RemoveSink` devolve `false` quando não acha o par.

As macros são o caminho normal: `ZSDK_LOG_INFO`, `ZSDK_LOG_WARN` e `ZSDK_LOG_ERROR`, todas com tag e formato `printf`. Com `ZSDK_ENABLE_LOGGING` em 0, `INFO` e `WARN` viram `((void)0)` e desaparecem do binário, enquanto `ERROR` continua chamando o `Logger`. O padrão é 1, então em builds de desenvolvimento os três níveis existem.

### O que o overlay mede, e quanto custa mantê-lo ligado

O painel abre e fecha com ombro esquerdo mais D-Pad para baixo, por `IsComboJustTriggered` no jogador 1, e `SetVisible` permite controlar por código. Quando fechado, `Render` retorna antes de tocar em qualquer estado de OpenGL.

Cada campo do painel tem uma origem precisa:

| Campo | Origem |
| :--- | :--- |
| FPS em número grande | `fpsFrames` divididos pela janela de 500 ms medida com `Clock::GetUptimeMs` |
| `FPS x.xms` | média do mesmo intervalo: `fpsAccumMs / fpsFrames` |
| `D:n V:n` | draw calls e vértices do frame anterior, zerados a cada `Update` |
| gráfico | as últimas 44 amostras de tempo de frame, com linhas de referência em 16,67 ms e 33,33 ms |
| `MEM uK/uM (x.x%) PK:uK` | `totalRamBytes + totalVramBytes`, o orçamento e `peakRamBytes` |
| barra segmentada | VRAM, `Core`, `Graphics`, `Audio`, `Filesystem` e o resto da RAM como `Other` |
| linhas de log | as últimas entradas do ring buffer, duas linhas visuais por entrada |

O FPS vem de deltas de `Clock::GetUptimeMs`, não do `deltaTimeMs` que o jogo recebe, então ele mostra a taxa real, inclusive o estouro do timer do BREW. No gráfico, a barra fica verde até 35 ms, amarela entre 35 e 44 ms e vermelha acima de 44 ms.

`V` é o valor passado para `RecordDrawCall`, que o `Renderer` preenche com `vertexCount` em `DrawArrays` e com `indexCount` em `DrawElementsU16`. Num desenho indexado, portanto, `V` conta índices, não vértices únicos.

O custo de manter o painel ligado é de desenho, não de memória. Cada retângulo e cada linha de texto é um `glDrawArrays` imediato com array de vértices no cliente: no pior caso, com o gráfico cheio, são 91 chamadas só para o gráfico (44 amostras com barra e topo, mais fundo e duas linhas de referência) e cerca de 115 no total do painel. Nada disso passa por VBO, e o `Renderer::ResetStateCache` no fim do `Render` invalida o cache de estado, então o próximo quadro do jogo reaplica o que já estava igual. Em compensação, os buffers são estáticos: 9 KB para os vértices e coordenadas de uma linha de texto de 96 caracteres, 3 KB para o ring de log e 176 bytes para o histórico de frames, tudo em `.bss`, fora do heap e portanto fora do `MEM` do próprio painel.

### Assinatura e configuração

`DebugOverlay` tem uma única instância implícita e métodos estáticos. `Initialize` reinicia os acumuladores de FPS e o relógio da janela, sem tocar em visibilidade, no histórico de frames nem no ring de log, e retorna `Result::Ok`. `Shutdown` libera o atlas de fonte compartilhado e marca o overlay como não inicializado. `Update` recebe o `InputManager` e o delta do frame, lê o combo e empurra a amostra de tempo. `Render` desenha se estiver visível.

```cpp
#include "zsdk/debug/debug_overlay.hpp"
#include "zsdk/debug/memory_tracker.hpp"

void SetupProfiler(uint32_t budgetBytes) {
    zsdk::debug::MemoryTracker::SetBudgetBytes(budgetBytes);
    zsdk::debug::MemoryTracker::SetStrictBudget(true); /* no-op no Zeebo */
    if (zsdk::Failed(zsdk::debug::DebugOverlay::Initialize())) {
        return;
    }
    zsdk::debug::DebugOverlay::SetVisible(true);
}

void FrameProfiler(const zsdk::input::InputManager& input, uint32_t deltaTimeMs) {
    zsdk::debug::DebugOverlay::Update(input, static_cast<float>(deltaTimeMs));
}

void PresentProfiler(int32_t width, int32_t height) {
    zsdk::debug::DebugOverlay::Render(width, height);
}
```

Chame `Update` antes do seu `OnUpdate`, que é a ordem que os dois backends usam, e `Render` imediatamente antes do swap. `Shutdown` pertence ao fim do app: depois dele o painel não desenha mais nada até um novo `Initialize`.

## Uso passo a passo

Passo 1, alocar pelo tracker. Toda alocação de vida longa do jogo deveria passar por aqui, senão ela não aparece em `MEM` e não participa do teto.

```cpp
#include "zsdk/debug/memory_tracker.hpp"

struct LevelData {
    uint32_t tileCount;
    uint8_t* tiles;
};

LevelData* LoadLevel(uint32_t tileCount) {
    LevelData* level = static_cast<LevelData*>(
        zsdk::debug::MemoryTracker::Alloc(zsdk::debug::MemorySubsystem::User,
                                          sizeof(LevelData)));
    if (!level) {
        return nullptr;
    }
    level->tiles = static_cast<uint8_t*>(
        zsdk::debug::MemoryTracker::Alloc(zsdk::debug::MemorySubsystem::User,
                                          tileCount));
    if (!level->tiles) {
        zsdk::debug::MemoryTracker::Free(zsdk::debug::MemorySubsystem::User, level);
        return nullptr;
    }
    level->tileCount = tileCount;
    return level;
}

void UnloadLevel(LevelData* level) {
    if (!level) {
        return;
    }
    zsdk::debug::MemoryTracker::Free(zsdk::debug::MemorySubsystem::User, level->tiles);
    zsdk::debug::MemoryTracker::Free(zsdk::debug::MemorySubsystem::User, level);
}
```

`Alloc` com tamanho 0 devolve `nullptr` sem tocar no heap, então um `tileCount` calculado que dê zero vira falha de carga, não alocação vazia. Trate o retorno sempre.

Passo 2, ler o snapshot. `GetSnapshot` copia o estado para a estrutura que você passa, então não existe ponteiro pendurado nem bloqueio. Um relatório curto cabe em uma linha por subsistema, respeitando o limite de 96 bytes do `Logger`.

```cpp
#include "zsdk/debug/logger.hpp"
#include "zsdk/debug/memory_tracker.hpp"

void LogMemoryReport() {
    zsdk::debug::MemorySnapshot snap;
    zsdk::debug::MemoryTracker::GetSnapshot(snap);

    ZSDK_LOG_INFO("MEM", "RAM %uK VRAM %uK de %uK",
                  snap.totalRamBytes / 1024u,
                  snap.totalVramBytes / 1024u,
                  snap.budgetBytes / 1024u);

    for (uint8_t i = 0u; i < static_cast<uint8_t>(zsdk::debug::MemorySubsystem::Count); ++i) {
        const zsdk::debug::SubsystemMemStats& s = snap.subsystems[i];
        if (s.allocationCount == 0u) {
            continue;
        }
        ZSDK_LOG_INFO("MEM", "%s %uK pico %uK",
                      zsdk::debug::MemoryTracker::GetSubsystemName(
                          static_cast<zsdk::debug::MemorySubsystem>(i)),
                      s.currentBytes / 1024u,
                      s.peakBytes / 1024u);
    }
}
```

Passo 3, filtrar e capturar log. `SetMinLevel` vale para todos os destinos ao mesmo tempo, inclusive para o overlay. Um sink próprio serve para contar erros, alimentar telemetria ou espelhar a linha em outro meio.

```cpp
#include "zsdk/debug/logger.hpp"

static uint32_t g_errorCount = 0u;

void CountErrors(zsdk::debug::LogLevel level, const char* line, void* userData) {
    ZSDK_UNUSED(line);
    ZSDK_UNUSED(userData);
    if (level == zsdk::debug::LogLevel::Error) {
        ++g_errorCount;
    }
}

void InstallErrorCounter() {
    if (!zsdk::debug::Logger::AddSink(&CountErrors, nullptr)) {
        ZSDK_LOG_WARN("LOG", "tabela de sinks cheia");
    }
    zsdk::debug::Logger::SetMinLevel(zsdk::debug::LogLevel::Info);
}

void RemoveErrorCounter() {
    zsdk::debug::Logger::RemoveSink(&CountErrors, nullptr);
}
```

Passo 4, gravar em arquivo. `FileLogSink::Open` abre o caminho pelo `Vfs` e se registra como sink na mesma chamada; `Close` desregistra e fecha. O teto de bytes existe para não consumir o filesystem, que é compartilhado com os saves.

```cpp
#include "zsdk/debug/file_log.hpp"
#include "zsdk/fs/vfs.hpp"

static zsdk::fs::Vfs            g_vfs;
static zsdk::debug::FileLogSink g_fileLog;

zsdk::Result StartFileLog(void* platformContext) {
    zsdk::Result r = g_vfs.Initialize(platformContext, false);
    if (zsdk::Failed(r)) {
        return r;
    }
    /* 32 KiB deixam espaço para saves no mesmo filesystem */
    r = g_fileLog.Open(g_vfs, "udata:/zsdk.log", true, 32768u);
    if (zsdk::Succeeded(r)) {
        ZSDK_LOG_INFO("FS", "log em arquivo aberto");
    }
    return r;
}

void StopFileLog() {
    g_fileLog.Close();
    g_vfs.Shutdown();
}
```

Passo 5, o mesmo pela ABI C, para apps escritos em C ou chamados por FFI. A ABI expõe leitura de totais e visibilidade do painel, não a tabela por subsistema.

```c
#include "zsdk/c/zsdk_c.h"

void ReportAndToggle(void) {
    ZSDK_C_LOG_INFO("CUBE_C", "RAM %uK VRAM %uK teto %uK",
                    (unsigned)(zsdk_memory_ram_used() / 1024u),
                    (unsigned)(zsdk_memory_vram_used() / 1024u),
                    (unsigned)(zsdk_memory_budget() / 1024u));
    zsdk_debug_overlay_set_visible(!zsdk_debug_overlay_is_visible());
}
```

## Referência da API

`MemoryTracker`, todos os métodos estáticos:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `SetBudgetBytes` | `void SetBudgetBytes(uint32_t budgetBytes)` | `0` volta ao padrão de 32 MiB |
| `GetBudgetBytes` | `uint32_t GetBudgetBytes()` | Valor atual do teto |
| `SetStrictBudget` | `void SetStrictBudget(bool enabled)` | Só tem efeito no Windows |
| `IsStrictBudget` | `bool IsStrictBudget()` | `false` no Zeebo, sempre |
| `CanAllocateVram` | `bool CanAllocateVram(uint32_t bytes)` | Consulta sem reservar |
| `Alloc` | `void* Alloc(MemorySubsystem subsystem, size_t size)` | `nullptr` com tamanho 0 ou acima do teto |
| `Realloc` | `void* Realloc(MemorySubsystem subsystem, void* ptr, size_t newSize)` | Mantém o subsistema original do bloco |
| `Free` | `void Free(MemorySubsystem subsystem, void* ptr)` | Ignora o subsistema quando o rastreio está ligado |
| `RecordVramAlloc` | `void RecordVramAlloc(uint32_t bytes)` | Soma em `totalVramBytes` e no pico |
| `RecordVramFree` | `void RecordVramFree(uint32_t bytes)` | Satura em 0, sem ficar negativo |
| `GetSnapshot` | `void GetSnapshot(MemorySnapshot& outSnapshot)` | Cópia do estado atual |
| `GetSubsystemName` | `const char* GetSubsystemName(MemorySubsystem subsystem)` | Nome curto de 4 letras ou `UNK` |

`Logger`:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `Log` | `static void Log(LogLevel level, const char* tag, const char* fmt, ...)` | Caminho por trás das macros |
| `LogV` | `static void LogV(LogLevel level, const char* tag, const char* fmt, va_list args)` | Variante com `va_list` |
| `AddSink` | `static bool AddSink(LogSinkFn fn, void* userData)` | Até 4; recusa duplicata e tabela cheia |
| `RemoveSink` | `static bool RemoveSink(LogSinkFn fn, void* userData)` | `false` se o par não existe |
| `SetMinLevel` | `static void SetMinLevel(LogLevel level)` | Padrão `Info` |
| `GetMinLevel` | `static LogLevel GetMinLevel()` | Piso atual |

`DebugOverlay`, `FileLogSink` e a ABI C:

| Símbolo | Assinatura | Observação |
| :--- | :--- | :--- |
| `DebugOverlay::Render` | `static void Render(int32_t viewportWidth, int32_t viewportHeight)` | Desenha só se visível e inicializado |
| `DebugOverlay::PushLog` | `static void PushLog(const char* message)` | Trunca em 95 caracteres |
| `DebugOverlay::RecordDrawCall` | `static void RecordDrawCall(uint32_t vertexCount = 0u)` | Conta uma chamada e soma vértices |
| `FileLogSink::Open` | `Result Open(fs::Vfs& vfs, const char* path, bool truncate = true, uint32_t maxBytes = 65536u)` | Registra como sink |
| `FileLogSink::Close` | `void Close()` | Desregistra e fecha |
| `zsdk_memory_ram_used` | `uint32_t zsdk_memory_ram_used(void)` | `totalRamBytes` |
| `zsdk_memory_vram_used` | `uint32_t zsdk_memory_vram_used(void)` | `totalVramBytes` |
| `zsdk_memory_budget` | `uint32_t zsdk_memory_budget(void)` | `budgetBytes` |
| `zsdk_debug_overlay_set_visible` | `void zsdk_debug_overlay_set_visible(int visible)` | Equivalente C de `SetVisible` |

## Limites e armadilhas

O `MemoryTracker` só vê o que passa por ele. A memória do `LuaVM`, por exemplo, fica fora: ela aparece no heap do BREW mas não entra em `totalRamBytes`, então o `MEM` do overlay mostra menos que o consumo real. O [TODO.md](../TODO.md) registra isso como limitação conhecida e pede integração sob um subsistema próprio. O mesmo vale para `MALLOC` direto em código de plataforma, para buffers estáticos e para o próprio binário carregado no heap.

VRAM não tem divisão por subsistema. `RecordVramAlloc` recebe apenas bytes, então a barra do overlay mostra VRAM como bloco único e a legenda por subsistema cobre somente quatro dos sete: `Network`, `Input` e `User` caem no segmento `Other`, junto com a RAM não classificada. O segmento `Other` é calculado por subtração, então nunca fica negativo, mas também esconde de quem é a memória.

Cada bloco rastreado custa 8 bytes a mais que o pedido, e esse overhead não é contado. Uma carga com milhares de alocações pequenas infla o heap sem mexer no número do profiler. Agrupe alocações pequenas em blocos maiores.

`Free` ignora o argumento `subsystem` quando `ZSDK_ENABLE_MEMORY_TRACKING` está ligado: o subsistema vem do cabeçalho. Isso protege contra passar o subsistema errado ao liberar, mas significa que liberar com `free()` cru, ou passar para `Free` um ponteiro que não veio de `Alloc`, lê 8 bytes antes do bloco e corrompe a contabilidade e o heap.

No Zeebo nada bloqueia por orçamento. `SetStrictBudget` é no-op e `IsStrictBudget` devolve `false`, então um jogo que passa dos 32 MiB no PC com o teto desligado vai falhar no console do jeito pior: `MALLOC` devolvendo `nullptr` longe da causa. Mantenha o teto ligado no PC.

O ring de log do overlay guarda 32 linhas de até 96 bytes (`ZSDK_DEBUG_LOG_CAPACITY` e `ZSDK_DEBUG_LOG_LINE_LEN`, ambos sobrescrevíveis por define). Uma linha que não couber é truncada no buffer de formatação, antes de chegar a qualquer destino, inclusive ao sink de arquivo.

`FileLogSink` para de escrever ao atingir `maxBytes` e se desregistra em silêncio, sem linha avisando que parou. No BREW `File::Flush` é no-op, com a suposição de que `IFILE_Write` não bufferiza; no PC o `Flush` por linha é o que mantém o log útil se o processo morrer. Não existe rotação de arquivo, e o filesystem de 1 GB é compartilhado com os saves, então o teto padrão de 64 KiB é um valor de desenvolvimento.

Com `ZSDK_ENABLE_DEBUG_OVERLAY` em 0 o overlay inteiro compila para funções inline vazias, e a flag deriva de `NDEBUG`: build de release sem `ZSDK_FORCE_DEBUG_OVERLAY` não tem painel nem contadores, e chamar `Initialize` ou `Render` ali não faz nada e não avisa. Como `ZSDK_ENABLE_MEMORY_TRACKING` herda essa flag, o teto também desaparece.

## Erros comuns

| Sintoma | Causa | Correção |
| :--- | :--- | :--- |
| `Alloc` devolve `nullptr` no PC com RAM sobrando | Soma de RAM rastreada e VRAM passou do `budgetBytes` com `SetStrictBudget(true)` | Ler `PK` no overlay ou `zsdk_memory_*`; reduzir o consumo ou subir o orçamento em `AppConfig` |
| `MEM` do overlay não fecha com a soma dos subsistemas | `Other` agrupa `Network`, `Input`, `User` e RAM não classificada | Comparar com `GetSnapshot` por subsistema antes de acusar vazamento |
| Overlay não abre com o teclado no Windows | O combo exige ombro esquerdo, e a tabela de teclado do backend não tem ombro | Usar um controle de Xbox, o item de menu do exemplo ou `SetVisible` |
| Overlay não aparece em build de release | `ZSDK_ENABLE_DEBUG_OVERLAY` vale 0 com `NDEBUG` | Definir `ZSDK_FORCE_DEBUG_OVERLAY` ou aceitar que release não tem painel |
| Contadores negativos ou corrompidos | `free()` em bloco do tracker, ou `Free` em ponteiro de `malloc` | Liberar com `MemoryTracker::Free` o que veio de `MemoryTracker::Alloc` |
| Linha de log chega cortada no meio | Buffer de 96 bytes incluindo `[P/TAG] ` | Encurtar tag e formato, ou aumentar `ZSDK_DEBUG_LOG_LINE_LEN` |
| Log em arquivo para de crescer durante a sessão | `maxBytes` atingido, sink desregistrado sem aviso | Aumentar o cap, reduzir o nível mínimo ou abrir um arquivo novo |
| Média de FPS do painel não bate com o `deltaTimeMs` | O overlay mede `Clock::GetUptimeMs`, não o delta clampado | Usar o painel como taxa real e o `deltaTimeMs` como tempo lógico do jogo |

## Veja também

- [02 Core e async](./02_core_e_async.md): `Application`, `AppConfig` e o laço de frame onde o overlay é chamado.
- [05 Gráficos 3D e 2D](./05_graficos_3d_e_2d.md): `Renderer`, texturas e VBOs, as alocações que aparecem em `GFX` e em VRAM.
- [06 Filesystem e savegame](./06_filesystem_e_savegame.md): o `Vfs` usado pelo `FileLogSink` e o filesystem compartilhado com os saves.
- [10 Arquitetura e portabilidade](./10_arquitetura_e_portabilidade.md): o orçamento de 32 MB no desenho geral da SDK.
- [memory_tracker.hpp](../include/zsdk/debug/memory_tracker.hpp), [logger.hpp](../include/zsdk/debug/logger.hpp) e [debug_overlay.hpp](../include/zsdk/debug/debug_overlay.hpp): os headers completos.
