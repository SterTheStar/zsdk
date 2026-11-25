# 03 — Sistema de Debug, Logs e RAM/VRAM Profiler

Headers incluídos: `debug/memory_tracker.hpp`, `debug/logger.hpp`, `debug/debug_overlay.hpp`, `debug/embedded_font.hpp`.

---

## 1. Debug Overlay In-Game (`DebugOverlay`)

O **DebugOverlay** é um painel renderizado diretamente via OpenGL ES 1.x / OpenGL sobre o framebuffer, usando uma fonte bitmap **8x8 embutida em ROM** (768 bytes, zero assets externos).

* **Atalho de Ativação:** Pressione simultaneamente **`L + D-Pad Baixo`** (no Windows: **`Q + Seta Baixo`** ou `LB + Baixo` no Xbox).
* **Discreto por padrão:** Quando fechado, não desenha nenhum indicador na tela nem altera o estado OpenGL.
* **Métricas Exibidas:**
  * FPS médio real (janela de 500 ms sem clamp), *frame time* (ms), *draw calls* (`Drw`) e quantidade de vértices (`Vtx`) submetidos no frame anterior.
  * Uso total de memória (`RAM + VRAM`), percentual em relação ao limite configurável (padrão `32 MB`) com barra colorida (verde/amarelo/vermelho) e pico de RAM.
  * Uso de VRAM (texturas + VBOs) e quebra por subsistema (`CORE`, `GFX`, `AUD`, `FS`).
  * Console com as últimas linhas de log.
* **Compilação Condicional:** Defina `ZSDK_ENABLE_DEBUG_OVERLAY=0` para remover o código do painel visual, e `ZSDK_ENABLE_LOGGING=0` em builds finais de produção para remover os logs `INFO`/`WARN` com zero overhead (mantendo `ZSDK_LOG_ERROR`).

```cpp
zsdk::debug::DebugOverlay::SetVisible(true);
bool visivel = zsdk::debug::DebugOverlay::IsVisible();
```

---

## 2. Alocadores Rastreados (`MemoryTracker`)

Todas as alocações internas do ZSDK passam pelo `MemoryTracker` (usando `MALLOC`/`REALLOC`/`FREE` do BREW no Zeebo), registrando uso atual, pico e contagem por subsistema:

* `MemorySubsystem::Core`
* `MemorySubsystem::Graphics`
* `MemorySubsystem::Audio`
* `MemorySubsystem::Filesystem`
* `MemorySubsystem::Input`
* `MemorySubsystem::Network`
* `MemorySubsystem::User`

```cpp
void* buf = zsdk::debug::MemoryTracker::Alloc(zsdk::debug::MemorySubsystem::User, 1024);
// ...
zsdk::debug::MemoryTracker::Free(zsdk::debug::MemorySubsystem::User, buf);

zsdk::debug::MemorySnapshot snap;
zsdk::debug::MemoryTracker::GetSnapshot(snap);
```

As classes `Texture` e `MeshBuffer` também contabilizam automaticamente toda alocação e liberação de **VRAM** via `RecordVramAlloc()` e `RecordVramFree()`.

### Trava Rígida de Orçamento no Windows (`SetStrictBudget`)
Para simular o limite de memória do console durante o desenvolvimento no PC:
* **Windows:** `MemoryTracker::IsStrictBudget()` vem ativo por padrão (`true`) com orçamento de 32 MiB (`SetBudgetBytes`). Se `RAM + VRAM` ultrapassar o limite, `Alloc`/`Realloc` retorna `nullptr` e `Texture::Create`/`MeshBuffer::Create` retornam `Result::OutOfMemory`. Pode ser alterado ou desativado em runtime via `MemoryTracker::SetStrictBudget(false)`.
* **Zeebo:** `SetStrictBudget` é *no-op* (`IsStrictBudget()` retorna `false`) e nunca bloqueia alocações artificiais.

---

## 3. Sistema de Logs (`Logger`)

Envia mensagens formatadas para o `__DBGPRINTF` do BREW (ou `stdout` no PC) e para o ring-buffer do `DebugOverlay`:

```cpp
ZSDK_LOG_INFO("GAME", "Fase %d carregada (%u objetos)", faseId, totalObjetos);
ZSDK_LOG_WARN("AUD",  "Canal ocupado");
ZSDK_LOG_ERROR("FS",  "Falha ao abrir arquivo");
```

* **Uso em C / Lua / FFI:** Use a ABI C99 em `zsdk/c/zsdk_c.h` (`ZSDK_C_LOG_INFO`, `zsdk_log`, `zsdk_log_msg`) ou os bindings em `bindings/lua/` (`zsdk.log.info/warn/error`). Consulte [bindings/README.md](../bindings/README.md).

> **Nota técnica:** O `Logger` e o `DebugOverlay` evitam `VSNPRINTF` e `%f` da `AEEStdLib.h` do BREW (que usa `AEEVaList` proprietária e não formata ponto flutuante), garantindo saída limpa no console do Zeebo.


## Sinks de log e `FileLogSink`

`Logger::AddSink(fn, user)` / `RemoveSink` (até 4 destinos extras, recebem a linha já formatada) e `Logger::SetMinLevel(level)` (linhas abaixo do nível são descartadas em todos os destinos). `debug::FileLogSink` (`zsdk/debug/file_log.hpp`) grava cada linha em um arquivo via `Vfs`, por exemplo `Open(vfs, "udata:/zsdk.log")` ou `sd:/zsdk.log`. Para proteger a NAND, ele para e se desregistra ao atingir `maxBytes` (padrão 64 KiB).
