# 03 — Sistema de Debug, Logs e RAM/VRAM Profiler

Headers incluídos: `debug/memory_tracker.hpp`, `debug/logger.hpp`, `debug/debug_overlay.hpp`, `debug/embedded_font.hpp`.

---

## 1. Debug Overlay In-Game (`DebugOverlay`)

O **DebugOverlay** é um painel renderizado diretamente via OpenGL ES 1.x / OpenGL sobre o framebuffer, usando uma fonte bitmap **8x8 embutida em ROM** (768 bytes, zero assets externos).

* **Atalho de Ativação:** Pressione simultaneamente **`START + D-Pad Baixo`** ou **`HOME + D-Pad Baixo`** (no Windows: **`Enter + Seta Baixo`**).
* **Indicador Discreto:** Quando o painel completo está fechado, exibe um pequeno aviso `START+DN:DBG` no canto superior direito.
* **Métricas Exibidas:**
  * FPS em tempo real, *frame time* (ms) e quantidade de *draw calls* do frame anterior.
  * Uso total de memória (`RAM + VRAM`), percentual em relação ao limite (padrão `32 MB`) com barra colorida (verde/amarelo/vermelho) e pico de RAM.
  * Uso de VRAM (texturas + VBOs) e quebra por subsistema (`CORE`, `GFX`, `AUD`, `FS`).
  * Console com as últimas linhas de log.
* **Compilação Condicional:** Defina `ZSDK_ENABLE_DEBUG_OVERLAY=0` em builds finais de produção para remover todo o código do painel e os logs `INFO`/`WARN` com zero overhead.

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

---

## 3. Sistema de Logs (`Logger`)

Envia mensagens formatadas para o `__DBGPRINTF` do BREW (ou `stdout` no PC) e para o ring-buffer do `DebugOverlay`:

```cpp
ZSDK_LOG_INFO("GAME", "Fase %d carregada (%u objetos)", faseId, totalObjetos);
ZSDK_LOG_WARN("AUD",  "Canal ocupado");
ZSDK_LOG_ERROR("FS",  "Falha ao abrir arquivo");
```

> **Nota técnica:** O `Logger` e o `DebugOverlay` evitam `VSNPRINTF` e `%f` da `AEEStdLib.h` do BREW (que usa `AEEVaList` proprietária e não formata ponto flutuante), garantindo saída limpa no console do Zeebo.
